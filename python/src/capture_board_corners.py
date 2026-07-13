"""Interactive board-corner capture workflow."""

from __future__ import annotations

import sys
from pathlib import Path

if __package__ is None:
    sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from src.apps.capture_corners import run_capture_corners_loop
from src.apps.cli import parse_config_args


def main() -> None:
    args = parse_config_args("Capture chessboard corners and save them to config")
    raise SystemExit(run_capture_corners_loop(args.config))


if __name__ == "__main__":
    main()
