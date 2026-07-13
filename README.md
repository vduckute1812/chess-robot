# Chess Robot — C++ Vision

Phase 1 vision stack (camera, calibration, board warp, piece occupancy). Full spec: [camera_development.md](camera_development.md).

The previous Python prototype is under [`python/`](python/).

## Environment setup (like Python `venv`)

| Python | C++ |
|--------|-----|
| `python3 -m venv env` | `./scripts/setup_cpp_env.sh` |
| `source env/bin/activate` | `source cpp-env/activate.sh` |
| `pip install -r requirements.txt` | (deps installed into `cpp-env/prefix`) |
| `deactivate` | `deactivate_chess_cpp` |

```bash
# Build tools (once)
sudo apt install build-essential cmake pkg-config git

# OpenCV headers/libs (recommended; or use --from-source below)
sudo apt install libopencv-dev

# Create local C++ env (installs yaml-cpp into cpp-env/; uses system OpenCV)
chmod +x scripts/*.sh
./scripts/setup_cpp_env.sh
source cpp-env/activate.sh

# Build & run
./scripts/run.sh rebuild
./scripts/run.sh list
./scripts/run.sh vision
```

If you cannot install system OpenCV:

```bash
./scripts/setup_cpp_env.sh --from-source   # builds OpenCV into cpp-env (slow)
source cpp-env/activate.sh
./scripts/run.sh rebuild
```

`./scripts/run.sh` auto-sources `cpp-env/activate.sh` when present.

| Command | Binary |
|---------|--------|
| `setup` | create/update `cpp-env` |
| `list` | `list_cameras` |
| `calibrate` | `calibrate_camera` |
| `corners` | `capture_board_corners` |
| `vision` | `vision_main` |
| `pieces` | `detect_pieces` |
| `check-occupancy` | `check_occupancy` (offline empty/piece self-check) |

Piece occupancy: lock corners → `./scripts/run.sh pieces` → clear board → **e** → place pieces.
Offline: `./scripts/run.sh check-occupancy`

Extra args are forwarded (e.g. `./scripts/run.sh vision --config config/camera_config.yaml`).
