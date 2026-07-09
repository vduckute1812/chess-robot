"""Lens undistortion using stored calibration matrices."""

from __future__ import annotations

import cv2
import numpy as np


class FrameUndistorter:
    """Applies precomputed remap tables for lens correction."""

    def __init__(self, camera_matrix: np.ndarray, dist_coeffs: np.ndarray) -> None:
        self._camera_matrix = camera_matrix
        self._dist_coeffs = dist_coeffs
        self._maps: tuple[np.ndarray, np.ndarray] | None = None

    def apply(self, frame: np.ndarray) -> np.ndarray:
        self._ensure_maps(frame.shape)
        assert self._maps is not None
        map1, map2 = self._maps
        return cv2.remap(frame, map1, map2, interpolation=cv2.INTER_LINEAR)

    def _ensure_maps(self, frame_shape: tuple[int, ...]) -> None:
        height, width = frame_shape[:2]
        if self._maps is not None:
            map1, map2 = self._maps
            if map1.shape[0] == height and map1.shape[1] == width:
                return

        new_matrix, _roi = cv2.getOptimalNewCameraMatrix(
            self._camera_matrix,
            self._dist_coeffs,
            (width, height),
            alpha=0,
            newImgSize=(width, height),
        )
        map1, map2 = cv2.initUndistortRectifyMap(
            self._camera_matrix,
            self._dist_coeffs,
            None,
            new_matrix,
            (width, height),
            cv2.CV_16SC2,
        )
        self._maps = (map1, map2)

    def reset(self) -> None:
        self._maps = None
