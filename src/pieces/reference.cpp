#include "chess_robot/pieces/reference.hpp"

#include <algorithm>

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include "chess_robot/exceptions.hpp"

namespace chess_robot {

std::optional<cv::Mat> loadEmptyReference(const fs::path& path) {
  if (path.empty() || !fs::exists(path)) {
    return std::nullopt;
  }
  cv::Mat image = cv::imread(path.string(), cv::IMREAD_COLOR);
  if (image.empty()) {
    return std::nullopt;
  }
  return image;
}

fs::path saveEmptyReference(const cv::Mat& warped_board, const fs::path& path) {
  if (warped_board.empty()) {
    throw PieceDetectionError("Cannot save empty reference: warped board is empty.");
  }
  if (path.has_parent_path()) {
    fs::create_directories(path.parent_path());
  }
  if (!cv::imwrite(path.string(), warped_board)) {
    throw PieceDetectionError("Failed to write empty reference image: " + path.string());
  }
  return path;
}

cv::Rect centerCellRoi(cv::Size cell_size, double center_roi_ratio) {
  const double ratio = std::clamp(center_roi_ratio, 0.2, 1.0);
  const int width = std::max(1, static_cast<int>(cell_size.width * ratio));
  const int height = std::max(1, static_cast<int>(cell_size.height * ratio));
  const int x0 = (cell_size.width - width) / 2;
  const int y0 = (cell_size.height - height) / 2;
  return {x0, y0, width, height};
}

}  // namespace chess_robot
