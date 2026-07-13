#include "chess_robot/calibration/matrices.hpp"

#include "chess_robot/config.hpp"

namespace chess_robot {

std::optional<CalibrationMatrices> loadCalibrationMatrices(
    const std::optional<fs::path>& config_path) {
  YAML::Node cal = loadSection("calibration", config_path);
  if (!cal["enabled"] || !cal["enabled"].as<bool>()) {
    return std::nullopt;
  }
  if (!cal["camera_matrix"] || !cal["dist_coeffs"]) {
    return std::nullopt;
  }

  cv::Mat camera_matrix(3, 3, CV_64F);
  for (int r = 0; r < 3; ++r) {
    for (int c = 0; c < 3; ++c) {
      camera_matrix.at<double>(r, c) = cal["camera_matrix"][r][c].as<double>();
    }
  }

  const YAML::Node& dist_node = cal["dist_coeffs"];
  cv::Mat dist_coeffs(static_cast<int>(dist_node.size()), 1, CV_64F);
  for (std::size_t i = 0; i < dist_node.size(); ++i) {
    dist_coeffs.at<double>(static_cast<int>(i), 0) = dist_node[i].as<double>();
  }

  cv::Size image_size;
  if (cal["image_width"] && cal["image_height"]) {
    image_size = cv::Size(cal["image_width"].as<int>(), cal["image_height"].as<int>());
  }

  return CalibrationMatrices{camera_matrix, dist_coeffs, image_size};
}

fs::path saveCalibrationMatrices(const cv::Mat& camera_matrix,
                                 const cv::Mat& dist_coeffs,
                                 double reprojection_error,
                                 cv::Size image_size,
                                 const std::optional<fs::path>& config_path) {
  const fs::path path = resolveConfigPath(config_path);
  YAML::Node config = loadConfig(path);
  if (!config["calibration"]) {
    config["calibration"] = YAML::Node(YAML::NodeType::Map);
  }

  YAML::Node matrix_node(YAML::NodeType::Sequence);
  for (int r = 0; r < 3; ++r) {
    YAML::Node row(YAML::NodeType::Sequence);
    for (int c = 0; c < 3; ++c) {
      row.push_back(camera_matrix.at<double>(r, c));
    }
    matrix_node.push_back(row);
  }

  YAML::Node dist_node(YAML::NodeType::Sequence);
  cv::Mat flat = dist_coeffs.reshape(1, static_cast<int>(dist_coeffs.total()));
  for (int i = 0; i < flat.rows; ++i) {
    dist_node.push_back(flat.at<double>(i, 0));
  }

  config["calibration"]["enabled"] = true;
  config["calibration"]["camera_matrix"] = matrix_node;
  config["calibration"]["dist_coeffs"] = dist_node;
  config["calibration"]["reprojection_error"] = reprojection_error;
  config["calibration"]["image_width"] = image_size.width;
  config["calibration"]["image_height"] = image_size.height;

  return saveConfig(config, path);
}

}  // namespace chess_robot
