# Project Specification: AI Chess Robot Arm — Phase 1: Vision System

You are an expert embedded systems, computer vision, and robotics AI assistant. We are building an AI-powered chess-playing robot arm. This document outlines the overall architecture, with an immediate, strict focus on **Phase 1: Camera and Vision System**.

---

## 🛠️ Project Overview & Architecture

The project consists of three main modules:

1. **Vision Module (Current Focus):** Uses a standard webcam to capture the chessboard, detect pieces, and identify board positions/moves.
2. **AI Chess Engine Module (Future):** Analyzes the board state and determines the optimal next move.
3. **Robot Arm Control Module (Future):** Translates the chosen move into physical coordinate commands to move the chess pieces.

---

## 📸 Phase 1: Camera & Vision System Requirements

We are using a standard **USB Webcam** positioned above the chessboard. The immediate goal is to successfully stream video, calibrate the lens, detect the board, and track piece positions.

### 1. Technology Stack

* **Language:** Python 3.10+
* **Core Libraries:** `opencv-python`, `numpy`, `PyYAML`
* **Optional AI Vision (For later):** `ultralytics` (YOLOv8/v11 for piece detection if traditional CV isn't enough).

### 2. File Structure

```text
chess-robot/
├── config/
│   └── camera_config.yaml
├── src/
│   ├── __init__.py              # Public API exports
│   ├── config.py                # YAML load/save helpers
│   ├── exceptions.py            # Shared exception types
│   ├── camera/
│   │   ├── capture.py           # ChessCamera
│   │   ├── discovery.py         # Device probing and listing
│   │   └── undistort.py         # Lens correction
│   ├── calibration/
│   │   ├── calibrator.py        # CameraCalibrator
│   │   ├── checkerboard.py      # Checkerboard corner detection
│   │   └── matrices.py          # Save/load intrinsic matrices
│   ├── board/
│   │   ├── detector.py          # BoardDetector
│   │   └── geometry.py          # Warp, grid split, corner math
│   ├── apps/
│   │   ├── vision.py            # Main vision test loop
│   │   ├── calibrate.py         # Calibration workflow loop
│   │   └── list_cameras.py      # Device listing CLI
│   ├── main.py                  # Entry point: python -m src.main
│   ├── calibrate_camera.py      # Entry point: python -m src.calibrate_camera
│   └── list_cameras.py          # Entry point: python -m src.list_cameras
└── requirements.txt
```

---

## 🤖 AI Coding Rules & Instructions

When generating code or helping me debug the **Vision Module**, adhere to these guidelines:

### Code Quality & Style

* **Object-Oriented:** Structure the vision system using clean Python classes (e.g., `ChessCamera`, `CameraCalibrator`, `BoardDetector`).
* **Error Handling:** Always include robust error handling for hardware connections (e.g., if the webcam is unplugged or fails to open).
* **Performance:** Use efficient OpenCV operations. Avoid heavy processing on the main UI/capture thread where possible.
* **Visualization:** When writing test scripts, always include an OpenCV display window (`cv2.imshow`) so I can see the visual output in real-time.

### Step-by-Step Implementation Strategy

Tackle the vision pipeline in this order:

#### Step 1: Simple Camera Stream

Establish a reliable feed from the webcam, handle resolution settings, and gracefully exit on a keypress (e.g., pressing `q`).

```bash
python -m src.main
```

#### Step 2: Camera Intrinsic Calibration (Lens Undistortion)

**Why:** Webcam lenses introduce barrel/pincushion distortion. Correcting this before board detection improves corner accuracy and makes square sizes more uniform across the image.

**What to use:** A printed **checkerboard calibration pattern** — not the chess playing board. The default config expects a board with **9×6 inner corners** (10×7 squares). Print it flat and measure one square side length in millimeters for `square_size_mm` in `config/camera_config.yaml`.

**Workflow:**

1. Set your webcam resolution in `config/camera_config.yaml` (calibration must match the resolution used at runtime).
2. Run the calibration tool:

   ```bash
   python -m src.calibrate_camera
   ```

3. Hold the checkerboard in view and move it to different positions/angles.
4. Press **SPACE** each time corners are detected (collect at least 15 samples).
5. Press **c** to compute and save `camera_matrix` and `dist_coeffs` into the config.
6. Press **r** to reset samples, **q** to quit without saving.

**Output:** `config/camera_config.yaml` is updated with:

* `calibration.enabled: true`
* `camera_matrix` — 3×3 intrinsic matrix
* `dist_coeffs` — lens distortion coefficients
* `reprojection_error` — lower is better (aim for &lt; 1.0 pixel)

`ChessCamera.read()` applies undistortion automatically when calibration is enabled. In the main vision loop, press **u** to toggle undistorted vs raw frames.

#### Step 3: Board Perspective Warp

Isolate the playing board by detecting its four outer corners and warping to a top-down square view (homography). This is separate from lens calibration — it corrects **perspective**, not lens distortion.

* Automatic detection via edge/contour finding
* Manual fallback: set `board.manual_corners` in the config

#### Step 4: Grid Segmentation

Divide the warped top-down image into 64 equal squares to prepare for piece/move detection.

---

## ⚡ Quickstart

```bash
# Create and activate the project virtual environment
python3 -m venv env
source env/bin/activate
pip install -r requirements.txt

# 1. Test live camera feed
python -m src.main

# 2. Calibrate lens (print checkerboard first)
python -m src.calibrate_camera

# 3. Test full pipeline with undistortion + board warp + grid
python -m src.main
```

Without activating the venv, use `env/bin/python` directly:

```bash
env/bin/python -m src.main
```

### Keyboard shortcuts

| Script | Key | Action |
|--------|-----|--------|
| `src.main` | `q` | Quit |
| `src.main` | `u` | Toggle undistorted / raw camera |
| `src.main` | `d` | Toggle board corner overlay |
| `src.main` | `w` | Toggle warped board view |
| `src.main` | `g` | Toggle 8×8 grid overlay |
| `src.calibrate_camera` | `SPACE` | Capture calibration sample |
| `src.calibrate_camera` | `c` | Compute and save calibration |
| `src.calibrate_camera` | `r` | Reset samples |
| `src.calibrate_camera` | `q` | Quit |

---

## 📋 Calibration vs Board Detection

| | Lens calibration | Board warp |
|---|---|---|
| **Purpose** | Fix lens distortion | Fix viewing angle |
| **Input** | Checkerboard pattern | Chess playing board |
| **Method** | `cv2.calibrateCamera` | Homography / 4-point warp |
| **Stored in** | `calibration.*` | `board.*` |
| **When** | Once per camera + resolution | Every frame (or manual corners) |

Run lens calibration **before** relying on automatic board corner detection for best results.
