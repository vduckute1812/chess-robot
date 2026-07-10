"""Vision system application loop."""

from __future__ import annotations

from pathlib import Path

import cv2

from src.apps.runtime import camera_session, read_frame
from src.board import BoardDetector
from src.camera import ChessCamera
from src.config import DEFAULT_CONFIG_PATH
from src.exceptions import BoardDetectionError, CameraError

WINDOW_CAMERA = "Chess Robot - Camera Feed"
WINDOW_WARPED = "Chess Robot - Warped Board"
WINDOW_GRID = "Chess Robot - Grid Overlay"


def print_controls(camera: ChessCamera) -> None:
    print("Vision test running.")
    if camera.is_calibrated:
        print("  Lens calibration: enabled")
    else:
        print("  Lens calibration: not set (run python -m src.calibrate_camera)")
    print("  q - quit")
    print("  u - toggle undistorted/raw camera view")
    print("  d - toggle board detection overlay")
    print("  w - toggle warped board view")
    print("  g - toggle grid overlay")
    print("  Tip: save corners with python -m src.capture_board_corners")


def run_vision_loop(config_path: Path = DEFAULT_CONFIG_PATH) -> int:
    detector = BoardDetector(config_path)

    try:
        with camera_session(config_path) as camera:
            print_controls(camera)
            return _vision_loop(camera, detector)
    except (CameraError, FileNotFoundError):
        return 1


def _vision_loop(camera: ChessCamera, detector: BoardDetector) -> int:
    show_detection = True
    show_warped = True
    show_grid = True
    show_undistorted = camera.is_calibrated

    while True:
        frame = read_frame(camera, apply_undistort=show_undistorted)
        if frame is None:
            break

        try:
            corners = detector.detect_corners(frame)
        except BoardDetectionError:
            corners = None

        display = (
            detector.draw_corner_overlay(frame, corners)
            if show_detection
            else frame.copy()
        )

        warped = None
        grid_view = None
        if corners is not None and (show_warped or show_grid):
            warped = detector.warp_board(frame, corners)
            if show_grid:
                grid_view = detector.draw_grid_overlay(warped)

        cv2.imshow(WINDOW_CAMERA, display)

        if show_warped and warped is not None:
            cv2.imshow(WINDOW_WARPED, warped)
        elif not show_warped:
            cv2.destroyWindow(WINDOW_WARPED)

        if show_grid and grid_view is not None:
            cv2.imshow(WINDOW_GRID, grid_view)
        elif not show_grid:
            cv2.destroyWindow(WINDOW_GRID)

        key = cv2.waitKey(1) & 0xFF
        if key == ord("q"):
            break
        if key == ord("u"):
            show_undistorted = not show_undistorted
        if key == ord("d"):
            show_detection = not show_detection
        if key == ord("w"):
            show_warped = not show_warped
        if key == ord("g"):
            show_grid = not show_grid

    return 0
