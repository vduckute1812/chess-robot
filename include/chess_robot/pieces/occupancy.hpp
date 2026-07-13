#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include <opencv2/core.hpp>

#include "chess_robot/pieces/classifier.hpp"

namespace chess_robot {

namespace fs = std::filesystem;

using OccupancyGrid = std::vector<std::vector<SquareState>>;

struct CellDiffScore {
  double mean_abs_diff = 0.0;       // illumination-normalized MAD (primary)
  double raw_mean_abs_diff = 0.0;   // raw gray MAD (debug)
  double changed_ratio = 0.0;
  double live_mean = 0.0;
  double empty_mean = 0.0;
  double empty_stddev = 0.0;
  double adaptive_threshold = 0.0;
};

/**
 * Classical piece occupancy on a warped board (C++).
 *
 * Illumination: mean-normalized absdiff so global light / soft shadows matter less.
 * Empty cells that stay empty can slowly adapt the reference (light drift).
 */
class PieceOccupancyDetector {
 public:
  explicit PieceOccupancyDetector(const std::optional<fs::path>& config_path = std::nullopt);

  int gridSize() const { return grid_size_; }
  bool hasEmptyReference() const { return !empty_board_.empty(); }
  const fs::path& emptyReferencePath() const { return empty_reference_path_; }
  cv::Size emptyReferenceSize() const {
    return empty_board_.empty() ? cv::Size() : empty_board_.size();
  }

  /** Drop in-memory empty reference (forces re-capture with e). */
  void clearEmptyReference();

  void setEmptyReference(const cv::Mat& warped_board);

  fs::path captureEmptyReference(const cv::Mat& warped_board,
                                 const std::optional<fs::path>& path = std::nullopt);

  fs::path captureEmptyReferenceAverage(const std::vector<cv::Mat>& warped_frames,
                                        const std::optional<fs::path>& path = std::nullopt);

  OccupancyGrid detectRaw(const cv::Mat& warped_board) const;

  /** Temporal filter + optional slow empty-reference adaptation. */
  OccupancyGrid detect(const cv::Mat& warped_board);

  void resetTemporalState();

  std::vector<std::vector<CellDiffScore>> scoreGrid(const cv::Mat& warped_board) const;

  cv::Mat drawOverlay(const cv::Mat& warped_board, const OccupancyGrid& grid) const;
  cv::Mat drawDebugOverlay(const cv::Mat& warped_board, const OccupancyGrid& grid) const;

  static int countOccupied(const OccupancyGrid& grid);
  std::string toAscii(const OccupancyGrid& grid, bool near_side_first = false) const;

 private:
  SquareState classifyFromScore(const CellDiffScore& score, bool has_ref) const;
  CellDiffScore scoreCell(const cv::Mat& live_cell,
                          const std::optional<cv::Mat>& empty_cell) const;
  OccupancyGrid applyTemporal(const OccupancyGrid& raw);
  void adaptEmptyReference(const cv::Mat& warped_board, const OccupancyGrid& stable);

  fs::path config_path_;
  int grid_size_ = 8;
  double center_roi_ratio_ = 0.45;
  double occupancy_threshold_ = 8.0;
  double adaptive_stddev_mult_ = 2.5;
  double pixel_diff_threshold_ = 12.0;
  double occupied_pixel_ratio_ = 0.12;
  double texture_threshold_ = 16.0;
  double white_brightness_delta_ = 12.0;
  int blur_kernel_ = 5;
  int confirm_frames_ = 4;
  bool illumination_normalize_ = true;
  double empty_adapt_alpha_ = 0.03;
  int empty_adapt_after_frames_ = 45;
  fs::path empty_reference_path_{"data/empty_board.png"};
  cv::Mat empty_board_;

  std::vector<std::vector<int>> occupied_streak_;
  std::vector<std::vector<int>> empty_streak_;
  OccupancyGrid stable_grid_;
};

}  // namespace chess_robot
