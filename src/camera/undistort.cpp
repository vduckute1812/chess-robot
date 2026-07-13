#include "chess_robot/camera/undistort.hpp"

#include <opencv2/calib3d.hpp>
#include <opencv2/imgproc.hpp>

namespace chess_robot {

FrameUndistorter::FrameUndistorter(cv::Mat camera_matrix,
                                   cv::Mat dist_coeffs,
                                   cv::Size calibration_size)
    : camera_matrix_(std::move(camera_matrix)),
      dist_coeffs_(std::move(dist_coeffs)),
      calibration_size_(calibration_size) {}

cv::Mat FrameUndistorter::scaledCameraMatrix(cv::Size frame_size) const {
  cv::Mat scaled = camera_matrix_.clone();
  if (calibration_size_.width <= 0 || calibration_size_.height <= 0) {
    return scaled;
  }
  if (calibration_size_ == frame_size) {
    return scaled;
  }

  const double sx = static_cast<double>(frame_size.width) / calibration_size_.width;
  const double sy = static_cast<double>(frame_size.height) / calibration_size_.height;
  scaled.at<double>(0, 0) *= sx;  // fx
  scaled.at<double>(0, 2) *= sx;  // cx
  scaled.at<double>(1, 1) *= sy;  // fy
  scaled.at<double>(1, 2) *= sy;  // cy
  return scaled;
}

cv::Mat FrameUndistorter::apply(const cv::Mat& frame) {
  ensureMaps(frame.size());
  cv::Mat undistorted;
  cv::remap(frame, undistorted, map1_, map2_, cv::INTER_LINEAR);
  return undistorted;
}

void FrameUndistorter::ensureMaps(cv::Size size) {
  if (has_maps_ && map1_.rows == size.height && map1_.cols == size.width) {
    return;
  }

  const cv::Mat camera_matrix = scaledCameraMatrix(size);
  cv::Mat new_matrix =
      cv::getOptimalNewCameraMatrix(camera_matrix, dist_coeffs_, size, /*alpha=*/0.0, size);
  cv::initUndistortRectifyMap(
      camera_matrix, dist_coeffs_, cv::Mat(), new_matrix, size, CV_16SC2, map1_, map2_);
  has_maps_ = true;
}

void FrameUndistorter::reset() {
  map1_.release();
  map2_.release();
  has_maps_ = false;
}

}  // namespace chess_robot
