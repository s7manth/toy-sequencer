## Building

```shell
cmake -S . -B build && cmake --build build
```

## How to run the applications?

Each application runs on its own thread independently. The build script would build all the applications available.
Ping and Pong are applications written to illustrate the behavior of the sequencer.

In order to run the application, a `.env` file is necessary. An example `.env.example` has been provided.

There will mainly be two different udp address/port combinations. One for commands and the other for events.

To run any application:

```
./build/src/<app name>
```

A little about the applications:

- sequencer: Orders incoming commands deterministically and emits sequenced events.
- md: Ingests market data (HTTP SSE) and publishes TopOfBook commands.
- scrappy: Subscribes to events and writes them to a file for inspection.

Test applications:

- ping: Sends a PING command and logs the sequenced ack event.
- pong: Listens for sequenced event from ping and replies with a PONG command.

### Aside

To get mock market data:

```
python extern/mdapi.py --symbols AAPL,MSFT --interval 0.1
```

## Testing

```shell
# Run all tests
ctest --test-dir build

# Run tests with verbose output
ctest --test-dir build --verbose

# Run tests and show output on failure
ctest --test-dir build --output-on-failure
```
