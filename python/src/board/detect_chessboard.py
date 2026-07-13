"""Detect outer board corners from an OpenCV chessboard inner-corner grid."""

from __future__ import annotations

import numpy as np

from src.board.corners import is_plausible_board_quad, outer_corners_from_inner_grid
from src.utils.chessboard import find_inner_corners


def find_chessboard_outer_corners(
    frame: np.ndarray,
    *,
    pattern_size: tuple[int, int],
) -> np.ndarray | None:
    """Return ordered playing-area corners, or ``None`` if the pattern is missing."""
    inner = find_inner_corners(frame, pattern_size, robust=True)
    if inner is None:
        return None

    outer = outer_corners_from_inner_grid(inner, pattern_size)
    if not is_plausible_board_quad(outer, frame.shape):
        return None
    return outer
