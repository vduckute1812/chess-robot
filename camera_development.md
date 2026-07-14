# Project Specification: AI Chess Robot Arm — Phase 1: Vision System (C++)

We are building an AI-powered chess-playing robot arm. This document is the source of truth for **Phase 1: Camera and Vision System**, implemented in **C++**.

---

## Project Overview & Architecture

1. **Vision Module (Current Focus):** USB webcam → calibrate lens → detect board → detect pieces → (next) moves.
2. **AI Chess Engine Module (Future):** Choose the next move from board state.
3. **Robot Arm Control Module (Future):** Execute physical piece moves.

---

## Phase 1: Camera & Vision System

### 1. Technology Stack

* **Language:** C++17
* **Build:** CMake 3.16+
* **Libraries:** OpenCV 4.x (`core`, `imgproc`, `calib3d`, `highgui`, `videoio`), yaml-cpp (system package or FetchContent)
* **Config:** [`config/camera_config.yaml`](config/camera_config.yaml)
* **Optional AI vision (later):** ONNX Runtime / OpenCV DNN for piece classification if classical CV is not enough

### 2. File Structure

```text
chess-robot/
├── CMakeLists.txt
├── config/
│   └── camera_config.yaml
├── include/chess_robot/
│   ├── config.hpp
│   ├── exceptions.hpp
│   ├── camera/
│   │   ├── capture.hpp          # ChessCamera
│   │   ├── discovery.hpp
│   │   └── undistort.hpp
│   ├── calibration/
│   │   ├── calibrator.hpp
│   │   ├── checkerboard.hpp
│   │   └── matrices.hpp
│   ├── board/
│   │   ├── detector.hpp         # BoardDetector facade
│   │   ├── corners.hpp
│   │   ├── detect_chessboard.hpp
│   │   ├── detect_contour.hpp
│   │   └── warp.hpp
│   ├── pieces/
│   │   ├── occupancy.hpp        # PieceOccupancyDetector facade
│   │   └── reference.hpp        # Empty-board capture + center ROI
│   └── utils/
│       └── chessboard.hpp
├── src/                         # Matching .cpp implementations
├── apps/
│   ├── common.hpp / common.cpp  # CLI --config, camera open helper
│   ├── vision_main.cpp
│   ├── calibrate_camera.cpp
│   ├── capture_board_corners.cpp
│   ├── detect_pieces.cpp
│   └── list_cameras.cpp
├── data/
│   └── empty_board.png          # Optional empty-board reference (gitignored)
├── scripts/
│   ├── setup_cpp_env.sh         # Create cpp-env/
│   ├── run.sh                   # Build/run apps (auto-activates cpp-env)
│   └── cpp_env_activate.template.sh
├── cpp-env/                     # Local C++ env (gitignored; after setup)
├── assets/
└── camera_development.md
```

### 3. Class map

| Facade | Role |
|--------|------|
| `chess_robot::ChessCamera` | Device discovery, capture, optional undistort |
| `chess_robot::CameraCalibrator` | Checkerboard samples → intrinsics → YAML |
| `chess_robot::BoardDetector` | Outer corners → warp → 8×8 grid / overlays |
| `chess_robot::PieceOccupancyDetector` | Warped cells → empty / occupied |

---

## Coding Rules

* Prefer RAII (`cv::Mat`, `cv::VideoCapture`, smart pointers); release the camera and destroy windows on exit.
* Use `std::optional` for missing detections; throw `CameraError` / `CalibrationError` / `BoardDetectionError` at domain boundaries.
* Keep YAML keys stable (`camera`, `calibration`, `board`, `detection`, `pieces`).
* Interactive apps must show live `cv::imshow` feedback with the same keybindings as below.
* Shared chessboard corner finding lives in `utils/chessboard`; GUI/CLI helpers in `apps/common.*`.

---

## Build & Quickstart

### C++ environment

```bash
./scripts/setup_cpp_env.sh
source cpp-env/activate.sh
# later: deactivate_chess_cpp
```

```bash
sudo apt install build-essential cmake pkg-config git libopencv-dev

chmod +x scripts/*.sh
./scripts/setup_cpp_env.sh          # installs yaml-cpp into cpp-env/; uses system OpenCV
source cpp-env/activate.sh

./scripts/run.sh rebuild
./build/apps/list_cameras
./scripts/run.sh calibrate
./scripts/run.sh corners
./scripts/run.sh vision
./scripts/run.sh pieces
```

