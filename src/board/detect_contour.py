"""Contour-based outer board corner fallback."""

from __future__ import annotations

import cv2
import numpy as np

from src.board.corners import is_plausible_board_quad, order_corners
from src.utils.chessboard import to_gray

_APPROX_EPSILONS = (0.02, 0.03, 0.04, 0.05)


def _approx_quad(contour: np.ndarray) -> np.ndarray | None:
    peri = cv2.arcLength(contour, True)
    if peri <= 0:
        return None

    for epsilon_scale in _APPROX_EPSILONS:
        approx = cv2.approxPolyDP(contour, epsilon_scale * peri, True)
        if len(approx) == 4 and cv2.isContourConvex(approx):
            return approx.reshape(4, 2).astype(np.float32)
    return None


def find_largest_quadrilateral(
    frame: np.ndarray,
    *,
    canny_low: int,
    canny_high: int,
    blur_kernel: int,
    min_contour_area: float,
) -> np.ndarray | None:
    gray = to_gray(frame)
    kernel = blur_kernel if blur_kernel % 2 == 1 else blur_kernel + 1
    blurred = cv2.GaussianBlur(gray, (kernel, kernel), 0)
    edges = cv2.Canny(blurred, canny_low, canny_high)

    morph_kernel = cv2.getStructuringElement(cv2.MORPH_RECT, (5, 5))
    edges = cv2.morphologyEx(edges, cv2.MORPH_CLOSE, morph_kernel, iterations=2)

    contours, _ = cv2.findContours(edges, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
    contours = sorted(contours, key=cv2.contourArea, reverse=True)

    for contour in contours[:10]:
        area = cv2.contourArea(contour)
        if area < min_contour_area:
            continue

        for candidate in (contour, cv2.convexHull(contour)):
            quad = _approx_quad(candidate)
            if quad is None:
                continue

            ordered = order_corners(quad)
            if not is_plausible_board_quad(ordered, frame.shape):
                continue
            return ordered

    return None
