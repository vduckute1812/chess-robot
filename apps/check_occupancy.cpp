#include <algorithm>
#include <iostream>
#include <string>

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include "chess_robot/config.hpp"
#include "chess_robot/exceptions.hpp"
#include "chess_robot/pieces/occupancy.hpp"
#include "chess_robot/pieces/reference.hpp"

namespace {

void printUsage(const char* argv0) {
  std::cerr << "Usage: " << argv0 << " [--config path] [--image path]\n"
            << "  Offline C++ self-check: load empty reference and require 0/64 occupied.\n"
            << "  Also verifies a synthetic dark piece is detected.\n";
}

}  // namespace

int main(int argc, char** argv) {
  using namespace chess_robot;

  fs::path config = defaultConfigPath();
  fs::path image;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--config" && i + 1 < argc) {
      config = argv[++i];
    } else if (arg == "--image" && i + 1 < argc) {
      image = argv[++i];
    } else if (arg == "-h" || arg == "--help") {
      printUsage(argv[0]);
      return 0;
    } else {
      printUsage(argv[0]);
      return 2;
    }
  }

  PieceOccupancyDetector pieces(config);
  if (image.empty()) {
    image = pieces.emptyReferencePath();
  }

  auto loaded = loadEmptyReference(image);
  if (!loaded.has_value()) {
    std::cerr << "FAIL: could not load empty image: " << image << "\n";
    return 1;
  }

  pieces.setEmptyReference(*loaded);

  const OccupancyGrid self = pieces.detectRaw(*loaded);
  const int self_occ = PieceOccupancyDetector::countOccupied(self);
  std::cout << "Empty self-check on " << image << "\n";
  std::cout << pieces.toAscii(self);
  std::cout << "occupied=" << self_occ << "/64\n";

  cv::Mat with_piece = loaded->clone();
  const int cell = with_piece.rows / pieces.gridSize();
  // Place a dark disk on rank-2 file-a style cell (near bottom-left of image).
  const int row = pieces.gridSize() - 2;
  const int col = 0;
  cv::circle(with_piece,
             cv::Point(col * cell + cell / 2, row * cell + cell / 2),
             std::max(8, cell / 4),
             cv::Scalar(30, 30, 30),
             -1,
             cv::LINE_AA);

  const OccupancyGrid with = pieces.detectRaw(with_piece);
  const int with_occ = PieceOccupancyDetector::countOccupied(with);
  std::cout << "Synthetic piece check (expect occupied at r=" << row << " c=" << col << "):\n";
  std::cout << pieces.toAscii(with);
  std::cout << "occupied=" << with_occ << "/64 cell="
            << (with[row][col] != SquareState::Empty ? "HIT" : "MISS") << "\n";

  int status = 0;
  if (self_occ != 0) {
    std::cerr << "FAIL: empty board must be 0/64 (got " << self_occ << ")\n";
    status = 1;
  }
  if (with[row][col] == SquareState::Empty) {
    std::cerr << "FAIL: synthetic piece not detected\n";
    status = 1;
  }
  if (status == 0) {
    std::cout << "PASS\n";
  }
  return status;
}
