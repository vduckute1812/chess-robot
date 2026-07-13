#include "common.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

#include <opencv2/highgui.hpp>

#include "chess_robot/exceptions.hpp"

namespace chess_robot::apps {

CliOptions parseConfigArgs(int argc, char** argv, const std::string& description) {
  CliOptions options;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "-h" || arg == "--help") {
      std::cout << description << "\n\nOptions:\n  --config PATH   Path to camera_config.yaml\n  -h, --help      Show this help\n";
      std::exit(0);
    }
    if (arg == "--config") {
      if (i + 1 >= argc) {
        std::cerr << "--config requires a path\n";
        std::exit(2);
      }
      options.config = argv[++i];
      continue;
    }
    std::cerr << "Unknown argument: " << arg << "\n";
    std::exit(2);
  }
  return options;
}

std::optional<ChessCamera> openCameraOrExit(const fs::path& config_path) {
  try {
    ChessCamera camera(config_path);
    camera.open();
    return camera;
  } catch (const CameraError& exc) {
    std::cerr << "Camera error: " << exc.what() << "\n";
  } catch (const std::exception& exc) {
    std::cerr << "Camera error: " << exc.what() << "\n";
  }
  return std::nullopt;
}

void destroyGuiWindows() {
  try {
    cv::destroyAllWindows();
  } catch (const cv::Exception&) {
  }
}

void safeDestroyWindow(const std::string& name) {
  try {
    cv::destroyWindow(name);
  } catch (const cv::Exception&) {
    // Qt backend throws if the window was never created.
  }
}

}  // namespace chess_robot::apps
