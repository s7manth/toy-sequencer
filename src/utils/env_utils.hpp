#include <cstdlib>
#include <fstream>

class EnvUtils {
private:
  static void load_env(const std::string &path) {
    std::ifstream file(path);
    std::string line;
    while (std::getline(file, line)) {
      if (line.empty() || line[0] == '#')
        continue;
      auto pos = line.find('=');
      if (pos == std::string::npos)
        continue;
      std::string key = line.substr(0, pos);
      std::string val = line.substr(pos + 1);
      setenv(key.c_str(), val.c_str(), 1); // overwrite = 1
    }
  }

public:
  static void load_env() { load_env(".env"); }
};
