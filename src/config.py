"""Configuration loading and persistence."""

from __future__ import annotations

from pathlib import Path
from typing import Any

import yaml

DEFAULT_CONFIG_PATH = Path(__file__).resolve().parent.parent / "config" / "camera_config.yaml"


def resolve_config_path(config_path: str | Path | None = None) -> Path:
    if config_path is None:
        return DEFAULT_CONFIG_PATH
    return Path(config_path)


def load_config(config_path: str | Path | None = None) -> dict[str, Any]:
    path = resolve_config_path(config_path)
    if not path.exists():
        raise FileNotFoundError(f"Camera config not found: {path}")

    with path.open("r", encoding="utf-8") as handle:
        data = yaml.safe_load(handle)

    return data or {}


def load_section(section: str, config_path: str | Path | None = None) -> dict[str, Any]:
    return load_config(config_path).get(section, {})


def save_config(data: dict[str, Any], config_path: str | Path | None = None) -> Path:
    path = resolve_config_path(config_path)
    with path.open("w", encoding="utf-8") as handle:
        yaml.safe_dump(data, handle, sort_keys=False, default_flow_style=False)
    return path
