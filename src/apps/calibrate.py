"""Camera calibration application loop."""

from __future__ import annotations

import sys
from pathlib import Path

import cv2

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
    camera = ChessCamera(config_path)
    calibrator = CameraCalibrator(config_path)

    try:
        camera.open()
    except (CameraError, FileNotFoundError) as exc:
        print(f"Camera error: {exc}", file=sys.stderr)
        return 1

    print_controls(calibrator)

    try:
        while True:
            try:
                frame = camera.read(apply_undistort=False)
            except CameraError as exc:
                print(f"Frame read error: {exc}", file=sys.stderr)
                break

            overlay = calibrator.draw_overlay(frame)
            status = (
                f"Samples: {calibrator.sample_count}/{calibrator.min_samples}  "
                f"{'READY' if calibrator.sample_count >= calibrator.min_samples else 'collect more'}"
            )
            cv2.putText(
                overlay,
                status,
                (20, 40),
                cv2.FONT_HERSHEY_SIMPLEX,
                0.8,
                (0, 255, 0),
                2,
                cv2.LINE_AA,
            )
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

    finally:
        camera.release()
        cv2.destroyAllWindows()

    return 0
