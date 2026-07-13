"""Capture and save board corners to config."""

from __future__ import annotations

from pathlib import Path

import cv2

from src.apps.runtime import camera_session, read_frame
from src.board import BoardDetector
from src.camera import ChessCamera
from src.config import DEFAULT_CONFIG_PATH
from src.exceptions import BoardDetectionError, CameraError

WINDOW_CAMERA = "Chess Robot - Capture Board Corners"
WINDOW_WARPED = "Chess Robot - Warped Preview"


def print_controls(camera: ChessCamera, detector: BoardDetector) -> None:
    print("Board corner capture running.")
    if camera.is_calibrated:
        print("  Lens calibration: enabled (undistorted preview)")
    else:
        print("  Lens calibration: not set (run python -m src.calibrate_camera)")
    if detector.manual_corners is not None:
        print("  Config currently has manual_corners (preview uses live auto-detect)")
    print("  s - save live-detected corners to board.manual_corners")
    print("  c - clear manual_corners from config")
    print("  q - quit")


def run_capture_corners_loop(config_path: Path = DEFAULT_CONFIG_PATH) -> int:
    detector = BoardDetector(config_path)

    try:
        with camera_session(config_path) as camera:
            print_controls(camera, detector)
            return _capture_loop(camera, detector, config_path)
    except (CameraError, FileNotFoundError):
        return 1


def _capture_loop(
    camera: ChessCamera,
    detector: BoardDetector,
    config_path: Path,
) -> int:
    use_undistort = camera.is_calibrated

    while True:
        frame = read_frame(camera, apply_undistort=use_undistort)
        if frame is None:
            break

        try:
            corners = detector.detect_corners(frame, use_manual=False)
        except BoardDetectionError:
            corners = None

        display = detector.draw_corner_overlay(frame, corners)
        status = "DETECTED - press s to save" if corners is not None else "Searching for board..."
        cv2.putText(
            display,
            status,
            (20, 40),
            cv2.FONT_HERSHEY_SIMPLEX,
            0.8,
            (0, 255, 0) if corners is not None else (0, 165, 255),
            2,
            cv2.LINE_AA,
        )
        cv2.imshow(WINDOW_CAMERA, display)

        if corners is not None:
            warped = detector.warp_board(frame, corners)
            cv2.imshow(WINDOW_WARPED, detector.draw_grid_overlay(warped))
        else:
            cv2.destroyWindow(WINDOW_WARPED)

        key = cv2.waitKey(1) & 0xFF
        if key == ord("q"):
            break
        if key == ord("s"):
            try:
                saved = detector.save_corners_to_config(corners, config_path)
                print(f"Saved manual_corners to {saved}")
                print("Vision loop will use these corners until cleared.")
            except BoardDetectionError as exc:
                print(f"Could not save corners: {exc}")
        if key == ord("c"):
            saved = detector.clear_manual_corners(config_path)
            print(f"Cleared manual_corners in {saved}")

    return 0
