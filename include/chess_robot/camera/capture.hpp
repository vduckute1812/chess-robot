#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>

#include "chess_robot/camera/discovery.hpp"
#include "chess_robot/camera/undistort.hpp"

namespace chess_robot {

namespace fs = std::filesystem;

class ChessCamera {
 public:
  explicit ChessCamera(const std::optional<fs::path>& config_path = std::nullopt);

  int deviceId() const { return device_id_; }
  const std::optional<std::string>& devicePath() const { return device_path_; }
  int width() const { return width_; }
  int height() const { return height_; }
  int fps() const { return fps_; }
  bool isOpen() const;
  bool isCalibrated() const { return undistorter_ != nullptr; }
  const std::optional<std::string>& activeDevice() const { return active_device_; }

  static std::vector<DeviceInfo> listDevices(int max_index = 10);

  void open();
  cv::Mat read(bool apply_undistort = true);
  void release();

 private:
  void loadUndistorter();

  fs::path config_path_;
  int device_id_ = 0;
  std::optional<std::string> device_path_;
  int width_ = 1280;
  int height_ = 720;
  int fps_ = 30;

  cv::VideoCapture cap_;
  bool opened_ = false;
  std::optional<std::string> active_device_;
  std::unique_ptr<FrameUndistorter> undistorter_;
};

}  // namespace chess_robot
