#include "chess_robot/camera/discovery.hpp"

#include <cstdlib>
#include <filesystem>
#include <set>
#include <sstream>

#include <opencv2/core/utils/logger.hpp>

namespace chess_robot {

namespace fs = std::filesystem;

int videoBackend() {
#ifdef __linux__
  return cv::CAP_V4L2;
#else
  return cv::CAP_ANY;
#endif
}

cv::VideoCapture createCapture(const std::string& device) {
  return cv::VideoCapture(device, videoBackend());
}

cv::VideoCapture createCapture(int device_index) {
  return cv::VideoCapture(device_index, videoBackend());
}

std::optional<cv::VideoCapture> probeCapture(const std::string& device) {
  cv::VideoCapture cap = createCapture(device);
  if (!cap.isOpened()) {
    return std::nullopt;
  }
  cv::Mat frame;
  if (!cap.read(frame) || frame.empty()) {
    cap.release();
    return std::nullopt;
  }
  return cap;
}

std::optional<cv::VideoCapture> probeCapture(int device_index) {
  cv::VideoCapture cap = createCapture(device_index);
  if (!cap.isOpened()) {
    return std::nullopt;
  }
  cv::Mat frame;
  if (!cap.read(frame) || frame.empty()) {
    cap.release();
    return std::nullopt;
  }
  return cap;
}

std::vector<std::string> buildDeviceCandidates(const std::optional<std::string>& device_path,
                                               int device_id,
                                               int max_index) {
  std::vector<std::string> candidates;
  std::set<std::string> seen;

  auto add = [&](const std::string& value) {
    if (seen.insert(value).second) {
      candidates.push_back(value);
    }
  };

  if (device_path.has_value() && !device_path->empty()) {
    add(*device_path);
  }
  add(std::to_string(device_id));

#ifdef __linux__
  for (int i = 0; i < 64; ++i) {
    const fs::path path = fs::path("/dev") / ("video" + std::to_string(i));
    if (fs::exists(path)) {
      add(path.string());
    }
  }
#endif

  for (int index = 0; index < max_index; ++index) {
    add(std::to_string(index));
  }
  return candidates;
}

namespace {

bool isIntegerToken(const std::string& token, int* out) {
  if (token.empty()) {
    return false;
  }
  char* end = nullptr;
  const long value = std::strtol(token.c_str(), &end, 10);
  if (end == token.c_str() || *end != '\0') {
    return false;
  }
  *out = static_cast<int>(value);
  return true;
}

std::optional<cv::VideoCapture> probeToken(const std::string& token) {
  int index = 0;
  if (isIntegerToken(token, &index) && token.find('/') == std::string::npos) {
    return probeCapture(index);
  }
  return probeCapture(token);
}

cv::VideoCapture openToken(const std::string& token) {
  int index = 0;
  if (isIntegerToken(token, &index) && token.find('/') == std::string::npos) {
    return createCapture(index);
  }
  return createCapture(token);
}

}  // namespace

std::optional<DiscoveredCapture> discoverCapture(const std::optional<std::string>& device_path,
                                                 int device_id) {
  const auto previous = cv::utils::logging::getLogLevel();
  cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_SILENT);

  std::optional<DiscoveredCapture> result;
  for (const std::string& device : buildDeviceCandidates(device_path, device_id)) {
    auto cap = probeToken(device);
    if (cap.has_value()) {
      result = DiscoveredCapture{device, std::move(*cap)};
      break;
    }
  }

  cv::utils::logging::setLogLevel(previous);
  return result;
}

std::vector<DeviceInfo> listDevices(int max_index) {
  std::vector<DeviceInfo> devices;
  std::set<std::string> seen;

  auto record = [&](const std::string& device, const cv::Mat& frame) {
    if (!seen.insert(device).second) {
      return;
    }
    std::ostringstream resolution;
    resolution << frame.cols << "x" << frame.rows;
    const bool is_index = device.find('/') == std::string::npos;
    devices.push_back(DeviceInfo{
        is_index ? ("index " + device) : device,
        resolution.str(),
        true,
    });
  };

  std::vector<std::string> candidates;
#ifdef __linux__
  for (int i = 0; i < 64; ++i) {
    const fs::path path = fs::path("/dev") / ("video" + std::to_string(i));
    if (fs::exists(path)) {
      candidates.push_back(path.string());
    }
  }
#endif
  for (int index = 0; index < max_index; ++index) {
    candidates.push_back(std::to_string(index));
  }

  const auto previous = cv::utils::logging::getLogLevel();
  cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_SILENT);

  for (const std::string& device : candidates) {
    cv::VideoCapture cap = openToken(device);
    if (!cap.isOpened()) {
      continue;
    }
    cv::Mat frame;
    if (cap.read(frame) && !frame.empty()) {
      record(device, frame);
    }
    cap.release();
  }

  cv::utils::logging::setLogLevel(previous);
  return devices;
}

}  // namespace chess_robot
