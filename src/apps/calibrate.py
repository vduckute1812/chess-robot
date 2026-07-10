"""Camera calibration application loop."""

from __future__ import annotations

from pathlib import Path

import cv2

from src.apps.runtime import camera_session, read_frame
from src.calibration import CameraCalibrator
from src.camera import ChessCamera
from src.config import DEFAULT_CONFIG_PATH
from src.exceptions import CalibrationError, CameraError

WINDOW = "Chess Robot - Camera Calibration"


def print_controls(calibrator: CameraCalibrator) -> None:
    print("Camera calibration running.")
    print(f"  Pattern: {calibrator.cols}x{calibrator.rows} inner corners")
    print(f"  Target samples: {calibrator.min_samples}")
    print("  SPACE - capture sample when checkerboard is detected")
    print("  c     - compute calibration and save to config")
    print("  r     - reset collected samples")
    print("  q     - quit")


def run_calibration_loop(config_path: Path = DEFAULT_CONFIG_PATH) -> int:
    calibrator = CameraCalibrator(config_path)

    try:
        with camera_session(config_path) as camera:
            print_controls(calibrator)
            return _calibration_loop(camera, calibrator, config_path)
    except (CameraError, FileNotFoundError):
        return 1


def _calibration_loop(
    camera: ChessCamera,
    calibrator: CameraCalibrator,
    config_path: Path,
) -> int:
    while True:
        frame = read_frame(camera, apply_undistort=False)
        if frame is None:
            break

        overlay = calibrator.draw_overlay(frame)
        cv2.imshow(WINDOW, overlay)

        key = cv2.waitKey(1) & 0xFF
        if key == ord("q"):
            break
        if key == ord("r"):
            calibrator.reset_samples()
            print("Reset calibration samples.")
        if key == ord(" "):
            if calibrator.add_sample(frame):
                print(f"Captured sample {calibrator.sample_count}.")
            else:
                print("Checkerboard not detected. Adjust board and try again.")
        if key == ord("c"):
            try:
                _matrix, _dist, error = calibrator.calibrate()
                saved_path = calibrator.save_to_config(config_path)
                print(f"Calibration saved to {saved_path}")
                print(f"Reprojection error: {error:.4f} pixels")
                print("Restart the vision loop to use undistorted frames.")
                break
            except CalibrationError as exc:
                print(f"Calibration error: {exc}")

    return 0
