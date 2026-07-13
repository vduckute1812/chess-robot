#include "chess_robot/utils/chessboard.hpp"

#include <opencv2/calib3d.hpp>
#include <opencv2/imgproc.hpp>

namespace chess_robot {

namespace {

constexpr int kClassicBase =
    cv::CALIB_CB_ADAPTIVE_THRESH | cv::CALIB_CB_NORMALIZE_IMAGE;
constexpr int kFastFlags = kClassicBase | cv::CALIB_CB_FAST_CHECK;
constexpr int kSlowFlags = kClassicBase;

std::optional<cv::Mat> findClassic(const cv::Mat& gray, cv::Size pattern_size, int flags) {
  cv::Mat corners;
  const bool found = cv::findChessboardCorners(gray, pattern_size, corners, flags);
  if (!found || corners.empty()) {
    return std::nullopt;
  }
  return refineCorners(gray, corners);
}

std::optional<cv::Mat> findSb(const cv::Mat& gray, cv::Size pattern_size) {
  cv::Mat corners;
  int flags = 0;
#ifdef CALIB_CB_ACCURACY
  flags |= cv::CALIB_CB_ACCURACY;
#endif
  const bool found = cv::findChessboardCornersSB(gray, pattern_size, corners, flags);
  if (!found || corners.empty()) {
    return std::nullopt;
  }
  return corners;
}

}  // namespace

cv::Mat toGray(const cv::Mat& frame) {
  if (frame.channels() == 1) {
    return frame;
  }
  cv::Mat gray;
  cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
  return gray;
}

std::vector<cv::Mat> prepareGrayVariants(const cv::Mat& frame) {
  cv::Mat gray = toGray(frame);
  cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE(2.0, cv::Size(8, 8));
  cv::Mat equalized;
  clahe->apply(gray, equalized);
  cv::Mat blurred;
  cv::GaussianBlur(gray, blurred, cv::Size(5, 5), 0);
  cv::Mat equalized_blurred;
  clahe->apply(blurred, equalized_blurred);
  return {gray, equalized, equalized_blurred};
}

cv::Mat refineCorners(const cv::Mat& gray, const cv::Mat& corners) {
  cv::Mat refined = corners.clone();
  cv::cornerSubPix(
      gray,
      refined,
      cv::Size(11, 11),
      cv::Size(-1, -1),
      cv::TermCriteria(cv::TermCriteria::EPS + cv::TermCriteria::MAX_ITER, 30, 0.001));
  return refined;
}

std::optional<cv::Mat> findInnerCorners(const cv::Mat& frame,
                                        cv::Size pattern_size,
                                        bool robust) {
  if (pattern_size.width < 2 || pattern_size.height < 2) {
    return std::nullopt;
  }

  if (!robust) {
    cv::Mat gray = toGray(frame);
    if (auto corners = findClassic(gray, pattern_size, kFastFlags)) {
      return corners;
    }
    return findClassic(gray, pattern_size, kSlowFlags);
  }

  const std::vector<cv::Mat> variants = prepareGrayVariants(frame);
  for (const cv::Mat& gray : variants) {
    if (auto corners = findClassic(gray, pattern_size, kFastFlags)) {
      return corners;
    }
  }
  for (const cv::Mat& gray : variants) {
    if (auto corners = findClassic(gray, pattern_size, kSlowFlags)) {
      return corners;
    }
    if (auto corners = findSb(gray, pattern_size)) {
      return corners;
    }
  }
  return std::nullopt;
}

}  // namespace chess_robot
