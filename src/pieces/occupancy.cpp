#include "chess_robot/pieces/occupancy.hpp"

#include <algorithm>
#include <cstdio>
#include <sstream>

#include <opencv2/imgproc.hpp>

#include "chess_robot/board/warp.hpp"
#include "chess_robot/config.hpp"
#include "chess_robot/exceptions.hpp"
#include "chess_robot/pieces/reference.hpp"
#include "chess_robot/utils/chessboard.hpp"

namespace chess_robot {

namespace {

cv::Mat prepareRoiGray(const cv::Mat& cell, const cv::Rect& roi, int blur_kernel) {
  cv::Mat gray = toGray(cell(roi));
  const int kernel = (blur_kernel % 2 == 1) ? blur_kernel : blur_kernel + 1;
  if (kernel >= 3) {
    cv::GaussianBlur(gray, gray, cv::Size(kernel, kernel), 0);
  }
  return gray;
}

/** Subtract mean so global brightness / soft shadow bias is removed. */
cv::Mat meanNormalize(const cv::Mat& gray_u8) {
  cv::Mat f32;
  gray_u8.convertTo(f32, CV_32F);
  f32 -= static_cast<float>(cv::mean(f32)[0]);
  return f32;
}

char stateChar(SquareState state) {
  return state == SquareState::Occupied ? 'X' : '.';
}

cv::Scalar stateColor(SquareState state) {
  return state == SquareState::Occupied ? cv::Scalar(0, 255, 255) : cv::Scalar(80, 80, 80);
}

void drawStateMarkers(cv::Mat& overlay, const OccupancyGrid& grid, int grid_size) {
  const int cell = overlay.rows / grid_size;
  for (int row = 0; row < grid_size; ++row) {
    if (row >= static_cast<int>(grid.size()) ||
        static_cast<int>(grid[row].size()) != grid_size) {
      continue;
    }
    for (int col = 0; col < grid_size; ++col) {
      const SquareState state = grid[row][col];
      if (state == SquareState::Empty) {
        continue;
      }
      const cv::Point center(col * cell + cell / 2, row * cell + cell / 2);
      const int radius = std::max(4, cell / 5);
      cv::circle(overlay, center, radius, stateColor(state), -1, cv::LINE_AA);
      cv::circle(overlay, center, radius, cv::Scalar(0, 255, 0), 1, cv::LINE_AA);
      cv::putText(overlay,
                  "X",
                  cv::Point(center.x - 6, center.y + 5),
                  cv::FONT_HERSHEY_SIMPLEX,
                  0.45,
                  cv::Scalar(0, 0, 0),
                  1,
                  cv::LINE_AA);
    }
  }
}

void drawGridLines(cv::Mat& overlay, int grid_size) {
  const int cell = overlay.rows / grid_size;
  for (int i = 1; i < grid_size; ++i) {
    const int pos = i * cell;
    cv::line(overlay, cv::Point(pos, 0), cv::Point(pos, overlay.rows - 1), cv::Scalar(0, 255, 0),
             1);
    cv::line(overlay, cv::Point(0, pos), cv::Point(overlay.cols - 1, pos), cv::Scalar(0, 255, 0),
             1);
  }
}

}  // namespace

PieceOccupancyDetector::PieceOccupancyDetector(const std::optional<fs::path>& config_path)
    : config_path_(resolveConfigPath(config_path)) {
  YAML::Node board = loadSection("board", config_path_);
  grid_size_ = board["grid_size"] ? board["grid_size"].as<int>() : 8;

  YAML::Node pieces = loadSection("pieces", config_path_);
  if (pieces && !pieces.IsNull()) {
    center_roi_ratio_ =
        pieces["center_roi_ratio"] ? pieces["center_roi_ratio"].as<double>() : 0.45;
    occupancy_threshold_ =
        pieces["occupancy_threshold"] ? pieces["occupancy_threshold"].as<double>() : 8.0;
    adaptive_stddev_mult_ =
        pieces["adaptive_stddev_mult"] ? pieces["adaptive_stddev_mult"].as<double>() : 2.5;
    pixel_diff_threshold_ =
        pieces["pixel_diff_threshold"] ? pieces["pixel_diff_threshold"].as<double>() : 12.0;
    occupied_pixel_ratio_ =
        pieces["occupied_pixel_ratio"] ? pieces["occupied_pixel_ratio"].as<double>() : 0.12;
    texture_threshold_ =
        pieces["texture_threshold"] ? pieces["texture_threshold"].as<double>() : 16.0;
    white_brightness_delta_ = pieces["white_brightness_delta"]
                                  ? pieces["white_brightness_delta"].as<double>()
                                  : 12.0;
    blur_kernel_ = pieces["blur_kernel"] ? pieces["blur_kernel"].as<int>() : 5;
    confirm_frames_ = pieces["confirm_frames"] ? pieces["confirm_frames"].as<int>() : 4;
    illumination_normalize_ = !pieces["illumination_normalize"] ||
                              pieces["illumination_normalize"].as<bool>();
    empty_adapt_alpha_ =
        pieces["empty_adapt_alpha"] ? pieces["empty_adapt_alpha"].as<double>() : 0.03;
    empty_adapt_after_frames_ = pieces["empty_adapt_after_frames"]
                                    ? pieces["empty_adapt_after_frames"].as<int>()
                                    : 45;
    if (pieces["empty_reference_path"] && !pieces["empty_reference_path"].IsNull()) {
      empty_reference_path_ = pieces["empty_reference_path"].as<std::string>();
    }
  }

  if (!empty_reference_path_.is_absolute()) {
    empty_reference_path_ = config_path_.parent_path().parent_path() / empty_reference_path_;
  }
  if (auto loaded = loadEmptyReference(empty_reference_path_)) {
    empty_board_ = *loaded;
  }
  resetTemporalState();
}

void PieceOccupancyDetector::resetTemporalState() {
  occupied_streak_.assign(grid_size_, std::vector<int>(grid_size_, 0));
  empty_streak_.assign(grid_size_, std::vector<int>(grid_size_, 0));
  stable_grid_.assign(grid_size_, std::vector<SquareState>(grid_size_, SquareState::Empty));
}

void PieceOccupancyDetector::clearEmptyReference() {
  empty_board_.release();
  resetTemporalState();
}

void PieceOccupancyDetector::setEmptyReference(const cv::Mat& warped_board) {
  if (warped_board.empty()) {
    throw PieceDetectionError("Cannot set empty reference: image is empty.");
  }
  empty_board_ = warped_board.clone();
  resetTemporalState();
}

fs::path PieceOccupancyDetector::captureEmptyReference(
    const cv::Mat& warped_board, const std::optional<fs::path>& path) {
  return captureEmptyReferenceAverage({warped_board}, path);
}

fs::path PieceOccupancyDetector::captureEmptyReferenceAverage(
    const std::vector<cv::Mat>& warped_frames, const std::optional<fs::path>& path) {
  if (warped_frames.empty() || warped_frames.front().empty()) {
    throw PieceDetectionError("Cannot capture empty reference: no warped frames.");
  }

  cv::Mat acc;
  warped_frames.front().convertTo(acc, CV_32FC3);
  int count = 1;
  for (std::size_t i = 1; i < warped_frames.size(); ++i) {
    if (warped_frames[i].empty() || warped_frames[i].size() != warped_frames.front().size()) {
      continue;
    }
    cv::Mat f32;
    warped_frames[i].convertTo(f32, CV_32FC3);
    acc += f32;
    ++count;
  }
  acc *= (1.0 / static_cast<double>(count));
  cv::Mat averaged;
  acc.convertTo(averaged, CV_8UC3);

  const fs::path out = path.value_or(empty_reference_path_);
  const fs::path saved = saveEmptyReference(averaged, out);
  empty_board_ = averaged;
  empty_reference_path_ = saved;
  resetTemporalState();

  YAML::Node config = loadConfig(config_path_);
  if (!config["pieces"]) {
    config["pieces"] = YAML::Node(YAML::NodeType::Map);
  }
  fs::path relative = saved;
  const fs::path project_root = config_path_.parent_path().parent_path();
  std::error_code ec;
  const fs::path rel = fs::relative(saved, project_root, ec);
  if (!ec && !rel.empty() && rel.native().find("..") == std::string::npos) {
    relative = rel;
  }
  config["pieces"]["empty_reference_path"] = relative.generic_string();
  config["pieces"]["method"] = "classical";
  saveConfig(config, config_path_);
  return saved;
}

CellDiffScore PieceOccupancyDetector::scoreCell(
    const cv::Mat& live_cell, const std::optional<cv::Mat>& empty_cell) const {
  CellDiffScore score;
  const cv::Rect roi = centerCellRoi(live_cell.size(), center_roi_ratio_);
  const cv::Mat live_roi = prepareRoiGray(live_cell, roi, blur_kernel_);
  score.live_mean = cv::mean(live_roi)[0];
  score.empty_mean = score.live_mean;

  auto gradientMag = [](const cv::Mat& gray) {
    cv::Mat gx, gy, mag;
    cv::Sobel(gray, gx, CV_32F, 1, 0, 3);
    cv::Sobel(gray, gy, CV_32F, 0, 1, 3);
    cv::magnitude(gx, gy, mag);
    return mag;
  };

  if (!empty_cell.has_value() || empty_cell->empty()) {
    cv::Scalar mean;
    cv::Scalar stddev;
    cv::meanStdDev(live_roi, mean, stddev);
    score.mean_abs_diff = stddev[0];
    score.raw_mean_abs_diff = stddev[0];
    score.empty_stddev = stddev[0];
    score.adaptive_threshold = texture_threshold_;
    score.changed_ratio = score.mean_abs_diff >= texture_threshold_ ? 1.0 : 0.0;
    return score;
  }

  cv::Mat empty_roi = prepareRoiGray(*empty_cell, roi, blur_kernel_);
  if (empty_roi.size() != live_roi.size()) {
    cv::resize(empty_roi, empty_roi, live_roi.size(), 0, 0, cv::INTER_AREA);
  }

  cv::Scalar empty_mean;
  cv::Scalar empty_std;
  cv::meanStdDev(empty_roi, empty_mean, empty_std);
  score.empty_mean = empty_mean[0];
  score.empty_stddev = empty_std[0];

  cv::Mat raw_diff;
  cv::absdiff(live_roi, empty_roi, raw_diff);
  score.raw_mean_abs_diff = cv::mean(raw_diff)[0];

  // Gradient MAD: robust to global brightness; sensitive to piece edges/shape.
  cv::Mat grad_diff;
  cv::absdiff(gradientMag(live_roi), gradientMag(empty_roi), grad_diff);
  const double grad_mad = cv::mean(grad_diff)[0];

  cv::Mat struct_diff;
  if (illumination_normalize_) {
    cv::absdiff(meanNormalize(live_roi), meanNormalize(empty_roi), struct_diff);
  } else {
    struct_diff = raw_diff;
    struct_diff.convertTo(struct_diff, CV_32F);
  }
  const double struct_mad = cv::mean(struct_diff)[0];

  // Primary score blends gradient (light-robust) with structure.
  score.mean_abs_diff = std::max(grad_mad, struct_mad);
  score.adaptive_threshold =
      std::max(occupancy_threshold_, adaptive_stddev_mult_ * (score.empty_stddev * 0.25 + 1.0));

  cv::Mat mask;
  cv::threshold(grad_diff, mask, pixel_diff_threshold_, 255.0, cv::THRESH_BINARY);
  score.changed_ratio = static_cast<double>(cv::countNonZero(mask)) /
                        static_cast<double>(std::max(1, mask.rows * mask.cols));

  // Brightness cue for flat piece-on-flat-square (gradient alone can be weak),
  // gated so a global light shift (uniform raw diff, low gradient change) does not fire.
  const double mean_delta = std::abs(score.live_mean - score.empty_mean);
  if (mean_delta >= white_brightness_delta_ && grad_mad >= occupancy_threshold_ * 0.5) {
    score.changed_ratio = std::max(score.changed_ratio, occupied_pixel_ratio_);
    score.mean_abs_diff = std::max(score.mean_abs_diff, score.adaptive_threshold);
  }
  return score;
}

SquareState PieceOccupancyDetector::classifyFromScore(const CellDiffScore& score,
                                                      bool has_ref) const {
  bool occupied = false;
  if (has_ref) {
    occupied = score.mean_abs_diff >= score.adaptive_threshold &&
               score.changed_ratio >= occupied_pixel_ratio_;
  } else {
    occupied = score.mean_abs_diff >= texture_threshold_;
  }
  return occupied ? SquareState::Occupied : SquareState::Empty;
}

std::vector<std::vector<CellDiffScore>> PieceOccupancyDetector::scoreGrid(
    const cv::Mat& warped_board) const {
  if (warped_board.empty() || warped_board.rows != warped_board.cols) {
    throw PieceDetectionError("Warped board must be a non-empty square for occupancy scoring.");
  }

  const auto live_cells = segmentSquareGrid(warped_board, grid_size_);
  std::optional<std::vector<std::vector<cv::Mat>>> empty_cells;
  if (!empty_board_.empty()) {
    cv::Mat empty = empty_board_;
    if (empty.size() != warped_board.size()) {
      cv::resize(empty_board_, empty, warped_board.size(), 0, 0, cv::INTER_LINEAR);
    }
    empty_cells = segmentSquareGrid(empty, grid_size_);
  }

  std::vector<std::vector<CellDiffScore>> scores(
      grid_size_, std::vector<CellDiffScore>(grid_size_));
  for (int row = 0; row < grid_size_; ++row) {
    for (int col = 0; col < grid_size_; ++col) {
      std::optional<cv::Mat> empty_cell;
      if (empty_cells.has_value()) {
        empty_cell = (*empty_cells)[row][col];
      }
      scores[row][col] = scoreCell(live_cells[row][col], empty_cell);
    }
  }
  return scores;
}

OccupancyGrid PieceOccupancyDetector::detectRaw(const cv::Mat& warped_board) const {
  const auto scores = scoreGrid(warped_board);
  const bool has_ref = !empty_board_.empty();
  OccupancyGrid grid(grid_size_, std::vector<SquareState>(grid_size_, SquareState::Empty));
  for (int row = 0; row < grid_size_; ++row) {
    for (int col = 0; col < grid_size_; ++col) {
      grid[row][col] = classifyFromScore(scores[row][col], has_ref);
    }
  }
  return grid;
}

OccupancyGrid PieceOccupancyDetector::applyTemporal(const OccupancyGrid& raw) {
  const int need = std::max(1, confirm_frames_);
  OccupancyGrid out = stable_grid_;
  for (int row = 0; row < grid_size_; ++row) {
    for (int col = 0; col < grid_size_; ++col) {
      const bool raw_occ = raw[row][col] != SquareState::Empty;
      if (raw_occ) {
        occupied_streak_[row][col] += 1;
        empty_streak_[row][col] = 0;
      } else {
        empty_streak_[row][col] += 1;
        occupied_streak_[row][col] = 0;
      }

      if (occupied_streak_[row][col] >= need) {
        out[row][col] = raw[row][col];
      } else if (empty_streak_[row][col] >= need) {
        out[row][col] = SquareState::Empty;
      }
    }
  }
  stable_grid_ = out;
  return out;
}

void PieceOccupancyDetector::adaptEmptyReference(const cv::Mat& warped_board,
                                                 const OccupancyGrid& stable) {
  if (empty_board_.empty() || empty_adapt_alpha_ <= 0.0) {
    return;
  }
  if (empty_board_.size() != warped_board.size()) {
    cv::resize(empty_board_, empty_board_, warped_board.size(), 0, 0, cv::INTER_LINEAR);
  }

  const int cell = warped_board.rows / grid_size_;
  const double alpha = std::clamp(empty_adapt_alpha_, 0.0, 0.5);
  for (int row = 0; row < grid_size_; ++row) {
    for (int col = 0; col < grid_size_; ++col) {
      if (stable[row][col] != SquareState::Empty) {
        continue;
      }
      if (empty_streak_[row][col] < empty_adapt_after_frames_) {
        continue;
      }
      const cv::Rect rect(col * cell, row * cell, cell, cell);
      cv::Mat live_cell = warped_board(rect);
      cv::Mat empty_cell = empty_board_(rect);
      cv::Mat blended;
      cv::addWeighted(empty_cell, 1.0 - alpha, live_cell, alpha, 0.0, blended);
      blended.copyTo(empty_cell);
    }
  }
}

OccupancyGrid PieceOccupancyDetector::detect(const cv::Mat& warped_board) {
  const OccupancyGrid raw = detectRaw(warped_board);
  const OccupancyGrid stable = applyTemporal(raw);
  adaptEmptyReference(warped_board, stable);
  return stable;
}

int PieceOccupancyDetector::countOccupied(const OccupancyGrid& grid) {
  int count = 0;
  for (const auto& row : grid) {
    for (SquareState state : row) {
      if (state != SquareState::Empty) {
        ++count;
      }
    }
  }
  return count;
}

cv::Mat PieceOccupancyDetector::drawOverlay(const cv::Mat& warped_board,
                                            const OccupancyGrid& grid) const {
  cv::Mat overlay = warped_board.clone();
  drawStateMarkers(overlay, grid, grid_size_);
  drawGridLines(overlay, grid_size_);
  return overlay;
}

cv::Mat PieceOccupancyDetector::drawDebugOverlay(const cv::Mat& warped_board,
                                                 const OccupancyGrid& grid) const {
  cv::Mat overlay = drawOverlay(warped_board, grid);
  const auto scores = scoreGrid(warped_board);
  const int cell = overlay.rows / grid_size_;
  for (int row = 0; row < grid_size_; ++row) {
    for (int col = 0; col < grid_size_; ++col) {
      const CellDiffScore& s = scores[row][col];
      const int x0 = col * cell + 2;
      const int y0 = row * cell + 14;
      char buf[56];
      std::snprintf(buf,
                    sizeof(buf),
                    "n%.0f>%.0f %.0f%%",
                    s.mean_abs_diff,
                    s.adaptive_threshold,
                    100.0 * s.changed_ratio);
      cv::putText(overlay,
                  buf,
                  cv::Point(x0, y0),
                  cv::FONT_HERSHEY_SIMPLEX,
                  0.28,
                  cv::Scalar(0, 255, 255),
                  1,
                  cv::LINE_AA);
    }
  }
  return overlay;
}

std::string PieceOccupancyDetector::toAscii(const OccupancyGrid& grid,
                                            bool near_side_first) const {
  std::ostringstream out;
  const int rows = static_cast<int>(grid.size());
  for (int i = 0; i < rows; ++i) {
    const int row = near_side_first ? (rows - 1 - i) : i;
    for (int col = 0; col < static_cast<int>(grid[row].size()); ++col) {
      out << stateChar(grid[row][col]);
    }
    out << '\n';
  }
  return out.str();
}

}  // namespace chess_robot
