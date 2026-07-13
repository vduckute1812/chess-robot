"""Camera device discovery and probing."""

from __future__ import annotations

import glob
import sys
from typing import Any

import cv2
import numpy as np

from src.utils.opencv import suppress_opencv_logs, v4l2_backend


def create_capture(device: int | str) -> cv2.VideoCapture:
    return cv2.VideoCapture(device, v4l2_backend())


def probe_capture(device: int | str) -> cv2.VideoCapture | None:
    """Open a device and verify at least one frame can be read."""
    cap = create_capture(device)
    if not cap.isOpened():
        cap.release()
        return None

    ok, frame = cap.read()
    if not ok or frame is None:
        cap.release()
        return None

    return cap


def build_device_candidates(
    device_path: str | None,
    device_id: int,
    max_index: int = 10,
) -> list[int | str]:
    candidates: list[int | str] = []

    if device_path:
        candidates.append(device_path)

    if device_id not in candidates:
        candidates.append(device_id)

    if sys.platform.startswith("linux"):
        for path in sorted(glob.glob("/dev/video*")):
            if path not in candidates:
                candidates.append(path)

    for index in range(max_index):
        if index not in candidates:
            candidates.append(index)

    return candidates


def discover_capture(
    device_path: str | None,
    device_id: int,
) -> tuple[int | str, cv2.VideoCapture] | None:
    """Return the first working device and an already-open capture handle."""
    with suppress_opencv_logs():
        for device in build_device_candidates(device_path, device_id):
            cap = probe_capture(device)
            if cap is not None:
                return device, cap
    return None


def list_devices(max_index: int = 10) -> list[dict[str, Any]]:
    """Return capture devices that OpenCV can open and read from."""
    devices: list[dict[str, Any]] = []
    seen: set[int | str] = set()

    def record(device: int | str, frame: np.ndarray) -> None:
        if device in seen:
            return
        seen.add(device)
        height, width = frame.shape[:2]
        path = device if isinstance(device, str) else f"index {device}"
        devices.append(
            {
                "device": device,
                "path": path,
                "resolution": f"{width}x{height}",
                "works": True,
            }
        )

    candidates: list[int | str] = []
    if sys.platform.startswith("linux"):
        candidates.extend(sorted(glob.glob("/dev/video*")))
    candidates.extend(range(max_index))

    with suppress_opencv_logs():
        for device in candidates:
            cap = create_capture(device)
            if not cap.isOpened():
                cap.release()
                continue
            ok, frame = cap.read()
            if ok and frame is not None:
                record(device, frame)
            cap.release()

    return devices
