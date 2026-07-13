#pragma once

#include <filesystem>
#include <optional>
#include <string>

#include "chess_robot/camera/capture.hpp"

namespace chess_robot::apps {

namespace fs = std::filesystem;

struct CliOptions {
  fs::path config = fs::path("config") / "camera_config.yaml";
};

CliOptions parseConfigArgs(int argc, char** argv, const std::string& description);

/** Open camera or print error and return nullopt. */
std::optional<ChessCamera> openCameraOrExit(const fs::path& config_path);

void destroyGuiWindows();

/** destroyWindow that no-ops if the window was never created (Qt backend aborts otherwise). */
void safeDestroyWindow(const std::string& name);

}  // namespace chess_robot::apps
