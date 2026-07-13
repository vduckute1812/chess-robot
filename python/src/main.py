"""Main entry point for testing the Phase 1 vision system."""

from __future__ import annotations

import sys
from pathlib import Path

if __package__ is None:
    sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from src.apps.cli import parse_config_args
from src.apps.vision import run_vision_loop


def main() -> None:
    args = parse_config_args("Chess robot vision system test loop")
    raise SystemExit(run_vision_loop(args.config))


if __name__ == "__main__":
    main()
