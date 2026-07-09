"""Checkerboard detection for intrinsic calibration."""

from __future__ import annotations

import cv2
import numpy as np


def find_checkerboard_corners(
    frame: np.ndarray,
    pattern_size: tuple[int, int],
) -> tuple[bool, np.ndarray | None]:
    gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
    found, corners = cv2.findChessboardCorners(gray, pattern_size, None)
    if not found or corners is None:
        return False, None

    criteria = (cv2.TERM_CRITERIA_EPS + cv2.TERM_CRITERIA_MAX_ITER, 30, 0.001)
    refined = cv2.cornerSubPix(gray, corners, (11, 11), (-1, -1), criteria)
    return True, refined


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
