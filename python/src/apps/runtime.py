"""Shared GUI / camera session helpers for interactive apps."""

from __future__ import annotations

import sys
from collections.abc import Iterator
from contextlib import contextmanager
from pathlib import Path

import cv2
import numpy as np

from src.camera import ChessCamera
from src.config import DEFAULT_CONFIG_PATH
from src.exceptions import CameraError
from src.utils.opencv import ensure_qt_fonts


def prepare_gui() -> None:
    """One-time OpenCV Qt setup for windowed apps."""
    ensure_qt_fonts()


def open_camera(config_path: Path | str | None = DEFAULT_CONFIG_PATH) -> ChessCamera:
    """Create and open a camera. Re-raises after printing on failure."""
    prepare_gui()
    camera = ChessCamera(config_path)
    try:
        camera.open()
    except (CameraError, FileNotFoundError) as exc:
        print(f"Camera error: {exc}", file=sys.stderr)
        raise
    return camera


@contextmanager
def camera_session(
    config_path: Path | str | None = DEFAULT_CONFIG_PATH,
) -> Iterator[ChessCamera]:
    """Open a camera for a GUI loop and always release + destroy windows."""
    camera = open_camera(config_path)
    try:
        yield camera
    finally:
        camera.release()
        cv2.destroyAllWindows()


def read_frame(camera: ChessCamera, *, apply_undistort: bool) -> np.ndarray | None:
    """Read a frame, printing and returning ``None`` on failure."""
    try:
        return camera.read(apply_undistort=apply_undistort)
    except CameraError as exc:
        print(f"Frame read error: {exc}", file=sys.stderr)
        return None
