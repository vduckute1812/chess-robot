"""Shared CLI helpers for package entry points."""

from __future__ import annotations

import argparse
from pathlib import Path

from src.config import DEFAULT_CONFIG_PATH


def add_config_argument(parser: argparse.ArgumentParser) -> argparse.ArgumentParser:
    parser.add_argument(
        "--config",
        type=Path,
        default=DEFAULT_CONFIG_PATH,
        help="Path to camera_config.yaml",
    )
    return parser


def parse_config_args(description: str) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=description)
    add_config_argument(parser)
    return parser.parse_args()
