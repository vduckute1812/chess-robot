#pragma once

#include <optional>
#include <vector>

#include <opencv2/core.hpp>
#include <yaml-cpp/yaml.h>

namespace chess_robot {

std::optional<cv::Mat> parseManualCorners(const YAML::Node& corners);

cv::Mat orderCorners(const cv::Mat& points);

cv::Mat outerCornersFromInnerGrid(const cv::Mat& inner_corners, cv::Size pattern_size);

std::vector<std::vector<double>> cornersToConfigList(const cv::Mat& corners);

bool isPlausibleBoardQuad(const cv::Mat& corners, cv::Size image_size);

}  // namespace chess_robot
