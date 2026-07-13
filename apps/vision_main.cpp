#include <iostream>

#include <opencv2/highgui.hpp>

#include "chess_robot/board/detector.hpp"
#include "chess_robot/exceptions.hpp"
#include "common.hpp"

namespace {

constexpr const char* kWindowCamera = "Chess Robot - Camera Feed";
constexpr const char* kWindowWarped = "Chess Robot - Warped Board";
constexpr const char* kWindowGrid = "Chess Robot - Grid Overlay";

void printControls(const chess_robot::ChessCamera& camera) {
  std::cout << "Vision test running.\n";
  if (camera.isCalibrated()) {
    std::cout << "  Lens calibration: enabled\n";
  } else {
    std::cout << "  Lens calibration: not set (run ./build/apps/calibrate_camera)\n";
  }
  std::cout << "  q - quit\n"
            << "  u - toggle undistorted/raw camera view\n"
            << "  d - toggle board detection overlay\n"
            << "  w - toggle warped board view\n"
            << "  g - toggle grid overlay\n"
            << "  Tip: save corners with ./build/apps/capture_board_corners\n";
}

}  // namespace

int main(int argc, char** argv) {
  using namespace chess_robot;
  using namespace chess_robot::apps;

  const CliOptions options =
      parseConfigArgs(argc, argv, "Chess robot vision system test loop");

  BoardDetector detector(options.config);
  auto camera = openCameraOrExit(options.config);
  if (!camera.has_value()) {
    return 1;
  }

  printControls(*camera);

  bool show_detection = true;
  bool show_warped = true;
  bool show_grid = true;
  bool show_undistorted = camera->isCalibrated();

  while (true) {
    cv::Mat frame;
    try {
      frame = camera->read(show_undistorted);
    } catch (const CameraError& exc) {
      std::cerr << "Frame read error: " << exc.what() << "\n";
      break;
    }

    std::optional<cv::Mat> corners;
    try {
      corners = detector.detectCorners(frame);
    } catch (const BoardDetectionError&) {
      corners.reset();
    }

    cv::Mat display =
        show_detection ? detector.drawCornerOverlay(frame, corners) : frame.clone();

    cv::Mat warped;
    cv::Mat grid_view;
    if (corners.has_value() && (show_warped || show_grid)) {
      warped = detector.warpBoard(frame, corners);
      if (show_grid) {
        grid_view = detector.drawGridOverlay(warped);
      }
    }

    cv::imshow(kWindowCamera, display);
    if (show_warped && !warped.empty()) {
      cv::imshow(kWindowWarped, warped);
    } else if (!show_warped) {
      safeDestroyWindow(kWindowWarped);
    }
    if (show_grid && !grid_view.empty()) {
      cv::imshow(kWindowGrid, grid_view);
    } else if (!show_grid) {
      safeDestroyWindow(kWindowGrid);
    }

    const int key = cv::waitKey(1) & 0xFF;
    if (key == 'q') {
      break;
    }
    if (key == 'u') {
      show_undistorted = !show_undistorted;
    }
    if (key == 'd') {
      show_detection = !show_detection;
    }
    if (key == 'w') {
      show_warped = !show_warped;
    }
    if (key == 'g') {
      show_grid = !show_grid;
    }
  }

  camera->release();
  destroyGuiWindows();
  return 0;
}
