"""
Simple Market Data API with mock Top-of-Book updates and a subscription model.

Subscribers register callbacks to receive updates of the form:
  TopOfBook(symbol, bid_price, bid_size, ask_price, ask_size, timestamp)

Usage example:

    from extern.mdapi import MarketDataAPI, TopOfBook

    def on_md(top: TopOfBook) -> None:
        print(top)

    api = MarketDataAPI(symbols=["AAPL", "MSFT"], update_interval_seconds=0.25)
    api.start()
    sub_id = api.subscribe(on_md)              # receive all symbols
    # sub_id = api.subscribe(on_md, {"AAPL"}) # receive only AAPL

    # ... later ...
    api.unsubscribe(sub_id)
    api.stop()
"""

from __future__ import annotations

import random
import threading
import time
from dataclasses import dataclass
from typing import Callable, Dict, Iterable, Optional, Set, Tuple


@dataclass(frozen=True)
class TopOfBook:
    symbol: str
    bid_price: float
    bid_size: int
    ask_price: float
    ask_size: int
    timestamp: float


SubscriberCallback = Callable[[TopOfBook], None]


class MarketDataAPI:
    """
    Generates mock top-of-book data on a background thread and dispatches to subscribers.

    - Prices evolve via a bounded random walk per symbol
    - Bid/ask maintain a fixed minimum spread
    - Sizes are randomized in a range
    """

    def __init__(
        self,
        symbols: Optional[Iterable[str]] = None,
        update_interval_seconds: float = 0.5,
        random_seed: Optional[int] = None,
    ) -> None:
        self._symbols: Tuple[str, ...] = tuple(symbols or ("AAPL", "MSFT", "GOOG", "AMZN"))
        self._update_interval_seconds: float = max(0.01, float(update_interval_seconds))
        self._rng = random.Random(random_seed)

        # Per-symbol state: mid price baseline and current spread
        self._mid_price_by_symbol: Dict[str, float] = {
            symbol: self._rng.uniform(50.0, 300.0) for symbol in self._symbols
        }

        # Subscribers: id -> (callback, symbols filter or None for all)
        self._next_subscription_id: int = 1
        self._subscribers: Dict[int, Tuple[SubscriberCallback, Optional[Set[str]]]] = {}

        # Threading
        self._lock = threading.RLock()
        self._running = False
        self._thread: Optional[threading.Thread] = None

        # Spread and size parameters
        self._min_tick: float = 0.01
        self._min_spread_ticks: int = 2
        self._max_spread_ticks: int = 8
        self._size_min: int = 10
        self._size_max: int = 500

        # Random walk step parameters
        self._max_mid_move_per_tick: float = 0.05

    # Public API -----------------------------------------------------------------
    def start(self) -> None:
        with self._lock:
            if self._running:
                return
            self._running = True
            self._thread = threading.Thread(target=self._run_loop, name="mdapi-generator", daemon=True)
            self._thread.start()

    def stop(self) -> None:
        with self._lock:
            self._running = False
        if self._thread is not None:
            self._thread.join(timeout=max(1.0, 3 * self._update_interval_seconds))
            self._thread = None

    def subscribe(self, callback: SubscriberCallback, symbols: Optional[Iterable[str]] = None) -> int:
        """
        Subscribe to receive TopOfBook updates. If `symbols` is provided, only
        updates for those symbols are delivered; otherwise, all symbols are delivered.
        Returns an integer subscription id for later unsubscription.
        """
        if not callable(callback):
            raise TypeError("callback must be callable")
        with self._lock:
            subscription_id = self._next_subscription_id
            self._next_subscription_id += 1
            symbol_filter = None if symbols is None else set(symbols)
            self._subscribers[subscription_id] = (callback, symbol_filter)
            return subscription_id

    def unsubscribe(self, subscription_id: int) -> bool:
        with self._lock:
            return self._subscribers.pop(subscription_id, None) is not None

    def publish_top_of_book(self, top: TopOfBook) -> None:
        """Manually publish a TopOfBook update to all interested subscribers."""
        self._dispatch(top)

    # Internal -------------------------------------------------------------------
    def _run_loop(self) -> None:
        next_wake = time.time()
        while True:
            with self._lock:
                if not self._running:
                    break
                symbols_snapshot = self._symbols
            now = time.time()
            if now < next_wake:
                time.sleep(min(0.01, next_wake - now))
                continue
            next_wake = now + self._update_interval_seconds

            # Generate and dispatch one update per symbol per tick
            for symbol in symbols_snapshot:
                top = self._generate_top_of_book(symbol)
                self._dispatch(top)

    def _generate_top_of_book(self, symbol: str) -> TopOfBook:
        # Mid price random walk
        mid = self._mid_price_by_symbol[symbol]
        mid += self._rng.uniform(-self._max_mid_move_per_tick, self._max_mid_move_per_tick)
        mid = max(1.0, mid)
        self._mid_price_by_symbol[symbol] = mid

        spread_ticks = self._rng.randint(self._min_spread_ticks, self._max_spread_ticks)
        spread = spread_ticks * self._min_tick

        bid = self._round_down_to_tick(mid - spread / 2.0)
        ask = self._round_up_to_tick(max(bid + spread, mid + spread / 2.0))

        bid_size = self._rng.randint(self._size_min, self._size_max)
        ask_size = self._rng.randint(self._size_min, self._size_max)

        return TopOfBook(
            symbol=symbol,
            bid_price=bid,
            bid_size=bid_size,
            ask_price=ask,
            ask_size=ask_size,
            timestamp=time.time(),
        )

    def _dispatch(self, top: TopOfBook) -> None:
        # Copy subscribers under lock, then call outside lock to avoid deadlocks
        with self._lock:
            subscribers_snapshot = list(self._subscribers.items())
        for _, (callback, symbol_filter) in subscribers_snapshot:
            if symbol_filter is not None and top.symbol not in symbol_filter:
                continue
            try:
                callback(top)
            except Exception:
                # Swallow callback exceptions to avoid disrupting the feed
                pass

    def _round_down_to_tick(self, price: float) -> float:
        ticks = int(price / self._min_tick)
        return ticks * self._min_tick

    def _round_up_to_tick(self, price: float) -> float:
        ticks = int((price + self._min_tick - 1e-12) / self._min_tick)
        return ticks * self._min_tick


