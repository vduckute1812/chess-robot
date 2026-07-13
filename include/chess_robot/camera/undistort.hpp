#pragma once

#include <opencv2/core.hpp>

namespace chess_robot {

class FrameUndistorter {
 public:
  FrameUndistorter(cv::Mat camera_matrix,
                   cv::Mat dist_coeffs,
                   cv::Size calibration_size = {});

  cv::Mat apply(const cv::Mat& frame);
  void reset();

 private:
  void ensureMaps(cv::Size size);
  cv::Mat scaledCameraMatrix(cv::Size frame_size) const;

  cv::Mat camera_matrix_;
  cv::Mat dist_coeffs_;
  cv::Size calibration_size_;
  cv::Mat map1_;
  cv::Mat map2_;
  bool has_maps_ = false;
};

}  // namespace chess_robot
