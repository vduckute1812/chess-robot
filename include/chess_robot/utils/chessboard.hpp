#pragma once

#include <optional>
#include <vector>

#include <opencv2/core.hpp>

namespace chess_robot {

cv::Mat toGray(const cv::Mat& frame);

std::vector<cv::Mat> prepareGrayVariants(const cv::Mat& frame);

cv::Mat refineCorners(const cv::Mat& gray, const cv::Mat& corners);

/** Find OpenCV inner corners for pattern_size (cols, rows). */
std::optional<cv::Mat> findInnerCorners(const cv::Mat& frame,
                                        cv::Size pattern_size,
                                        bool robust = false);

}  // namespace chess_robot
