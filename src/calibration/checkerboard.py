"""Checkerboard detection for intrinsic calibration."""

from __future__ import annotations

import cv2
import numpy as np

from src.utils.chessboard import find_inner_corners


def find_checkerboard_corners(
    frame: np.ndarray,
    pattern_size: tuple[int, int],
    *,
    robust: bool = True,
) -> tuple[bool, np.ndarray | None]:
    corners = find_inner_corners(frame, pattern_size, robust=robust)
    if corners is None:
        return False, None
    return True, corners


def draw_checkerboard_overlay(
    frame: np.ndarray,
    pattern_size: tuple[int, int],
    corners: np.ndarray,
) -> np.ndarray:
    overlay = frame.copy()
    cv2.drawChessboardCorners(overlay, pattern_size, corners, True)
    return overlay


def build_object_points(
    cols: int,
    rows: int,
    square_size_mm: float,
) -> np.ndarray:
    grid = np.zeros((rows * cols, 3), np.float32)
    grid[:, :2] = np.mgrid[0:cols, 0:rows].T.reshape(-1, 2)
    grid *= square_size_mm
    return grid
