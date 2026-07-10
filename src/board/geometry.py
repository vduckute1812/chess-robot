"""Board geometry helpers.

Prefer importing from the focused modules (``corners``, ``detect_chessboard``,
``detect_contour``, ``warp``). This module re-exports them for convenience.
"""

from src.board.corners import (
    corners_to_config_list,
    is_plausible_board_quad,
    order_corners,
    outer_corners_from_inner_grid,
    parse_manual_corners,
)
from src.board.detect_chessboard import find_chessboard_outer_corners
from src.board.detect_contour import find_largest_quadrilateral
from src.board.warp import segment_square_grid, warp_perspective

__all__ = [
    "corners_to_config_list",
    "find_chessboard_outer_corners",
    "find_largest_quadrilateral",
    "is_plausible_board_quad",
    "order_corners",
    "outer_corners_from_inner_grid",
    "parse_manual_corners",
    "segment_square_grid",
    "warp_perspective",
]
