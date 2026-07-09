"""List available camera devices."""

from __future__ import annotations

import sys

from src.camera import ChessCamera


def run_list_cameras() -> int:
    devices = ChessCamera.list_devices()
    if not devices:
        print("No working cameras found.")
        print("Tips:")
        print("  - Plug in the USB webcam")
        print("  - Close other apps using the camera")
        print("  - Add your user to the video group: sudo usermod -aG video $USER")
        return 1

    print("Working cameras:")
    for device in devices:
        print(f"  {device['path']}: {device['resolution']}")
    return 0


def main() -> None:
    raise SystemExit(run_list_cameras())


if __name__ == "__main__":
    main()