__all__ = [
    "TopOfBook",
    "MarketDataAPI",
]


# --------------------------- Script Entrypoint (HTTP) ---------------------------
def _serve_http(api: MarketDataAPI, host: str = "127.0.0.1", port: int = 8000) -> None:
    """
    Start a very small HTTP server exposing endpoints to inspect the latest
    top-of-book values generated by the API.

    Endpoints:
      - GET /symbols            -> ["AAPL", "MSFT", ...]
      - GET /top                -> { "AAPL": {...}, ... }
      - GET /top/<symbol>       -> { "symbol": "AAPL", ... }
    """
    import json
    import queue
    from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
    from urllib.parse import urlparse

    # Maintain the latest snapshot for each symbol
    latest_by_symbol: Dict[str, TopOfBook] = {}
    latest_lock = threading.RLock()

    def on_update(top: TopOfBook) -> None:
        with latest_lock:
            latest_by_symbol[top.symbol] = top

    # Warm up snapshot quickly
    api.subscribe(on_update)

    def tob_to_dict(t: TopOfBook) -> Dict[str, object]:
        return {
            "symbol": t.symbol,
            "bid_price": t.bid_price,
            "bid_size": t.bid_size,
            "ask_price": t.ask_price,
            "ask_size": t.ask_size,
            "timestamp": t.timestamp,
        }

    class Handler(BaseHTTPRequestHandler):
        # Swallow connection resets that can occur before headers are parsed
        def handle(self) -> None:  # type: ignore[override]
            try:
                super().handle()
            except (ConnectionResetError, BrokenPipeError):
                pass
            except Exception:
                # Avoid noisy tracebacks on abrupt disconnects
                pass

        def log_message(self, format: str, *args) -> None:  # silence default noisy logs
            return

        def _send_json(self, payload: object, status: int = 200) -> None:
            data = json.dumps(payload).encode("utf-8")
            self.send_response(status)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(data)))
            self.end_headers()
            self.wfile.write(data)

        def do_GET(self) -> None:  # type: ignore[override]
            parsed = urlparse(self.path or "/")
            path_parts = [p for p in parsed.path.split("/") if p]
            if not path_parts:
                return self._send_json({
                    "endpoints": [
                        "/symbols",
                        "/top",
                        "/top/<symbol>",
                        "/stream",
                        "/stream/<symbol>",
                    ]
                })

            # SSE streaming endpoints -------------------------------------------------
            if path_parts[0] == "stream":
                symbol_filter = None
                if len(path_parts) == 2 and path_parts[1]:
                    symbol = path_parts[1].upper()
                    if symbol not in api._symbols:
                        return self._send_json({"error": f"unknown symbol: {symbol}"}, status=404)
                    symbol_filter = {symbol}

                self.send_response(200)
                self.send_header("Content-Type", "text/event-stream")
                self.send_header("Cache-Control", "no-cache")
                self.send_header("Connection", "keep-alive")
                self.send_header("Access-Control-Allow-Origin", "*")
                self.end_headers()

                # Per-connection queue to buffer events; drop-oldest if slow client
                event_queue: "queue.Queue[Dict[str, object]]" = queue.Queue(maxsize=1024)

                def on_event(top: TopOfBook) -> None:
                    if symbol_filter is not None and top.symbol not in symbol_filter:
                        return
                    item = tob_to_dict(top)
                    try:
                        event_queue.put_nowait(item)
                    except queue.Full:
                        try:
                            _ = event_queue.get_nowait()
                        except queue.Empty:
                            pass
                        try:
                            event_queue.put_nowait(item)
                        except queue.Full:
                            pass

                sub_id = api.subscribe(on_event, symbols=symbol_filter)
                try:
                    # Send an initial comment and retry hint
                    try:
                        self.wfile.write(b": connected\n")
                        self.wfile.write(b"retry: 2000\n\n")
                        self.wfile.flush()
                    except Exception:
                        return

                    last_keepalive = time.time()
                    while True:
                        try:
                            item = event_queue.get(timeout=15.0)
                            payload = json.dumps(item).encode("utf-8")
                            self.wfile.write(b"data: ")
                            self.wfile.write(payload)
                            self.wfile.write(b"\n\n")
                            self.wfile.flush()
                        except queue.Empty:
                            # Heartbeat to keep connection alive through proxies
                            now = time.time()
                            if now - last_keepalive >= 15.0:
                                try:
                                    self.wfile.write(b": keepalive\n\n")
                                    self.wfile.flush()
                                except Exception:
                                    break
                                last_keepalive = now
                        except BrokenPipeError:
                            break
                        except ConnectionResetError:
                            break
                        except Exception:
                            # Any other IO error ends the stream
                            break
                finally:
                    api.unsubscribe(sub_id)
                return

            if path_parts[0] == "symbols" and len(path_parts) == 1:
                return self._send_json(list(api._symbols))

            if path_parts[0] == "top" and len(path_parts) == 1:
                with latest_lock:
                    snap = {s: tob_to_dict(t) for s, t in latest_by_symbol.items()}
                return self._send_json(snap)

            if path_parts[0] == "top" and len(path_parts) == 2:
                symbol = path_parts[1].upper()
                if symbol not in api._symbols:
                    return self._send_json({"error": f"unknown symbol: {symbol}"}, status=404)
                with latest_lock:
                    t = latest_by_symbol.get(symbol)
                if t is None:
                    return self._send_json({"error": "no data yet"}, status=503)
                return self._send_json(tob_to_dict(t))

            return self._send_json({"error": "not found"}, status=404)

    httpd = ThreadingHTTPServer((host, port), Handler)
    print(f"[mdapi] Serving HTTP on http://{host}:{port} ...")
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        httpd.server_close()


def _parse_args_and_run() -> None:
    import argparse

    parser = argparse.ArgumentParser(description="Mock Market Data API (Top-of-Book)")
    parser.add_argument("--symbols", type=str, default="AAPL,MSFT,GOOG,AMZN", help="Comma-separated symbols")
    parser.add_argument("--interval", type=float, default=0.25, help="Update interval seconds")
    parser.add_argument("--host", type=str, default="127.0.0.1", help="HTTP host")
    parser.add_argument("--port", type=int, default=8000, help="HTTP port")
    args = parser.parse_args()

    symbols = tuple(s.strip().upper() for s in args.symbols.split(",") if s.strip())
    api = MarketDataAPI(symbols=symbols, update_interval_seconds=args.interval)
    api.start()
    print(f"[mdapi] Started generator for symbols: {', '.join(symbols)} @ interval={args.interval}s")
    try:
        _serve_http(api, host=args.host, port=args.port)
    finally:
        api.stop()
        print("[mdapi] Stopped.")


if __name__ == "__main__":
    _parse_args_and_run()


