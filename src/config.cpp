#include "chess_robot/config.hpp"

#include <fstream>
#include <stdexcept>

namespace chess_robot {

fs::path defaultConfigPath() {
  return fs::path("config") / "camera_config.yaml";
}

fs::path resolveConfigPath(const std::optional<fs::path>& config_path) {
  if (!config_path.has_value() || config_path->empty()) {
    return defaultConfigPath();
  }
  return *config_path;
}

YAML::Node loadConfig(const std::optional<fs::path>& config_path) {
  const fs::path path = resolveConfigPath(config_path);
  if (!fs::exists(path)) {
    throw std::runtime_error("Camera config not found: " + path.string());
  }
  YAML::Node data = YAML::LoadFile(path.string());
  return data ? data : YAML::Node(YAML::NodeType::Map);
}

YAML::Node loadSection(const std::string& section,
                       const std::optional<fs::path>& config_path) {
  YAML::Node config = loadConfig(config_path);
  if (!config[section]) {
    return YAML::Node(YAML::NodeType::Map);
  }
  return config[section];
}

fs::path saveConfig(const YAML::Node& data,
                    const std::optional<fs::path>& config_path) {
  const fs::path path = resolveConfigPath(config_path);
  if (path.has_parent_path()) {
    fs::create_directories(path.parent_path());
  }
  std::ofstream out(path);
  if (!out) {
    throw std::runtime_error("Failed to write config: " + path.string());
  }
  out << data;
  return path;
}

}  // namespace chess_robot
