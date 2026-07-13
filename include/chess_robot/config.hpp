#pragma once

#include <filesystem>
#include <optional>
#include <string>

#include <yaml-cpp/yaml.h>

namespace chess_robot {

namespace fs = std::filesystem;

/** Default path: config/camera_config.yaml relative to the process CWD. */
fs::path defaultConfigPath();

fs::path resolveConfigPath(const std::optional<fs::path>& config_path = std::nullopt);

YAML::Node loadConfig(const std::optional<fs::path>& config_path = std::nullopt);

YAML::Node loadSection(const std::string& section,
                       const std::optional<fs::path>& config_path = std::nullopt);

fs::path saveConfig(const YAML::Node& data,
                    const std::optional<fs::path>& config_path = std::nullopt);

}  // namespace chess_robot
