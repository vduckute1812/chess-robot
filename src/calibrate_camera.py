"""Interactive checkerboard calibration workflow."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

if __package__ is None:
    sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from src.apps.calibrate import run_calibration_loop
from src.config import DEFAULT_CONFIG_PATH


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Calibrate webcam intrinsics with a checkerboard")
    parser.add_argument(
        "--config",
        type=Path,
        default=DEFAULT_CONFIG_PATH,
        help="Path to camera_config.yaml",
    )
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    raise SystemExit(run_calibration_loop(args.config))


if __name__ == "__main__":
    main()
