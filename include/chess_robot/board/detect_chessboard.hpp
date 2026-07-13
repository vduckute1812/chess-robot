#pragma once

#include <optional>

#include <opencv2/core.hpp>

namespace chess_robot {

std::optional<cv::Mat> findChessboardOuterCorners(const cv::Mat& frame, cv::Size pattern_size);

}  // namespace chess_robot
