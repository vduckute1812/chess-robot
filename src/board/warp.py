"""Perspective warp and square-grid segmentation."""

from __future__ import annotations

import cv2
import numpy as np


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
