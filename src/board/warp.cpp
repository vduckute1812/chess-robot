#include "chess_robot/board/warp.hpp"

#include <opencv2/imgproc.hpp>

namespace chess_robot {

cv::Mat warpPerspectiveBoard(const cv::Mat& frame, const cv::Mat& corners, int output_size) {
  cv::Mat src;
  corners.convertTo(src, CV_32F);
  src = src.reshape(2, 4);

  cv::Mat dst(4, 1, CV_32FC2);
  dst.at<cv::Point2f>(0) = cv::Point2f(0.0f, 0.0f);
  dst.at<cv::Point2f>(1) = cv::Point2f(static_cast<float>(output_size - 1), 0.0f);
  dst.at<cv::Point2f>(2) =
      cv::Point2f(static_cast<float>(output_size - 1), static_cast<float>(output_size - 1));
  dst.at<cv::Point2f>(3) = cv::Point2f(0.0f, static_cast<float>(output_size - 1));

  cv::Mat matrix = cv::getPerspectiveTransform(src, dst);
  cv::Mat warped;
  cv::warpPerspective(frame, warped, matrix, cv::Size(output_size, output_size));
  return warped;
}

std::vector<std::vector<cv::Mat>> segmentSquareGrid(const cv::Mat& warped_board, int grid_size) {
  if (warped_board.rows != warped_board.cols) {
    throw std::invalid_argument("Warped board image must be square.");
  }

  const int cell_size = warped_board.rows / grid_size;
  std::vector<std::vector<cv::Mat>> grid;
  grid.reserve(grid_size);
  for (int row = 0; row < grid_size; ++row) {
    std::vector<cv::Mat> row_cells;
    row_cells.reserve(grid_size);
    const int y0 = row * cell_size;
    const int y1 = y0 + cell_size;
    for (int col = 0; col < grid_size; ++col) {
      const int x0 = col * cell_size;
      const int x1 = x0 + cell_size;
      row_cells.push_back(warped_board(cv::Range(y0, y1), cv::Range(x0, x1)).clone());
    }
    grid.push_back(std::move(row_cells));
  }
  return grid;
}

}  // namespace chess_robot
