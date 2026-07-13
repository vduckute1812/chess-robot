#pragma once

#include <stdexcept>
#include <string>

namespace chess_robot {

class CameraError : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

class CalibrationError : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

class BoardDetectionError : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

class PieceDetectionError : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

}  // namespace chess_robot
