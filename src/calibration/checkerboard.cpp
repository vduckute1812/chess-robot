#include "chess_robot/calibration/checkerboard.hpp"

#include "chess_robot/utils/chessboard.hpp"

#include <opencv2/calib3d.hpp>

namespace chess_robot {

std::optional<cv::Mat> findCheckerboardCorners(const cv::Mat& frame,
                                               cv::Size pattern_size,
                                               bool robust) {
  return findInnerCorners(frame, pattern_size, robust);
}

cv::Mat drawCheckerboardOverlay(const cv::Mat& frame,
                                cv::Size pattern_size,
                                const cv::Mat& corners) {
  cv::Mat overlay = frame.clone();
  cv::drawChessboardCorners(overlay, pattern_size, corners, true);
  return overlay;
}

cv::Mat buildObjectPoints(int cols, int rows, float square_size_mm) {
  cv::Mat grid(rows * cols, 1, CV_32FC3);
  int index = 0;
  for (int row = 0; row < rows; ++row) {
    for (int col = 0; col < cols; ++col) {
      grid.at<cv::Vec3f>(index++) = cv::Vec3f(col * square_size_mm, row * square_size_mm, 0.0f);
    }
  }
  return grid;
}

}  // namespace chess_robot
