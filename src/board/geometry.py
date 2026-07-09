"""Board corner geometry helpers."""

from __future__ import annotations

import cv2
import numpy as np


def parse_manual_corners(corners: list) -> np.ndarray | None:
    if not corners or len(corners) != 4:
        return None

    points = np.array(corners, dtype=np.float32)
    if points.shape != (4, 2):
        raise ValueError("manual_corners must contain exactly 4 (x, y) points")

    return points


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


def find_largest_quadrilateral(
    frame: np.ndarray,
    *,
    canny_low: int,
    canny_high: int,
    blur_kernel: int,
    min_contour_area: float,
) -> np.ndarray | None:
    gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
    kernel = blur_kernel if blur_kernel % 2 == 1 else blur_kernel + 1
    blurred = cv2.GaussianBlur(gray, (kernel, kernel), 0)
    edges = cv2.Canny(blurred, canny_low, canny_high)

    contours, _ = cv2.findContours(edges, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
    contours = sorted(contours, key=cv2.contourArea, reverse=True)

    for contour in contours[:10]:
        area = cv2.contourArea(contour)
        if area < min_contour_area:
            continue

        peri = cv2.arcLength(contour, True)
        approx = cv2.approxPolyDP(contour, 0.02 * peri, True)
        if len(approx) == 4:
            return order_corners(approx.reshape(4, 2).astype(np.float32))

    return None


def warp_perspective(frame: np.ndarray, corners: np.ndarray, output_size: int) -> np.ndarray:
    dst = np.array(
        [
            [0, 0],
            [output_size - 1, 0],
            [output_size - 1, output_size - 1],
            [0, output_size - 1],
        ],
        dtype=np.float32,
    )
    matrix = cv2.getPerspectiveTransform(corners, dst)
    return cv2.warpPerspective(frame, matrix, (output_size, output_size))


def segment_square_grid(warped_board: np.ndarray, grid_size: int) -> list[list[np.ndarray]]:
    if warped_board.shape[0] != warped_board.shape[1]:
        raise ValueError("Warped board image must be square.")

    cell_size = warped_board.shape[0] // grid_size
    grid: list[list[np.ndarray]] = []

    for row in range(grid_size):
        row_cells: list[np.ndarray] = []
        y0 = row * cell_size
        y1 = y0 + cell_size
        for col in range(grid_size):
            x0 = col * cell_size
            x1 = x0 + cell_size
            row_cells.append(warped_board[y0:y1, x0:x1].copy())
        grid.append(row_cells)

    return grid
