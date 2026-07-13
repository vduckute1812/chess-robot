#pragma once

#include <filesystem>
#include <optional>
#include <vector>

#include <opencv2/core.hpp>

namespace chess_robot {

namespace fs = std::filesystem;

class BoardDetector {
 public:
  explicit BoardDetector(const std::optional<fs::path>& config_path = std::nullopt);

  int outputSize() const { return output_size_; }
  int gridSize() const { return grid_size_; }
  cv::Size patternSize() const { return pattern_size_; }
  bool hasManualCorners() const { return manual_corners_.has_value(); }

  cv::Mat detectCorners(const cv::Mat& frame, bool use_manual = true);
  fs::path saveCornersToConfig(const std::optional<cv::Mat>& corners = std::nullopt,
                               const std::optional<fs::path>& config_path = std::nullopt);
  fs::path clearManualCorners(const std::optional<fs::path>& config_path = std::nullopt);

  cv::Mat warpBoard(const cv::Mat& frame, const std::optional<cv::Mat>& corners = std::nullopt);
  std::vector<std::vector<cv::Mat>> segmentGrid(const cv::Mat& warped_board) const;
  cv::Mat drawGridOverlay(const cv::Mat& warped_board) const;
  cv::Mat drawCornerOverlay(const cv::Mat& frame,
                            const std::optional<cv::Mat>& corners = std::nullopt);

 private:
  std::optional<cv::Mat> detectAuto(const cv::Mat& frame) const;

  fs::path config_path_;
  int output_size_ = 800;
  int grid_size_ = 8;
  cv::Size pattern_size_{7, 7};
  bool prefer_chessboard_ = true;
  int canny_low_ = 50;
  int canny_high_ = 150;
  int blur_kernel_ = 5;
  double min_contour_area_ = 5000.0;
  std::optional<cv::Mat> manual_corners_;
  std::optional<cv::Mat> last_corners_;
};

}  // namespace chess_robot
