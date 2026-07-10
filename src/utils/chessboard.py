"""Shared OpenCV chessboard / checkerboard inner-corner finding."""

from __future__ import annotations

import cv2
import numpy as np

SUBPIX_CRITERIA = (
    cv2.TERM_CRITERIA_EPS + cv2.TERM_CRITERIA_MAX_ITER,
    30,
    0.001,
)
SUBPIX_WIN_SIZE = (11, 11)
SUBPIX_ZERO_ZONE = (-1, -1)

_CLASSIC_BASE = cv2.CALIB_CB_ADAPTIVE_THRESH | cv2.CALIB_CB_NORMALIZE_IMAGE
_FAST_FLAGS = _CLASSIC_BASE | cv2.CALIB_CB_FAST_CHECK
_SLOW_FLAGS = _CLASSIC_BASE


def to_gray(frame: np.ndarray) -> np.ndarray:
    if frame.ndim == 2:
        return frame
    return cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)


def prepare_gray_variants(frame: np.ndarray) -> list[np.ndarray]:
    """Return gray images with optional CLAHE / blur for harder lighting."""
    gray = to_gray(frame)
    clahe = cv2.createCLAHE(clipLimit=2.0, tileGridSize=(8, 8))
    blurred = cv2.GaussianBlur(gray, (5, 5), 0)
    return [gray, clahe.apply(gray), clahe.apply(blurred)]


def refine_corners(gray: np.ndarray, corners: np.ndarray) -> np.ndarray:
    return cv2.cornerSubPix(
        gray,
        corners,
        winSize=SUBPIX_WIN_SIZE,
        zeroZone=SUBPIX_ZERO_ZONE,
        criteria=SUBPIX_CRITERIA,
    )


def _find_classic(gray: np.ndarray, pattern_size: tuple[int, int], flags: int) -> np.ndarray | None:
    found, corners = cv2.findChessboardCorners(gray, pattern_size, flags)
    if not found or corners is None:
        return None
    return refine_corners(gray, corners)


def _find_sb(gray: np.ndarray, pattern_size: tuple[int, int]) -> np.ndarray | None:
    if not hasattr(cv2, "findChessboardCornersSB"):
        return None

    flags = 0
    if hasattr(cv2, "CALIB_CB_ACCURACY"):
        flags |= cv2.CALIB_CB_ACCURACY

    found, corners = cv2.findChessboardCornersSB(gray, pattern_size, flags=flags)
    if not found or corners is None:
        return None
    return corners


def find_inner_corners(
    frame: np.ndarray,
    pattern_size: tuple[int, int],
    *,
    robust: bool = False,
) -> np.ndarray | None:
    """Find OpenCV inner corners for ``pattern_size`` ``(cols, rows)``.

    ``robust=False`` uses a single gray image (fast, good for calibration HUD).
    ``robust=True`` tries contrast variants, slow classic flags, then SB.
    """
    cols, rows = pattern_size
    if cols < 2 or rows < 2:
        return None

    if not robust:
        gray = to_gray(frame)
        return _find_classic(gray, pattern_size, _FAST_FLAGS) or _find_classic(
            gray,
            pattern_size,
            _SLOW_FLAGS,
        )

    variants = prepare_gray_variants(frame)
    for gray in variants:
        corners = _find_classic(gray, pattern_size, _FAST_FLAGS)
        if corners is not None:
            return corners

    for gray in variants:
        corners = _find_classic(gray, pattern_size, _SLOW_FLAGS)
        if corners is not None:
            return corners
        corners = _find_sb(gray, pattern_size)
        if corners is not None:
            return corners

    return None
