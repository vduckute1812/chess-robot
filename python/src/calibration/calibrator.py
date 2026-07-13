"""Interactive checkerboard calibration workflow."""

from __future__ import annotations

from pathlib import Path

import cv2
import numpy as np

from src.calibration.checkerboard import (
    build_object_points,
    draw_checkerboard_overlay,
    find_checkerboard_corners,
)
from src.calibration.matrices import save_calibration_matrices
from src.config import load_section, resolve_config_path
from src.exceptions import CalibrationError


class CameraCalibrator:
    """Collects checkerboard samples and computes camera intrinsics."""

    def __init__(self, config_path: str | Path | None = None) -> None:
        self._config_path = resolve_config_path(config_path)
        cal_cfg = load_section("calibration", self._config_path)
        board_cfg = cal_cfg.get("checkerboard", {})

        self.cols = int(board_cfg.get("cols", 9))
        self.rows = int(board_cfg.get("rows", 6))
        self.square_size_mm = float(board_cfg.get("square_size_mm", 25.0))
        self.min_samples = int(cal_cfg.get("min_samples", 15))

        self._pattern_size = (self.cols, self.rows)
        self._object_template = build_object_points(self.cols, self.rows, self.square_size_mm)
        self._object_points: list[np.ndarray] = []
        self._image_points: list[np.ndarray] = []
        self._image_size: tuple[int, int] | None = None

        self.camera_matrix: np.ndarray | None = None
        self.dist_coeffs: np.ndarray | None = None
        self.reprojection_error: float | None = None

    @property
    def sample_count(self) -> int:
        return len(self._object_points)

    @property
    def is_calibrated(self) -> bool:
        return self.camera_matrix is not None and self.dist_coeffs is not None

    def find_checkerboard(
        self,
        frame: np.ndarray,
        *,
        robust: bool = False,
    ) -> tuple[bool, np.ndarray | None]:
        return find_checkerboard_corners(frame, self._pattern_size, robust=robust)

    def add_sample(self, frame: np.ndarray) -> bool:
        found, corners = self.find_checkerboard(frame, robust=True)
        if not found or corners is None:
            return False

        height, width = frame.shape[:2]
        if self._image_size is None:
            self._image_size = (width, height)
        elif self._image_size != (width, height):
            raise CalibrationError(
                "All calibration frames must have the same resolution. "
                f"Expected {self._image_size}, got {(width, height)}."
            )

        self._object_points.append(self._object_template.copy())
        self._image_points.append(corners)
        return True

    def calibrate(self) -> tuple[np.ndarray, np.ndarray, float]:
        if self.sample_count < self.min_samples:
            raise CalibrationError(
                f"Need at least {self.min_samples} valid samples, have {self.sample_count}."
            )
        if self._image_size is None:
            raise CalibrationError("No calibration samples collected.")

        error, camera_matrix, dist_coeffs, _rvecs, _tvecs = cv2.calibrateCamera(
            self._object_points,
            self._image_points,
            self._image_size,
            None,
            None,
        )

        self.camera_matrix = camera_matrix
        self.dist_coeffs = dist_coeffs
        self.reprojection_error = float(error)
        return camera_matrix, dist_coeffs, float(error)

    def draw_overlay(self, frame: np.ndarray) -> np.ndarray:
        found, corners = self.find_checkerboard(frame, robust=False)
        if found and corners is not None:
            overlay = draw_checkerboard_overlay(frame, self._pattern_size, corners)
        else:
            overlay = frame.copy()

        ready = self.sample_count >= self.min_samples
        status = (
            f"Samples: {self.sample_count}/{self.min_samples}  "
            f"{'READY' if ready else 'collect more'}"
        )
        cv2.putText(
            overlay,
            status,
            (20, 40),
            cv2.FONT_HERSHEY_SIMPLEX,
            0.8,
            (0, 255, 0),
            2,
            cv2.LINE_AA,
        )
        return overlay

    def save_to_config(self, config_path: str | Path | None = None) -> Path:
        if not self.is_calibrated or self._image_size is None:
            raise CalibrationError("Calibrate before saving results.")

        assert self.camera_matrix is not None
        assert self.dist_coeffs is not None
        assert self.reprojection_error is not None

        return save_calibration_matrices(
            self.camera_matrix,
            self.dist_coeffs,
            self.reprojection_error,
            self._image_size,
            config_path or self._config_path,
        )

    def reset_samples(self) -> None:
        self._object_points.clear()
        self._image_points.clear()
        self._image_size = None
