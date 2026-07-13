#pragma once

#include <optional>
#include <utility>
#include <vector>

#include <opencv2/core.hpp>

namespace chess_robot {

std::optional<cv::Mat> findCheckerboardCorners(const cv::Mat& frame,
                                               cv::Size pattern_size,
                                               bool robust = true);

cv::Mat drawCheckerboardOverlay(const cv::Mat& frame,
                                cv::Size pattern_size,
                                const cv::Mat& corners);

cv::Mat buildObjectPoints(int cols, int rows, float square_size_mm);

}  // namespace chess_robot
