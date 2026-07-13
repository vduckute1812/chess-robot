#include <iostream>

#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include "chess_robot/board/detector.hpp"
#include "chess_robot/exceptions.hpp"
#include "common.hpp"

namespace {

constexpr const char* kWindowCamera = "Chess Robot - Capture Board Corners";
constexpr const char* kWindowWarped = "Chess Robot - Warped Preview";

void printControls(const chess_robot::ChessCamera& camera,
                   const chess_robot::BoardDetector& detector) {
  std::cout << "Board corner capture running.\n";
  if (camera.isCalibrated()) {
    std::cout << "  Lens calibration: enabled (undistorted preview)\n";
  } else {
    std::cout << "  Lens calibration: not set (run ./build/apps/calibrate_camera)\n";
  }
  if (detector.hasManualCorners()) {
    std::cout << "  Config currently has manual_corners (preview uses live auto-detect)\n";
  }
  std::cout << "  s - save live-detected corners to board.manual_corners\n"
            << "  c - clear manual_corners from config\n"
            << "  q - quit\n";
}

}  // namespace

int main(int argc, char** argv) {
  using namespace chess_robot;
  using namespace chess_robot::apps;

  const CliOptions options =
      parseConfigArgs(argc, argv, "Capture chessboard corners and save them to config");

  BoardDetector detector(options.config);
  auto camera = openCameraOrExit(options.config);
  if (!camera.has_value()) {
    return 1;
  }

  printControls(*camera, detector);
  const bool use_undistort = camera->isCalibrated();

  while (true) {
    cv::Mat frame;
    try {
      frame = camera->read(use_undistort);
    } catch (const CameraError& exc) {
      std::cerr << "Frame read error: " << exc.what() << "\n";
      break;
    }

    std::optional<cv::Mat> corners;
    try {
      corners = detector.detectCorners(frame, /*use_manual=*/false);
    } catch (const BoardDetectionError&) {
      corners.reset();
    }

    cv::Mat display = detector.drawCornerOverlay(frame, corners);
    const std::string status =
        corners.has_value() ? "DETECTED - press s to save" : "Searching for board...";
    cv::putText(display,
                status,
                cv::Point(20, 40),
                cv::FONT_HERSHEY_SIMPLEX,
                0.8,
                corners.has_value() ? cv::Scalar(0, 255, 0) : cv::Scalar(0, 165, 255),
                2,
                cv::LINE_AA);
    cv::imshow(kWindowCamera, display);

    if (corners.has_value()) {
      cv::Mat warped = detector.warpBoard(frame, corners);
      cv::imshow(kWindowWarped, detector.drawGridOverlay(warped));
    } else {
      safeDestroyWindow(kWindowWarped);
    }

    const int key = cv::waitKey(1) & 0xFF;
    if (key == 'q') {
      break;
    }
    if (key == 's') {
      try {
        const fs::path saved = detector.saveCornersToConfig(corners, options.config);
        std::cout << "Saved manual_corners to " << saved << "\n"
                  << "Vision loop will use these corners until cleared.\n";
      } catch (const BoardDetectionError& exc) {
        std::cout << "Could not save corners: " << exc.what() << "\n";
      }
    }
    if (key == 'c') {
      const fs::path saved = detector.clearManualCorners(options.config);
      std::cout << "Cleared manual_corners in " << saved << "\n";
    }
  }

  camera->release();
  destroyGuiWindows();
  return 0;
}
