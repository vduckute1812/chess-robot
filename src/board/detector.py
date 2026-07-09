"""Chessboard detection, perspective warp, and grid segmentation."""

from __future__ import annotations

from pathlib import Path

import cv2
import numpy as np

from src.board.geometry import (
    find_largest_quadrilateral,
    parse_manual_corners,
    segment_square_grid,
    warp_perspective,
)
from src.config import load_config, resolve_config_path
from src.exceptions import BoardDetectionError


class BoardDetector:
    """Detects the chessboard, warps to a top-down view, and segments the grid."""

    def __init__(self, config_path: str | Path | None = None) -> None:
        config = load_config(config_path)
        board_cfg = config.get("board", {})
        detection_cfg = config.get("detection", {})

        self.output_size = int(board_cfg.get("output_size", 800))
        self.grid_size = int(board_cfg.get("grid_size", 8))
        self.manual_corners = parse_manual_corners(board_cfg.get("manual_corners", []))

        self.canny_low = int(detection_cfg.get("canny_low", 50))
        self.canny_high = int(detection_cfg.get("canny_high", 150))
        self.blur_kernel = int(detection_cfg.get("blur_kernel", 5))
        self.min_contour_area = float(detection_cfg.get("min_contour_area", 5000))

        self._config_path = resolve_config_path(config_path)
        self._last_corners: np.ndarray | None = None

    def detect_corners(self, frame: np.ndarray) -> np.ndarray:
        if self.manual_corners is not None:
            corners = self.manual_corners.copy()
        else:
            corners = find_largest_quadrilateral(
                frame,
                canny_low=self.canny_low,
                canny_high=self.canny_high,
                blur_kernel=self.blur_kernel,
                min_contour_area=self.min_contour_area,
            )
            if corners is None:
                if self._last_corners is not None:
                    return self._last_corners.copy()
                raise BoardDetectionError(
                    "Could not detect chessboard corners. "
                    "Try improving lighting, centering the board, or setting manual_corners."
                )

        self._last_corners = corners.copy()
        return corners

    def warp_board(self, frame: np.ndarray, corners: np.ndarray | None = None) -> np.ndarray:
        if corners is None:
            corners = self.detect_corners(frame)
        return warp_perspective(frame, corners, self.output_size)

    def segment_grid(self, warped_board: np.ndarray) -> list[list[np.ndarray]]:
        try:
            return segment_square_grid(warped_board, self.grid_size)
        except ValueError as exc:
            raise BoardDetectionError(str(exc)) from exc

    def draw_grid_overlay(self, warped_board: np.ndarray) -> np.ndarray:
        overlay = warped_board.copy()
        size = warped_board.shape[0]
        step = size // self.grid_size

        for i in range(1, self.grid_size):
            pos = i * step
            cv2.line(overlay, (pos, 0), (pos, size - 1), (0, 255, 0), 1)
            cv2.line(overlay, (0, pos), (size - 1, pos), (0, 255, 0), 1)

        return overlay

    def draw_corner_overlay(self, frame: np.ndarray, corners: np.ndarray | None = None) -> np.ndarray:
        overlay = frame.copy()
        if corners is None:
            try:
                corners = self.detect_corners(frame)
            except BoardDetectionError:
                return overlay

        pts = corners.astype(np.int32).reshape((-1, 1, 2))
        cv2.polylines(overlay, [pts], isClosed=True, color=(0, 255, 0), thickness=2)
        for idx, (x, y) in enumerate(corners.astype(np.int32)):
            cv2.circle(overlay, (int(x), int(y)), 6, (0, 0, 255), -1)
            cv2.putText(
                overlay,
                str(idx),
                (int(x) + 8, int(y) - 8),
                cv2.FONT_HERSHEY_SIMPLEX,
                0.5,
                (255, 255, 255),
                1,
                cv2.LINE_AA,
            )

        return overlay
