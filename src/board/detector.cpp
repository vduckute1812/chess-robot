#include "chess_robot/board/detector.hpp"

#include <algorithm>
#include <string>

#include <opencv2/imgproc.hpp>

#include "chess_robot/board/corners.hpp"
#include "chess_robot/board/detect_chessboard.hpp"
#include "chess_robot/board/detect_contour.hpp"
#include "chess_robot/board/warp.hpp"
#include "chess_robot/config.hpp"
#include "chess_robot/exceptions.hpp"

namespace chess_robot {

BoardDetector::BoardDetector(const std::optional<fs::path>& config_path)
    : config_path_(resolveConfigPath(config_path)) {
  YAML::Node board = loadSection("board", config_path_);
  YAML::Node detection = loadSection("detection", config_path_);

  output_size_ = board["output_size"] ? board["output_size"].as<int>() : 800;
  grid_size_ = board["grid_size"] ? board["grid_size"].as<int>() : 8;
  manual_corners_ = parseManualCorners(board["manual_corners"]);

  if (detection["pattern_size"] && detection["pattern_size"].IsSequence() &&
      detection["pattern_size"].size() == 2) {
    pattern_size_ = cv::Size(detection["pattern_size"][0].as<int>(),
                             detection["pattern_size"][1].as<int>());
  } else {
    const int inner = std::max(grid_size_ - 1, 2);
    pattern_size_ = cv::Size(inner, inner);
  }

  prefer_chessboard_ =
      detection["prefer_chessboard"] ? detection["prefer_chessboard"].as<bool>() : true;
  canny_low_ = detection["canny_low"] ? detection["canny_low"].as<int>() : 50;
  canny_high_ = detection["canny_high"] ? detection["canny_high"].as<int>() : 150;
  blur_kernel_ = detection["blur_kernel"] ? detection["blur_kernel"].as<int>() : 5;
  min_contour_area_ =
      detection["min_contour_area"] ? detection["min_contour_area"].as<double>() : 5000.0;
}

cv::Mat BoardDetector::detectCorners(const cv::Mat& frame, bool use_manual) {
  cv::Mat corners;
  if (use_manual && manual_corners_.has_value()) {
    corners = manual_corners_->clone();
  } else {
    auto detected = detectAuto(frame);
    if (!detected.has_value()) {
      if (last_corners_.has_value()) {
        return last_corners_->clone();
      }
      throw BoardDetectionError(
          "Could not detect chessboard corners. "
          "Try improving lighting, centering the board, or setting manual_corners.");
    }
    corners = *detected;
  }
  last_corners_ = corners.clone();
  return corners;
}

std::optional<cv::Mat> BoardDetector::detectAuto(const cv::Mat& frame) const {
  if (prefer_chessboard_) {
    auto corners = findChessboardOuterCorners(frame, pattern_size_);
    if (corners.has_value()) {
      return corners;
    }
  }
  return findLargestQuadrilateral(
      frame, canny_low_, canny_high_, blur_kernel_, min_contour_area_);
}

fs::path BoardDetector::saveCornersToConfig(const std::optional<cv::Mat>& corners,
                                            const std::optional<fs::path>& config_path) {
  cv::Mat to_save;
  if (corners.has_value()) {
    to_save = *corners;
  } else if (last_corners_.has_value()) {
    to_save = *last_corners_;
  } else {
    throw BoardDetectionError(
        "No corners to save. Detect the board first, then call saveCornersToConfig.");
  }

  const fs::path path = resolveConfigPath(config_path.value_or(config_path_));
  YAML::Node config = loadConfig(path);
  if (!config["board"]) {
    config["board"] = YAML::Node(YAML::NodeType::Map);
  }

  YAML::Node manual(YAML::NodeType::Sequence);
  for (const auto& point : cornersToConfigList(to_save)) {
    YAML::Node xy(YAML::NodeType::Sequence);
    xy.push_back(point[0]);
    xy.push_back(point[1]);
    manual.push_back(xy);
  }
  config["board"]["manual_corners"] = manual;

  const fs::path saved = saveConfig(config, path);
  manual_corners_ = parseManualCorners(config["board"]["manual_corners"]);
  if (manual_corners_.has_value()) {
    last_corners_ = manual_corners_->clone();
  }
  config_path_ = path;
  return saved;
}

fs::path BoardDetector::clearManualCorners(const std::optional<fs::path>& config_path) {
  const fs::path path = resolveConfigPath(config_path.value_or(config_path_));
  YAML::Node config = loadConfig(path);
  if (!config["board"]) {
    config["board"] = YAML::Node(YAML::NodeType::Map);
  }
  config["board"]["manual_corners"] = YAML::Node(YAML::NodeType::Sequence);
  const fs::path saved = saveConfig(config, path);
  manual_corners_.reset();
  config_path_ = path;
  return saved;
}

cv::Mat BoardDetector::warpBoard(const cv::Mat& frame, const std::optional<cv::Mat>& corners) {
  cv::Mat used = corners.has_value() ? *corners : detectCorners(frame);
  return warpPerspectiveBoard(frame, used, output_size_);
}

std::vector<std::vector<cv::Mat>> BoardDetector::segmentGrid(const cv::Mat& warped_board) const {
  try {
    return segmentSquareGrid(warped_board, grid_size_);
  } catch (const std::invalid_argument& exc) {
    throw BoardDetectionError(exc.what());
  }
}

cv::Mat BoardDetector::drawGridOverlay(const cv::Mat& warped_board) const {
  cv::Mat overlay = warped_board.clone();
  const int size = warped_board.rows;
  const int step = size / grid_size_;
  for (int i = 1; i < grid_size_; ++i) {
    const int pos = i * step;
    cv::line(overlay, cv::Point(pos, 0), cv::Point(pos, size - 1), cv::Scalar(0, 255, 0), 1);
    cv::line(overlay, cv::Point(0, pos), cv::Point(size - 1, pos), cv::Scalar(0, 255, 0), 1);
  }
  return overlay;
}

cv::Mat BoardDetector::drawCornerOverlay(const cv::Mat& frame,
                                         const std::optional<cv::Mat>& corners) {
  cv::Mat overlay = frame.clone();
  cv::Mat used;
  if (corners.has_value()) {
    used = *corners;
  } else {
    try {
      used = detectCorners(frame);
    } catch (const BoardDetectionError&) {
      return overlay;
    }
  }

  std::vector<cv::Point> pts;
  pts.reserve(4);
  cv::Mat ordered;
  used.convertTo(ordered, CV_32F);
  ordered = ordered.reshape(2, 4);
  for (int i = 0; i < 4; ++i) {
    const cv::Point2f p = ordered.at<cv::Point2f>(i);
    pts.emplace_back(cvRound(p.x), cvRound(p.y));
  }
  const std::vector<std::vector<cv::Point>> polylines = {pts};
  cv::polylines(overlay, polylines, true, cv::Scalar(0, 255, 0), 2);
  for (int i = 0; i < 4; ++i) {
    cv::circle(overlay, pts[i], 6, cv::Scalar(0, 0, 255), -1);
    cv::putText(overlay,
                std::to_string(i),
                cv::Point(pts[i].x + 8, pts[i].y - 8),
                cv::FONT_HERSHEY_SIMPLEX,
                0.5,
                cv::Scalar(255, 255, 255),
                1,
                cv::LINE_AA);
  }
  return overlay;
}

}  // namespace chess_robot
