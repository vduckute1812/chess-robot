"""Load and save intrinsic calibration matrices."""

from __future__ import annotations

from pathlib import Path

import numpy as np

from src.config import load_config, load_section, resolve_config_path


def load_calibration_matrices(
    config_path: str | Path | None = None,
) -> tuple[np.ndarray, np.ndarray] | None:
    cal_cfg = load_section("calibration", config_path)
    if not cal_cfg.get("enabled", False):
        return None

    camera_matrix = np.array(cal_cfg["camera_matrix"], dtype=np.float64)
    dist_coeffs = np.array(cal_cfg["dist_coeffs"], dtype=np.float64).reshape(-1, 1)
    return camera_matrix, dist_coeffs


def save_calibration_matrices(
    camera_matrix: np.ndarray,
    dist_coeffs: np.ndarray,
    reprojection_error: float,
    image_size: tuple[int, int],
    config_path: str | Path | None = None,
) -> Path:
    path = resolve_config_path(config_path)
    config = load_config(path)

    config.setdefault("calibration", {})
    config["calibration"]["enabled"] = True
    config["calibration"]["camera_matrix"] = camera_matrix.tolist()
    config["calibration"]["dist_coeffs"] = dist_coeffs.reshape(-1).tolist()
    config["calibration"]["reprojection_error"] = reprojection_error
    config["calibration"]["image_width"] = image_size[0]
    config["calibration"]["image_height"] = image_size[1]

    from src.config import save_config

    return save_config(config, path)
