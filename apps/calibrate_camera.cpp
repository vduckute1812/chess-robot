#include <iostream>

#include <opencv2/highgui.hpp>

#include "chess_robot/calibration/calibrator.hpp"
#include "chess_robot/exceptions.hpp"
#include "common.hpp"

namespace {

constexpr const char* kWindow = "Chess Robot - Camera Calibration";

void printControls(const chess_robot::CameraCalibrator& calibrator) {
  std::cout << "Camera calibration running.\n"
            << "  Pattern: " << calibrator.cols() << "x" << calibrator.rows()
            << " inner corners\n"
            << "  Target samples: " << calibrator.minSamples() << "\n"
            << "  SPACE - capture sample when checkerboard is detected\n"
            << "  c     - compute calibration and save to config\n"
            << "  r     - reset collected samples\n"
            << "  q     - quit\n";
}

}  // namespace

int main(int argc, char** argv) {
  using namespace chess_robot;
  using namespace chess_robot::apps;

  const CliOptions options =
      parseConfigArgs(argc, argv, "Calibrate webcam intrinsics with a checkerboard");

  CameraCalibrator calibrator(options.config);
  auto camera = openCameraOrExit(options.config);
  if (!camera.has_value()) {
    return 1;
  }

  printControls(calibrator);

  while (true) {
    cv::Mat frame;
    try {
      frame = camera->read(false);
    } catch (const CameraError& exc) {
      std::cerr << "Frame read error: " << exc.what() << "\n";
      break;
    }

    cv::Mat overlay = calibrator.drawOverlay(frame);
    cv::imshow(kWindow, overlay);

    const int key = cv::waitKey(1) & 0xFF;
    if (key == 'q') {
      break;
    }
    if (key == 'r') {
      calibrator.resetSamples();
      std::cout << "Reset calibration samples.\n";
    }
    if (key == ' ') {
      if (calibrator.addSample(frame)) {
        std::cout << "Captured sample " << calibrator.sampleCount() << ".\n";
      } else {
        std::cout << "Checkerboard not detected. Adjust board and try again.\n";
      }
    }
    if (key == 'c') {
      try {
        auto [matrix, dist, error] = calibrator.calibrate();
        (void)matrix;
        (void)dist;
        const fs::path saved = calibrator.saveToConfig(options.config);
        std::cout << "Calibration saved to " << saved << "\n"
                  << "Reprojection error: " << error << " pixels\n"
                  << "Restart the vision loop to use undistorted frames.\n";
        break;
      } catch (const CalibrationError& exc) {
        std::cout << "Calibration error: " << exc.what() << "\n";
      }
    }
  }

  camera->release();
  destroyGuiWindows();
  return 0;
}
