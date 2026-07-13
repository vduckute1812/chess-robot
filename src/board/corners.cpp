#include "chess_robot/board/corners.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include <opencv2/imgproc.hpp>

namespace chess_robot {

cv::Mat orderCorners(const cv::Mat& points) {
  CV_Assert(points.total() == 4);
  cv::Mat pts;
  points.convertTo(pts, CV_32F);
  pts = pts.reshape(2, 4);

  cv::Mat rect(4, 1, CV_32FC2);
  std::vector<float> sums(4);
  std::vector<float> diffs(4);
  for (int i = 0; i < 4; ++i) {
    const cv::Point2f p = pts.at<cv::Point2f>(i);
    sums[i] = p.x + p.y;
    diffs[i] = p.y - p.x;
  }

  const auto min_sum = static_cast<int>(std::min_element(sums.begin(), sums.end()) - sums.begin());
  const auto max_sum = static_cast<int>(std::max_element(sums.begin(), sums.end()) - sums.begin());
  const auto min_diff =
      static_cast<int>(std::min_element(diffs.begin(), diffs.end()) - diffs.begin());
  const auto max_diff =
      static_cast<int>(std::max_element(diffs.begin(), diffs.end()) - diffs.begin());

  rect.at<cv::Point2f>(0) = pts.at<cv::Point2f>(min_sum);   // TL
  rect.at<cv::Point2f>(1) = pts.at<cv::Point2f>(min_diff);  // TR
  rect.at<cv::Point2f>(2) = pts.at<cv::Point2f>(max_sum);   // BR
  rect.at<cv::Point2f>(3) = pts.at<cv::Point2f>(max_diff);  // BL
  return rect;
}

std::optional<cv::Mat> parseManualCorners(const YAML::Node& corners) {
  if (!corners || !corners.IsSequence() || corners.size() != 4) {
    return std::nullopt;
  }

  cv::Mat points(4, 1, CV_32FC2);
  for (std::size_t i = 0; i < 4; ++i) {
    if (!corners[i].IsSequence() || corners[i].size() != 2) {
      throw std::invalid_argument("manual_corners must contain exactly 4 (x, y) points");
    }
    points.at<cv::Point2f>(static_cast<int>(i)) =
        cv::Point2f(corners[i][0].as<float>(), corners[i][1].as<float>());
  }
  return orderCorners(points);
}

cv::Mat outerCornersFromInnerGrid(const cv::Mat& inner_corners, cv::Size pattern_size) {
  const int cols = pattern_size.width;
  const int rows = pattern_size.height;
  cv::Mat flat;
  inner_corners.convertTo(flat, CV_32F);
  flat = flat.reshape(2, rows * cols);

  auto at = [&](int row, int col) -> cv::Point2f {
    return flat.at<cv::Point2f>(row * cols + col);
  };

  const cv::Point2f tl = 3.0f * at(0, 0) - at(0, 1) - at(1, 0);
  const cv::Point2f tr = 3.0f * at(0, cols - 1) - at(0, cols - 2) - at(1, cols - 1);
  const cv::Point2f br =
      3.0f * at(rows - 1, cols - 1) - at(rows - 1, cols - 2) - at(rows - 2, cols - 1);
  const cv::Point2f bl = 3.0f * at(rows - 1, 0) - at(rows - 1, 1) - at(rows - 2, 0);

  cv::Mat outer(4, 1, CV_32FC2);
  outer.at<cv::Point2f>(0) = tl;
  outer.at<cv::Point2f>(1) = tr;
  outer.at<cv::Point2f>(2) = br;
  outer.at<cv::Point2f>(3) = bl;
  return orderCorners(outer);
}

std::vector<std::vector<double>> cornersToConfigList(const cv::Mat& corners) {
  cv::Mat ordered = orderCorners(corners);
  std::vector<std::vector<double>> result;
  result.reserve(4);
  for (int i = 0; i < 4; ++i) {
    const cv::Point2f p = ordered.at<cv::Point2f>(i);
    result.push_back({std::round(p.x * 100.0) / 100.0, std::round(p.y * 100.0) / 100.0});
  }
  return result;
}

bool isPlausibleBoardQuad(const cv::Mat& corners, cv::Size image_size) {
  cv::Mat pts;
  corners.convertTo(pts, CV_32F);
  pts = pts.reshape(2, 4);

  const double area = std::abs(cv::contourArea(pts));
  const double image_area = static_cast<double>(image_size.width) * image_size.height;
  if (area < 0.08 * image_area || area > 0.95 * image_area) {
    return false;
  }
  if (!cv::isContourConvex(pts)) {
    return false;
  }

  cv::Mat ordered = orderCorners(pts);
  std::vector<double> edges;
  edges.reserve(4);
  for (int i = 0; i < 4; ++i) {
    const cv::Point2f a = ordered.at<cv::Point2f>(i);
    const cv::Point2f b = ordered.at<cv::Point2f>((i + 1) % 4);
    edges.push_back(cv::norm(a - b));
  }
  const double min_edge = *std::min_element(edges.begin(), edges.end());
  const double max_edge = *std::max_element(edges.begin(), edges.end());
  if (min_edge < 20.0 || max_edge / min_edge > 3.0) {
    return false;
  }

  const double margin = 0.35 * std::max(image_size.width, image_size.height);
  for (int i = 0; i < 4; ++i) {
    const cv::Point2f p = ordered.at<cv::Point2f>(i);
    if (p.x < -margin || p.x > image_size.width + margin || p.y < -margin ||
        p.y > image_size.height + margin) {
      return false;
    }
  }
  return true;
}

}  // namespace chess_robot
