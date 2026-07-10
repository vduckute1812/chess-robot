"""Board corner ordering and validation helpers."""

from __future__ import annotations

import cv2
import numpy as np


def parse_manual_corners(corners: list) -> np.ndarray | None:
    if not corners or len(corners) != 4:
        return None

    points = np.array(corners, dtype=np.float32)
    if points.shape != (4, 2):
        raise ValueError("manual_corners must contain exactly 4 (x, y) points")

    return order_corners(points)


def order_corners(points: np.ndarray) -> np.ndarray:
    """Order points as top-left, top-right, bottom-right, bottom-left."""
    rect = np.zeros((4, 2), dtype=np.float32)
    s = points.sum(axis=1)
    diff = np.diff(points, axis=1).reshape(-1)

    rect[0] = points[np.argmin(s)]
    rect[2] = points[np.argmax(s)]
    rect[1] = points[np.argmin(diff)]
    rect[3] = points[np.argmax(diff)]
    return rect


def outer_corners_from_inner_grid(
    inner_corners: np.ndarray,
    pattern_size: tuple[int, int],
) -> np.ndarray:
    """Extrapolate the four playing-area corners from an inner-corner grid.

    ``pattern_size`` is OpenCV's ``(columns, rows)`` of inner corners, e.g.
    ``(7, 7)`` for an 8x8 chessboard.
    """
    cols, rows = pattern_size
    grid = inner_corners.reshape(rows, cols, 2).astype(np.float32)

    tl = 3.0 * grid[0, 0] - grid[0, 1] - grid[1, 0]
    tr = 3.0 * grid[0, -1] - grid[0, -2] - grid[1, -1]
    br = 3.0 * grid[-1, -1] - grid[-1, -2] - grid[-2, -1]
    bl = 3.0 * grid[-1, 0] - grid[-1, 1] - grid[-2, 0]
    return order_corners(np.stack([tl, tr, br, bl]))


def corners_to_config_list(corners: np.ndarray) -> list[list[float]]:
    """Serialize four corners as ``[[x, y], ...]`` for YAML ``manual_corners``."""
    points = order_corners(corners.astype(np.float32).reshape(4, 2))
    return [[round(float(x), 2), round(float(y), 2)] for x, y in points]


def is_plausible_board_quad(corners: np.ndarray, image_shape: tuple[int, ...]) -> bool:
    """Reject quads that are tiny, near full-frame, non-convex, or wildly skewed."""
    height, width = image_shape[:2]
    pts = corners.astype(np.float32).reshape(4, 2)
    area = float(cv2.contourArea(pts))
    image_area = float(width * height)
    if area < 0.08 * image_area or area > 0.95 * image_area:
        return False
    if not cv2.isContourConvex(pts):
        return False

    ordered = order_corners(pts)
    edges = [
        float(np.linalg.norm(ordered[i] - ordered[(i + 1) % 4]))
        for i in range(4)
    ]
    if min(edges) < 20:
        return False
    if max(edges) / min(edges) > 3.0:
        return False

    margin = 0.35 * max(width, height)
    if np.any(ordered[:, 0] < -margin) or np.any(ordered[:, 0] > width + margin):
        return False
    if np.any(ordered[:, 1] < -margin) or np.any(ordered[:, 1] > height + margin):
        return False
    return True
