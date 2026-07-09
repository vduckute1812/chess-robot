"""Vision system application loop."""

from __future__ import annotations

import sys
from pathlib import Path

import cv2

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


def run_vision_loop(config_path: Path = DEFAULT_CONFIG_PATH) -> int:
    camera = ChessCamera(config_path)
    detector = BoardDetector(config_path)

    try:
        camera.open()
    except (CameraError, FileNotFoundError) as exc:
        print(f"Camera error: {exc}", file=sys.stderr)
        return 1

    print_controls(camera)

    show_detection = True
    show_warped = True
    show_grid = True
    show_undistorted = camera.is_calibrated

    try:
        while True:
            try:
                frame = camera.read(apply_undistort=show_undistorted)
            except CameraError as exc:
                print(f"Frame read error: {exc}", file=sys.stderr)
                break

            warped = None
            grid_view = None

            display = detector.draw_corner_overlay(frame) if show_detection else frame.copy()

            if show_warped or show_grid:
                try:
                    corners = detector.detect_corners(frame)
                    warped = detector.warp_board(frame, corners)
                    if show_grid:
                        grid_view = detector.draw_grid_overlay(warped)
                except BoardDetectionError:
                    warped = None
                    grid_view = None

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

    finally:
        camera.release()
        cv2.destroyAllWindows()

    return 0
