#pragma once

#include <optional>

#include <opencv2/core.hpp>

namespace chess_robot {

std::optional<cv::Mat> findLargestQuadrilateral(const cv::Mat& frame,
                                                int canny_low,
                                                int canny_high,
                                                int blur_kernel,
                                                double min_contour_area);

}  // namespace chess_robot
