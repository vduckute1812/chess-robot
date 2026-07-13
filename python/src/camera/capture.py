"""Webcam capture and frame streaming."""

from __future__ import annotations

from pathlib import Path

import cv2
import numpy as np

from src.calibration.matrices import load_calibration_matrices
from src.camera.discovery import discover_capture, list_devices
from src.camera.undistort import FrameUndistorter
from src.config import load_section, resolve_config_path
from src.exceptions import CameraError


class ChessCamera:
    """Manages a USB webcam stream with configurable resolution and undistortion."""

    def __init__(self, config_path: str | Path | None = None) -> None:
        self._config_path = resolve_config_path(config_path)
        self._config = load_section("camera", self._config_path)
        self._cap: cv2.VideoCapture | None = None
        self._opened = False
        self._active_device: int | str | None = None
        self._undistorter: FrameUndistorter | None = None
        self._load_undistorter()

    def _load_undistorter(self) -> None:
        matrices = load_calibration_matrices(self._config_path)
        if matrices is None:
            return
        camera_matrix, dist_coeffs = matrices
        self._undistorter = FrameUndistorter(camera_matrix, dist_coeffs)

    @property
    def device_id(self) -> int:
        return int(self._config.get("device_id", 0))

    @property
    def device_path(self) -> str | None:
        path = self._config.get("device_path")
        return None if path in (None, "null", "") else str(path)

    @property
    def width(self) -> int:
        return int(self._config.get("width", 1280))

    @property
    def height(self) -> int:
        return int(self._config.get("height", 720))

    @property
    def fps(self) -> int:
        return int(self._config.get("fps", 30))

    @property
    def is_open(self) -> bool:
        return self._opened and self._cap is not None and self._cap.isOpened()

    @property
    def is_calibrated(self) -> bool:
        return self._undistorter is not None

    @property
    def active_device(self) -> int | str | None:
        return self._active_device

    @staticmethod
    def list_devices(max_index: int = 10) -> list[dict]:
        return list_devices(max_index)

    def open(self) -> None:
        if self.is_open:
            return

        discovered = discover_capture(self.device_path, self.device_id)
        if discovered is None:
            available = self.list_devices()
            names = [device["path"] for device in available] or ["none"]
            raise CameraError(
                "Failed to open any camera device. "
                "Check that the webcam is connected, not in use by another app, "
                "and that your user is in the 'video' group. "
                f"Set camera.device_path or camera.device_id in config/camera_config.yaml. "
                f"Working devices found: {', '.join(names)}."
            )

        device, cap = discovered
        self._active_device = device

        configured = self.device_path or self.device_id
        if device != configured:
            print(f"Camera: using {device} (configured {configured} unavailable)")

        cap.set(cv2.CAP_PROP_FRAME_WIDTH, self.width)
        cap.set(cv2.CAP_PROP_FRAME_HEIGHT, self.height)
        cap.set(cv2.CAP_PROP_FPS, self.fps)

        self._cap = cap
        self._opened = True
        if self._undistorter is not None:
            self._undistorter.reset()

    def read(self, apply_undistort: bool = True) -> np.ndarray:
        if not self.is_open:
            raise CameraError("Camera is not open. Call open() first.")

        assert self._cap is not None
        ok, frame = self._cap.read()
        if not ok or frame is None:
            raise CameraError(
                "Failed to read frame from camera. "
                "The device may have been disconnected."
            )

        if apply_undistort and self._undistorter is not None:
            return self._undistorter.apply(frame)

        return frame

    def release(self) -> None:
        if self._cap is not None:
            self._cap.release()
            self._cap = None
        self._opened = False
        self._active_device = None
        if self._undistorter is not None:
            self._undistorter.reset()

    def __enter__(self) -> ChessCamera:
        self.open()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb) -> None:
        self.release()
