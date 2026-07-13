#include "chess_robot/board/detect_chessboard.hpp"

#include "chess_robot/board/corners.hpp"
#include "chess_robot/utils/chessboard.hpp"

namespace chess_robot {

std::optional<cv::Mat> findChessboardOuterCorners(const cv::Mat& frame, cv::Size pattern_size) {
  auto inner = findInnerCorners(frame, pattern_size, true);
  if (!inner.has_value()) {
    return std::nullopt;
  }
  cv::Mat outer = outerCornersFromInnerGrid(*inner, pattern_size);
  if (!isPlausibleBoardQuad(outer, frame.size())) {
    return std::nullopt;
  }
  return outer;
}

}  // namespace chess_robot
