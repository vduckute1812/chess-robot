#include "chess_robot/calibration/calibrator.hpp"

#include <sstream>

#include <opencv2/calib3d.hpp>
#include <opencv2/imgproc.hpp>

#include "chess_robot/calibration/checkerboard.hpp"
#include "chess_robot/calibration/matrices.hpp"
#include "chess_robot/config.hpp"
#include "chess_robot/exceptions.hpp"

namespace chess_robot {

CameraCalibrator::CameraCalibrator(const std::optional<fs::path>& config_path)
    : config_path_(resolveConfigPath(config_path)) {
  YAML::Node cal = loadSection("calibration", config_path_);
  YAML::Node board = cal["checkerboard"] ? cal["checkerboard"] : YAML::Node(YAML::NodeType::Map);
  cols_ = board["cols"] ? board["cols"].as<int>() : 9;
  rows_ = board["rows"] ? board["rows"].as<int>() : 6;
  square_size_mm_ = board["square_size_mm"] ? board["square_size_mm"].as<float>() : 25.0f;
  min_samples_ = cal["min_samples"] ? cal["min_samples"].as<int>() : 15;
  pattern_size_ = cv::Size(cols_, rows_);
  object_template_ = buildObjectPoints(cols_, rows_, square_size_mm_);
}

std::optional<cv::Mat> CameraCalibrator::findCheckerboard(const cv::Mat& frame,
                                                          bool robust) const {
  return findCheckerboardCorners(frame, pattern_size_, robust);
}

bool CameraCalibrator::addSample(const cv::Mat& frame) {
  auto corners = findCheckerboard(frame, true);
  if (!corners.has_value()) {
    return false;
  }

  const cv::Size size(frame.cols, frame.rows);
  if (!image_size_.has_value()) {
    image_size_ = size;
  } else if (*image_size_ != size) {
    std::ostringstream message;
    message << "All calibration frames must have the same resolution. Expected "
            << image_size_->width << "x" << image_size_->height << ", got " << size.width
            << "x" << size.height << ".";
    throw CalibrationError(message.str());
  }

  object_points_.push_back(object_template_.clone());
  image_points_.push_back(corners->clone());
  return true;
}

std::tuple<cv::Mat, cv::Mat, double> CameraCalibrator::calibrate() {
  if (sampleCount() < min_samples_) {
    std::ostringstream message;
    message << "Need at least " << min_samples_ << " valid samples, have " << sampleCount()
            << ".";
    throw CalibrationError(message.str());
  }
  if (!image_size_.has_value()) {
    throw CalibrationError("No calibration samples collected.");
  }

  std::vector<cv::Mat> rvecs;
  std::vector<cv::Mat> tvecs;
  const double error = cv::calibrateCamera(
      object_points_, image_points_, *image_size_, camera_matrix_, dist_coeffs_, rvecs, tvecs);
  reprojection_error_ = error;
  return {camera_matrix_.clone(), dist_coeffs_.clone(), error};
}

cv::Mat CameraCalibrator::drawOverlay(const cv::Mat& frame) const {
  cv::Mat overlay;
  auto corners = findCheckerboard(frame, false);
  if (corners.has_value()) {
    overlay = drawCheckerboardOverlay(frame, pattern_size_, *corners);
  } else {
    overlay = frame.clone();
  }

  const bool ready = sampleCount() >= min_samples_;
  std::ostringstream status;
  status << "Samples: " << sampleCount() << "/" << min_samples_ << "  "
         << (ready ? "READY" : "collect more");
  cv::putText(overlay,
              status.str(),
              cv::Point(20, 40),
              cv::FONT_HERSHEY_SIMPLEX,
              0.8,
              cv::Scalar(0, 255, 0),
              2,
              cv::LINE_AA);
  return overlay;
}

fs::path CameraCalibrator::saveToConfig(const std::optional<fs::path>& config_path) const {
  if (!isCalibrated() || !image_size_.has_value() || !reprojection_error_.has_value()) {
    throw CalibrationError("Calibrate before saving results.");
  }
  return saveCalibrationMatrices(camera_matrix_,
                                 dist_coeffs_,
                                 *reprojection_error_,
                                 *image_size_,
                                 config_path.value_or(config_path_));
}

void CameraCalibrator::resetSamples() {
  object_points_.clear();
  image_points_.clear();
  image_size_.reset();
}

}  // namespace chess_robot
