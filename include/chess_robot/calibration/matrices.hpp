#pragma once

#include <filesystem>
#include <optional>

#include <opencv2/core.hpp>

namespace chess_robot {

namespace fs = std::filesystem;

struct CalibrationMatrices {
  cv::Mat camera_matrix;
  cv::Mat dist_coeffs;
  /** Resolution used when calibration was computed; (0,0) if unknown. */
  cv::Size image_size;
};

std::optional<CalibrationMatrices> loadCalibrationMatrices(
    const std::optional<fs::path>& config_path = std::nullopt);

fs::path saveCalibrationMatrices(const cv::Mat& camera_matrix,
                                 const cv::Mat& dist_coeffs,
                                 double reprojection_error,
                                 cv::Size image_size,
                                 const std::optional<fs::path>& config_path = std::nullopt);

}  // namespace chess_robot
