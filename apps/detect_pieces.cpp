#include <iostream>
#include <vector>

#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include "chess_robot/board/corners.hpp"
#include "chess_robot/board/detector.hpp"
#include "chess_robot/exceptions.hpp"
#include "chess_robot/pieces/occupancy.hpp"
#include "common.hpp"

namespace {

constexpr const char* kWindowCamera = "Chess Robot - Detect Pieces (camera)";
constexpr const char* kWindowPieces = "Chess Robot - Occupancy";
constexpr int kEmptyCaptureFrames = 12;
/** Minimum board width in the camera frame before empty-capture / detect are allowed. */
constexpr double kMinBoardWidthPx = 360.0;

double boardWidthPx(const cv::Mat& corners) {
  cv::Mat ordered = chess_robot::orderCorners(corners);
  const cv::Point2f tl = ordered.at<cv::Point2f>(0);
  const cv::Point2f tr = ordered.at<cv::Point2f>(1);
  const cv::Point2f br = ordered.at<cv::Point2f>(2);
  const cv::Point2f bl = ordered.at<cv::Point2f>(3);
  return 0.5 * (cv::norm(tr - tl) + cv::norm(br - bl));
}

void printControls(const chess_robot::ChessCamera& camera,
                   const chess_robot::BoardDetector& board,
                   const chess_robot::PieceOccupancyDetector& pieces) {
  std::cout << "Piece occupancy (gradient + light-norm + empty adapt).\n";
  std::cout << "  Warp size: " << board.outputSize() << "px\n";
  std::cout << "  ASCII map: line1 = NEAR side of board (bottom of warp).\n";
  if (camera.isCalibrated()) {
    std::cout << "  Lens calibration: enabled\n";
  }
  if (board.hasManualCorners()) {
    std::cout << "  Board corners: locked\n";
  } else {
    std::cout << "  Board corners: live auto-detect\n";
  }
  if (pieces.hasEmptyReference()) {
    std::cout << "  Empty reference: " << pieces.emptyReferencePath() << " ("
              << pieces.emptyReferenceSize().width << "px)\n";
  } else {
    std::cout << "  Empty reference: none — CLEAR the board, then press e\n";
  }
  std::cout << "  REQUIRED: board >= " << static_cast<int>(kMinBoardWidthPx)
            << "px wide in camera (move closer).\n"
            << "  e - capture empty reference (blocked if board too small / pieces on board)\n"
            << "  m - debug  p - ASCII  u - undistort  q - quit\n";
}

}  // namespace

