"""Interactive checkerboard calibration workflow."""

from __future__ import annotations

import sys
from pathlib import Path

if __package__ is None:
    sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from src.apps.calibrate import run_calibration_loop
from src.apps.cli import parse_config_args


def main() -> None:
    args = parse_config_args("Calibrate webcam intrinsics with a checkerboard")
    raise SystemExit(run_calibration_loop(args.config))


if __name__ == "__main__":
    main()
