"""OpenCV helpers."""

from __future__ import annotations

import sys
from contextlib import contextmanager
from typing import Iterator

import cv2


def v4l2_backend() -> int:
    return cv2.CAP_V4L2 if sys.platform.startswith("linux") else cv2.CAP_ANY


@contextmanager
def suppress_opencv_logs() -> Iterator[None]:
    """Hide noisy V4L2 probe warnings while scanning for a camera."""
    try:
        level = cv2.utils.logging.getLogLevel()
        cv2.utils.logging.setLogLevel(cv2.utils.logging.LOG_LEVEL_SILENT)
        yield
    finally:
        cv2.utils.logging.setLogLevel(level)
