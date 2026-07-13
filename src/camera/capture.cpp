#include "chess_robot/camera/capture.hpp"

#include <iostream>
#include <sstream>

#include "chess_robot/calibration/matrices.hpp"
#include "chess_robot/config.hpp"
#include "chess_robot/exceptions.hpp"

namespace chess_robot {

ChessCamera::ChessCamera(const std::optional<fs::path>& config_path)
    : config_path_(resolveConfigPath(config_path)) {
  YAML::Node camera = loadSection("camera", config_path_);
  device_id_ = camera["device_id"] ? camera["device_id"].as<int>() : 0;
  if (camera["device_path"] && !camera["device_path"].IsNull()) {
    const std::string path = camera["device_path"].as<std::string>();
    if (!path.empty() && path != "null") {
      device_path_ = path;
    }
  }
  width_ = camera["width"] ? camera["width"].as<int>() : 1280;
  height_ = camera["height"] ? camera["height"].as<int>() : 720;
  fps_ = camera["fps"] ? camera["fps"].as<int>() : 30;
  loadUndistorter();
}

void ChessCamera::loadUndistorter() {
  auto matrices = loadCalibrationMatrices(config_path_);
  if (!matrices.has_value()) {
    return;
  }
  undistorter_ = std::make_unique<FrameUndistorter>(
      matrices->camera_matrix, matrices->dist_coeffs, matrices->image_size);
}

bool ChessCamera::isOpen() const {
  return opened_ && cap_.isOpened();
}

std::vector<DeviceInfo> ChessCamera::listDevices(int max_index) {
  return chess_robot::listDevices(max_index);
}

void ChessCamera::open() {
  if (isOpen()) {
    return;
  }

  auto discovered = discoverCapture(device_path_, device_id_);
  if (!discovered.has_value()) {
    const auto available = listDevices();
    std::ostringstream names;
    if (available.empty()) {
      names << "none";
    } else {
      for (std::size_t i = 0; i < available.size(); ++i) {
        if (i > 0) {
          names << ", ";
        }
        names << available[i].path;
      }
    }
    throw CameraError(
        "Failed to open any camera device. "
        "Check that the webcam is connected, not in use by another app, "
        "and that your user is in the 'video' group. "
        "Set camera.device_path or camera.device_id in config/camera_config.yaml. "
        "Working devices found: " +
        names.str() + ".");
  }

  active_device_ = discovered->device_label;
  const std::string configured =
      device_path_.has_value() ? *device_path_ : std::to_string(device_id_);
  if (*active_device_ != configured) {
    std::cout << "Camera: using " << *active_device_ << " (configured " << configured
              << " unavailable)\n";
  }

  cap_ = std::move(discovered->capture);
  cap_.set(cv::CAP_PROP_FRAME_WIDTH, width_);
  cap_.set(cv::CAP_PROP_FRAME_HEIGHT, height_);
  cap_.set(cv::CAP_PROP_FPS, fps_);
  opened_ = true;
  if (undistorter_) {
    undistorter_->reset();
  }
}

cv::Mat ChessCamera::read(bool apply_undistort) {
  if (!isOpen()) {
    throw CameraError("Camera is not open. Call open() first.");
  }

  cv::Mat frame;
  if (!cap_.read(frame) || frame.empty()) {
    throw CameraError(
        "Failed to read frame from camera. "
        "The device may have been disconnected.");
  }

  if (apply_undistort && undistorter_) {
    return undistorter_->apply(frame);
  }
  return frame;
}

void ChessCamera::release() {
  if (cap_.isOpened()) {
    cap_.release();
  }
  opened_ = false;
  active_device_.reset();
  if (undistorter_) {
    undistorter_->reset();
  }
}

}  // namespace chess_robot
