#pragma once

#include <filesystem>
#include <optional>
#include <tuple>
#include <utility>
#include <vector>

#include <opencv2/core.hpp>

namespace chess_robot {

namespace fs = std::filesystem;

class CameraCalibrator {
 public:
  explicit CameraCalibrator(const std::optional<fs::path>& config_path = std::nullopt);

  int cols() const { return cols_; }
  int rows() const { return rows_; }
  float squareSizeMm() const { return square_size_mm_; }
  int minSamples() const { return min_samples_; }
  int sampleCount() const { return static_cast<int>(object_points_.size()); }
  bool isCalibrated() const {
    return !camera_matrix_.empty() && !dist_coeffs_.empty();
  }

  std::optional<cv::Mat> findCheckerboard(const cv::Mat& frame, bool robust = false) const;
  bool addSample(const cv::Mat& frame);
  std::tuple<cv::Mat, cv::Mat, double> calibrate();
  cv::Mat drawOverlay(const cv::Mat& frame) const;
  fs::path saveToConfig(const std::optional<fs::path>& config_path = std::nullopt) const;
  void resetSamples();

 private:
  fs::path config_path_;
  int cols_ = 9;
  int rows_ = 6;
  float square_size_mm_ = 25.0f;
  int min_samples_ = 15;
  cv::Size pattern_size_;
  cv::Mat object_template_;
  std::vector<cv::Mat> object_points_;
  std::vector<cv::Mat> image_points_;
  std::optional<cv::Size> image_size_;
  cv::Mat camera_matrix_;
  cv::Mat dist_coeffs_;
  std::optional<double> reprojection_error_;
};

}  // namespace chess_robot
