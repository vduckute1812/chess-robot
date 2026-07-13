#pragma once

#include <stdexcept>
#include <vector>

#include <opencv2/core.hpp>

namespace chess_robot {

cv::Mat warpPerspectiveBoard(const cv::Mat& frame, const cv::Mat& corners, int output_size);

std::vector<std::vector<cv::Mat>> segmentSquareGrid(const cv::Mat& warped_board, int grid_size);

}  // namespace chess_robot
