#pragma once

namespace chess_robot {

/** Per-square occupancy only (no white/black side). */
enum class SquareState {
  Empty = 0,
  Occupied = 1,
};

inline bool isOccupied(SquareState state) { return state == SquareState::Occupied; }

/** Chess-square color in image coords: (0,0) top-left is light when white is at bottom. */
inline bool isLightSquare(int row, int col) { return ((row + col) % 2) == 0; }

}  // namespace chess_robot
