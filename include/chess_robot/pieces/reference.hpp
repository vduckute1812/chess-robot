#pragma once

#include <filesystem>
#include <optional>

#include <opencv2/core.hpp>

namespace chess_robot {

namespace fs = std::filesystem;

/** Load empty-board reference image from disk (BGR or gray). */
std::optional<cv::Mat> loadEmptyReference(const fs::path& path);

/** Save warped empty board as the occupancy reference image. */
fs::path saveEmptyReference(const cv::Mat& warped_board, const fs::path& path);

/** Center crop of a cell (ratio in (0, 1]). */
cv::Rect centerCellRoi(cv::Size cell_size, double center_roi_ratio);

}  // namespace chess_robot