Without system OpenCV: `./scripts/setup_cpp_env.sh --from-source` (builds OpenCV into `cpp-env/`, slower).

`./scripts/run.sh` auto-activates `cpp-env` when present. Binaries also live under `build/apps/`.

### Keyboard shortcuts

| Binary | Key | Action |
|--------|-----|--------|
| `vision_main` | `q` | Quit |
| `vision_main` | `u` | Toggle undistorted / raw |
| `vision_main` | `d` | Toggle corner overlay |
| `vision_main` | `w` | Toggle warped board |
| `vision_main` | `g` | Toggle 8×8 grid |
| `calibrate_camera` | `SPACE` | Capture sample |
| `calibrate_camera` | `c` | Compute & save calibration |
| `calibrate_camera` | `r` | Reset samples |
| `calibrate_camera` | `q` | Quit |
| `capture_board_corners` | `s` | Save corners to `manual_corners` |
| `capture_board_corners` | `c` | Clear `manual_corners` |
| `capture_board_corners` | `q` | Quit |
| `detect_pieces` | `e` | Capture empty-board reference (self-check) |
| `detect_pieces` | `m` | Toggle MAD / changed% debug overlay |
| `detect_pieces` | `p` | Print ASCII occupancy grid |
| `detect_pieces` | `u` | Toggle undistorted / raw |
| `detect_pieces` | `q` | Quit |

---

## Step-by-Step Status

#### Step 1: Camera stream — DONE (C++)

```bash
./build/apps/list_cameras
./build/apps/vision_main
```

#### Step 2: Lens calibration — DONE (C++)

Printed checkerboard with **9×6 inner corners**. Set `square_size_mm` in config.

```bash
./build/apps/calibrate_camera
```

Writes `calibration.camera_matrix`, `dist_coeffs`, etc. Press **u** in `vision_main` to toggle undistortion.

#### Step 3: Board perspective warp — DONE (C++)

* Primary: OpenCV inner corners `(grid_size-1)²` → outer playing-area corners
* Fallback: contour quadrilateral
* Lock with `./build/apps/capture_board_corners` (**s** / **c**)

#### Step 4: Grid segmentation — DONE (C++)

`BoardDetector::segmentGrid` / `drawGridOverlay` on the warped square.

---

#### Step 5: Piece / occupancy detection — DONE (C++, classical)

**Goal:** For each of 64 squares: **empty vs occupied** (side/type later if needed).

**Approach:** classical CV on warped cell center ROIs.

1. Lock board corners (`./scripts/run.sh corners`). Prefer the board filling the camera (≥~360px wide).
2. Clear the board and press **e** (averages 12 frames → high-res `data/empty_board.png`).
3. Detection uses **gradient + mean-normalized** absdiff + changed-pixel ratio + temporal confirm.
4. Stable-empty cells slowly adapt the empty reference (`empty_adapt_*`).
5. Warp at `board.output_size` (default **1600**); move the camera closer for real optical detail.
6. Offline: `./scripts/run.sh check-occupancy`

ASCII: `.` = empty, `X` = occupied.

#### Step 6: Move detection — NEXT

Diff consecutive occupancy grids → UCI moves; use a C++ chess rules library for castling / en passant later.

---

## Calibration vs Board vs Pieces

| | Lens calibration | Board warp | Piece state |
|---|---|---|---|
| **Purpose** | Fix lens distortion | Fix viewing angle | Read 64 squares |
| **Input** | Checkerboard pattern | Playing board | Warped cells |
| **API** | `calibrateCamera` | Homography | Per-square CV (± DNN) |
| **YAML** | `calibration.*` | `board.*` | `pieces.*` |

**Order:** calibrate lens → lock board corners → detect pieces on the warped grid.

---

## Current status

| Step | Status |
|------|--------|
| 1 Camera stream | Done (C++) |
| 2 Lens calibration | Done (C++) |
| 3 Board warp | Done (C++) |
| 4 Grid segmentation | Done (C++) |
| 5 Piece / occupancy detection | Done (classical; types later) |
| 6 Move detection | **Next** |