int main(int argc, char** argv) {
  using namespace chess_robot;
  using namespace chess_robot::apps;

  const CliOptions options =
      parseConfigArgs(argc, argv, "Detect chess piece occupancy on the warped board");

  BoardDetector board(options.config);
  PieceOccupancyDetector pieces(options.config);
  auto camera = openCameraOrExit(options.config);
  if (!camera.has_value()) {
    return 1;
  }

  // Stale empty at wrong warp size (e.g. 800 vs 1600) cannot be trusted.
  if (pieces.hasEmptyReference() &&
      pieces.emptyReferenceSize().width != board.outputSize()) {
    std::cout << "Ignoring stale empty reference " << pieces.emptyReferenceSize().width
              << "px (warp is " << board.outputSize() << "px). Re-capture with e.\n";
    pieces.clearEmptyReference();
  }

  printControls(*camera, board, pieces);

  bool show_undistorted = camera->isCalibrated();
  bool show_debug = false;
  bool capturing_empty = false;
  std::vector<cv::Mat> empty_frames;
  empty_frames.reserve(kEmptyCaptureFrames);

  while (true) {
    cv::Mat frame;
    try {
      frame = camera->read(show_undistorted);
    } catch (const CameraError& exc) {
      std::cerr << "Frame read error: " << exc.what() << "\n";
      break;
    }

    std::optional<cv::Mat> corners;
    try {
      corners = board.detectCorners(frame);
    } catch (const BoardDetectionError&) {
      corners.reset();
    }

    OccupancyGrid grid;
    bool have_grid = false;
    int occupied = 0;
    double board_w = corners.has_value() ? boardWidthPx(*corners) : 0.0;
    const bool board_large_enough = board_w >= kMinBoardWidthPx;

    if (capturing_empty) {
      if (!corners.has_value() || !board_large_enough) {
        std::cout << "Empty capture aborted (need stable board >= "
                  << static_cast<int>(kMinBoardWidthPx) << "px).\n";
        capturing_empty = false;
        empty_frames.clear();
      } else {
        empty_frames.push_back(board.warpBoard(frame, corners));
        if (static_cast<int>(empty_frames.size()) >= kEmptyCaptureFrames) {
          try {
            const fs::path saved = pieces.captureEmptyReferenceAverage(empty_frames);
            const OccupancyGrid self = pieces.detectRaw(empty_frames.back());
            const int self_occ = PieceOccupancyDetector::countOccupied(self);
            std::cout << "Saved empty reference (" << empty_frames.back().cols << "px) to "
                      << saved << "\n";
            if (self_occ > 0) {
              std::cout << "WARNING: self-check " << self_occ
                        << "/64 — board may not have been empty. Clear pieces, press e again.\n";
              pieces.clearEmptyReference();
            } else {
              std::cout << "Self-check OK (0/64). Now place pieces.\n";
            }
          } catch (const PieceDetectionError& exc) {
            std::cout << "Could not save empty reference: " << exc.what() << "\n";
          }
          capturing_empty = false;
          empty_frames.clear();
        }
      }
    }

    cv::Mat camera_view = board.drawCornerOverlay(frame, corners);
    std::string hud;
    if (capturing_empty) {
      hud = "capturing empty " + std::to_string(empty_frames.size()) + "/" +
            std::to_string(kEmptyCaptureFrames);
    } else if (!corners.has_value()) {
      hud = "searching board...";
    } else if (!board_large_enough) {
      hud = "TOO FAR: board " + std::to_string(static_cast<int>(board_w)) + "px — move CLOSER";
    } else if (!pieces.hasEmptyReference()) {
      hud = "board OK | CLEAR board then press e";
    } else {
      hud = "ready | light-adapt on";
    }
    if (board_w > 0.0) {
      hud += " | " + std::to_string(static_cast<int>(board_w)) + "px";
    }

    const bool can_detect =
        corners.has_value() && board_large_enough && pieces.hasEmptyReference() && !capturing_empty;

    if (can_detect) {
      try {
        const cv::Mat warped = board.warpBoard(frame, corners);
        grid = pieces.detect(warped);
        have_grid = true;
        occupied = PieceOccupancyDetector::countOccupied(grid);
        hud += " | occ " + std::to_string(occupied) + "/64";
        cv::Mat piece_view =
            show_debug ? pieces.drawDebugOverlay(warped, grid) : pieces.drawOverlay(warped, grid);
        if (piece_view.cols > 1000) {
          cv::Mat shown;
          cv::resize(piece_view, shown, cv::Size(), 0.5, 0.5, cv::INTER_AREA);
          cv::imshow(kWindowPieces, shown);
        } else {
          cv::imshow(kWindowPieces, piece_view);
        }
      } catch (const PieceDetectionError& exc) {
        std::cerr << "Piece detection error: " << exc.what() << "\n";
        safeDestroyWindow(kWindowPieces);
      }
    } else {
      // Only close occupancy window when we are not showing the empty-capture preview.
      if (!(corners.has_value() && board_large_enough && !pieces.hasEmptyReference())) {
        safeDestroyWindow(kWindowPieces);
      }
      if (corners.has_value() && board_large_enough && !pieces.hasEmptyReference()) {
        cv::Mat warped = board.warpBoard(frame, corners);
        cv::Mat preview = board.drawGridOverlay(warped);
        if (preview.cols > 1000) {
          cv::resize(preview, preview, cv::Size(), 0.5, 0.5, cv::INTER_AREA);
        }
        cv::putText(preview,
                    "Clear board, press e",
                    cv::Point(20, 40),
                    cv::FONT_HERSHEY_SIMPLEX,
                    0.8,
                    cv::Scalar(0, 165, 255),
                    2);
        cv::imshow(kWindowPieces, preview);
      }
    }

    const cv::Scalar hud_color = !corners.has_value() ? cv::Scalar(0, 165, 255)
                               : !board_large_enough  ? cv::Scalar(0, 0, 255)
                                                      : cv::Scalar(0, 255, 0);
    cv::putText(camera_view, hud, cv::Point(20, 40), cv::FONT_HERSHEY_SIMPLEX, 0.55, hud_color, 2,
                cv::LINE_AA);
    cv::imshow(kWindowCamera, camera_view);

    const int key = cv::waitKey(1) & 0xFF;
    if (key == 'q') {
      break;
    }
    if (key == 'u') {
      show_undistorted = !show_undistorted;
      pieces.resetTemporalState();
      std::cout << (show_undistorted ? "Undistort ON\n" : "Undistort OFF\n");
    }
    if (key == 'm') {
      show_debug = !show_debug;
      std::cout << (show_debug ? "Debug overlay ON\n" : "Debug overlay OFF\n");
    }
    if (key == 'e' && !capturing_empty) {
      if (!corners.has_value()) {
        std::cout << "Cannot capture: board corners not found.\n";
      } else if (!board_large_enough) {
        std::cout << "REFUSED: board only " << static_cast<int>(board_w)
                  << "px wide (need >=" << static_cast<int>(kMinBoardWidthPx)
                  << "). Move camera closer, re-run ./scripts/run.sh corners, then e.\n";
      } else {
        capturing_empty = true;
        empty_frames.clear();
        std::cout << "Board must be EMPTY. Averaging " << kEmptyCaptureFrames << " frames...\n";
      }
    }
    if (key == 'p' && have_grid) {
      std::cout << "Occupied " << occupied << "/64  (line1 = NEAR side; X=piece .=empty):\n"
                << pieces.toAscii(grid, /*near_side_first=*/true);
      std::cout << "Example for 5+5 pieces on the near two ranks:\n"
                << "...XXXXX\n"
                << "...XXXXX\n"
                << "........\n";
    }
  }

  camera->release();
  destroyGuiWindows();
  return 0;
}
