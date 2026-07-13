#pragma once

#include <optional>
#include <string>
#include <vector>

#include <opencv2/videoio.hpp>

namespace chess_robot {

struct DeviceInfo {
  std::string path;
  std::string resolution;
  bool works = true;
};

/** Prefer V4L2 on Linux. */
int videoBackend();

cv::VideoCapture createCapture(const std::string& device);
cv::VideoCapture createCapture(int device_index);

std::optional<cv::VideoCapture> probeCapture(const std::string& device);
std::optional<cv::VideoCapture> probeCapture(int device_index);

std::vector<std::string> buildDeviceCandidates(const std::optional<std::string>& device_path,
                                               int device_id,
                                               int max_index = 10);

struct DiscoveredCapture {
  std::string device_label;
  cv::VideoCapture capture;
};

std::optional<DiscoveredCapture> discoverCapture(const std::optional<std::string>& device_path,
                                                 int device_id);

std::vector<DeviceInfo> listDevices(int max_index = 10);

}  // namespace chess_robot
