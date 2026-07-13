#include "chess_robot/board/detect_contour.hpp"

#include <algorithm>
#include <vector>

#include <opencv2/imgproc.hpp>

#include "chess_robot/board/corners.hpp"
#include "chess_robot/utils/chessboard.hpp"

namespace chess_robot {

namespace {

const std::vector<double> kApproxEpsilons = {0.02, 0.03, 0.04, 0.05};

std::optional<cv::Mat> approxQuad(const std::vector<cv::Point>& contour) {
  const double peri = cv::arcLength(contour, true);
  if (peri <= 0.0) {
    return std::nullopt;
  }
  for (double scale : kApproxEpsilons) {
    std::vector<cv::Point> approx;
    cv::approxPolyDP(contour, approx, scale * peri, true);
    if (approx.size() == 4 && cv::isContourConvex(approx)) {
      cv::Mat quad(4, 1, CV_32FC2);
      for (int i = 0; i < 4; ++i) {
        quad.at<cv::Point2f>(i) = cv::Point2f(static_cast<float>(approx[i].x),
                                              static_cast<float>(approx[i].y));
      }
      return quad;
    }
  }
  return std::nullopt;
}

}  // namespace

std::optional<cv::Mat> findLargestQuadrilateral(const cv::Mat& frame,
                                                int canny_low,
                                                int canny_high,
                                                int blur_kernel,
                                                double min_contour_area) {
  cv::Mat gray = toGray(frame);
  const int kernel = (blur_kernel % 2 == 1) ? blur_kernel : blur_kernel + 1;
  cv::Mat blurred;
  cv::GaussianBlur(gray, blurred, cv::Size(kernel, kernel), 0);
  cv::Mat edges;
  cv::Canny(blurred, edges, canny_low, canny_high);

  cv::Mat morph_kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5, 5));
  cv::morphologyEx(edges, edges, cv::MORPH_CLOSE, morph_kernel, cv::Point(-1, -1), 2);

  std::vector<std::vector<cv::Point>> contours;
  cv::findContours(edges, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
  std::sort(contours.begin(), contours.end(), [](const auto& a, const auto& b) {
    return cv::contourArea(a) > cv::contourArea(b);
  });

  const std::size_t limit = std::min<std::size_t>(contours.size(), 10);
  for (std::size_t i = 0; i < limit; ++i) {
    if (cv::contourArea(contours[i]) < min_contour_area) {
      continue;
    }

    std::vector<cv::Point> hull;
    cv::convexHull(contours[i], hull);
    for (const auto& candidate : {contours[i], hull}) {
      auto quad = approxQuad(candidate);
      if (!quad.has_value()) {
        continue;
      }
      cv::Mat ordered = orderCorners(*quad);
      if (!isPlausibleBoardQuad(ordered, frame.size())) {
        continue;
      }
      return ordered;
    }
  }
  return std::nullopt;
}

}  // namespace chess_robot
