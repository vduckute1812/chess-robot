#include <iostream>

#include "chess_robot/camera/capture.hpp"
#include "common.hpp"

int main(int argc, char** argv) {
  using namespace chess_robot;
  using namespace chess_robot::apps;

  parseConfigArgs(argc, argv, "List available camera devices");
  const auto devices = ChessCamera::listDevices();
  if (devices.empty()) {
    std::cout << "No working cameras found.\n"
              << "Tips:\n"
              << "  - Plug in the USB webcam\n"
              << "  - Close other apps using the camera\n"
              << "  - Add your user to the video group: sudo usermod -aG video $USER\n";
    return 1;
  }

  std::cout << "Working cameras:\n";
  for (const DeviceInfo& device : devices) {
    std::cout << "  " << device.path << ": " << device.resolution << "\n";
  }
  return 0;
}
