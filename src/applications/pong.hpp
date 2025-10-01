#include "generated/messages.pb.h"
#include <functional>
#include <string>

class PongApp {
public:
  explicit PongApp(std::function<void(const std::string &)> log);

  void on_event(const toysequencer::TextEvent &event);

private:
  std::function<void(const std::string &)> log_;
};
