"""OpenCV helpers."""

from __future__ import annotations

import sys
from contextlib import contextmanager
from pathlib import Path
from typing import Iterator

import cv2

_SYSTEM_FONT_DIRS = (
    Path("/usr/share/fonts/truetype/dejavu"),
    Path("/usr/share/fonts/TTF"),
    Path("/usr/share/fonts/truetype/liberation"),
)


def v4l2_backend() -> int:
    return cv2.CAP_V4L2 if sys.platform.startswith("linux") else cv2.CAP_ANY


def ensure_qt_fonts() -> None:
    """Point OpenCV's bundled Qt at system fonts to silence QFontDatabase warnings."""
    qt_fonts = Path(cv2.__file__).resolve().parent / "qt" / "fonts"
    if qt_fonts.is_dir() and any(qt_fonts.glob("*.ttf")):
        return

    source_dir = next((path for path in _SYSTEM_FONT_DIRS if path.is_dir()), None)
    if source_dir is None:
        return

    fonts = sorted(source_dir.glob("*.ttf"))
    if not fonts:
        return

    qt_fonts.mkdir(parents=True, exist_ok=True)
    for font in fonts:
        target = qt_fonts / font.name
        if target.exists():
            continue
        try:
            target.symlink_to(font)
        except OSError:
            continue


@contextmanager
def suppress_opencv_logs() -> Iterator[None]:
    """Hide noisy V4L2 probe warnings while scanning for a camera."""
    try:
        level = cv2.utils.logging.getLogLevel()
        cv2.utils.logging.setLogLevel(cv2.utils.logging.LOG_LEVEL_SILENT)
        yield
    finally:
        cv2.utils.logging.setLogLevel(level)
