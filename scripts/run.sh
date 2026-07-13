#!/usr/bin/env bash
# Build (if needed) and run chess-robot C++ apps from the repo root.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

# Auto-activate project C++ env (like working inside a Python venv).
if [[ -z "${CHESS_ROBOT_ENV:-}" && -f "$ROOT/cpp-env/activate.sh" ]]; then
  # shellcheck disable=SC1091
  source "$ROOT/cpp-env/activate.sh"
fi

BUILD_DIR="${BUILD_DIR:-$ROOT/build}"
APP_DIR="$BUILD_DIR/apps"
CONFIG="${CONFIG:-$ROOT/config/camera_config.yaml}"
JOBS="${JOBS:-$(nproc 2>/dev/null || echo 4)}"
USE_SYSROOT="${CHESS_ROBOT_USE_SYSROOT:-0}"

usage() {
  cat <<EOF
Usage: $(basename "$0") <command> [args...]

Commands:
  setup                 Create/update cpp-env (./scripts/setup_cpp_env.sh)
  build                 Configure + build (Release)
  rebuild               Wipe build/ then configure + build
  list | list_cameras   List working cameras
  vision | main         Live vision loop (board warp + grid)
  calibrate             Checkerboard lens calibration
  corners | capture     Capture/save board.manual_corners
  pieces | detect       Piece occupancy on warped board
  check-occupancy       Offline C++ empty/piece self-check (no camera)
  help                  Show this help

Environment:
  BUILD_DIR               Build directory (default: ./build)
  CONFIG                  Default --config path (default: ./config/camera_config.yaml)
  JOBS                    Parallel build jobs (default: nproc)
  CHESS_ROBOT_USE_SYSROOT Set to 1 to link against third_party/sysroot (experimental)

Examples:
  $(basename "$0") setup
  $(basename "$0") rebuild
  $(basename "$0") vision
  $(basename "$0") pieces
EOF
}

deps_hint() {
  cat >&2 <<EOF
Set up the C++ environment (like Python venv), then rebuild:
  ./scripts/setup_cpp_env.sh
  source cpp-env/activate.sh
  ./scripts/run.sh rebuild

Or install system packages:
  sudo apt install build-essential cmake pkg-config libopencv-dev libyaml-cpp-dev
  ./scripts/run.sh rebuild
EOF
}

sanitize_opencv_env() {
  if [[ "${OpenCV_DIR:-}" == *third_party/sysroot* ]]; then
    unset OpenCV_DIR
  fi
  if [[ "${CMAKE_PREFIX_PATH:-}" == *third_party/sysroot* ]]; then
    local cleaned=""
    local IFS=':'
    for part in $CMAKE_PREFIX_PATH; do
      if [[ "$part" != *third_party/sysroot* ]]; then
        cleaned="${cleaned:+$cleaned:}$part"
      fi
    done
    export CMAKE_PREFIX_PATH="$cleaned"
  fi
}

configure_cmake() {
  sanitize_opencv_env
  local extra=()
  if [[ "$USE_SYSROOT" == "1" ]]; then
    extra+=(-DCHESS_ROBOT_USE_SYSROOT=ON)
  else
    extra+=(-DCHESS_ROBOT_USE_SYSROOT=OFF)
    extra+=(-UOpenCV_DIR)
    # Keep CMAKE_PREFIX_PATH from cpp-env activate; only drop sysroot.
  fi

  echo "Configuring CMake in $BUILD_DIR ..."
  if [[ -n "${CHESS_ROBOT_PREFIX:-}" ]]; then
    echo "  CHESS_ROBOT_PREFIX=$CHESS_ROBOT_PREFIX"
  fi
  if ! cmake -S "$ROOT" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release "${extra[@]}"; then
    deps_hint
    return 1
  fi
}

ensure_build() {
  local need_configure=0
  if [[ ! -f "$BUILD_DIR/CMakeCache.txt" ]]; then
    need_configure=1
  elif grep -q 'third_party/sysroot' "$BUILD_DIR/CMakeCache.txt" 2>/dev/null \
    && [[ "$USE_SYSROOT" != "1" ]]; then
    echo "Build cache points at incomplete third_party/sysroot; reconfiguring..."
    need_configure=1
  fi

  if [[ $need_configure -eq 1 ]]; then
    configure_cmake
  fi

  echo "Building..."
  if ! cmake --build "$BUILD_DIR" -j"$JOBS"; then
    deps_hint
    return 1
  fi
}

rebuild() {
  echo "Removing $BUILD_DIR ..."
  rm -rf "$BUILD_DIR"
  ensure_build
}

run_app() {
  local name="$1"
  shift
  local bin="$APP_DIR/$name"
  if [[ ! -x "$bin" ]]; then
    ensure_build
  fi
  if [[ ! -x "$bin" ]]; then
    echo "error: binary not found: $bin" >&2
    deps_hint
    exit 1
  fi

  local args=("$@")
  if [[ "$name" != "list_cameras" ]]; then
    local has_config=0
    for arg in "${args[@]+"${args[@]}"}"; do
      if [[ "$arg" == "--config" ]]; then
        has_config=1
        break
      fi
    done
    if [[ $has_config -eq 0 ]]; then
      args=(--config "$CONFIG" "${args[@]+"${args[@]}"}")
    fi
  fi

  echo "Running: $bin ${args[*]-}"
  exec "$bin" "${args[@]+"${args[@]}"}"
}

cmd="${1:-help}"
shift || true

case "$cmd" in
  setup)
    exec "$ROOT/scripts/setup_cpp_env.sh" "$@"
    ;;
  build)
    ensure_build
    echo "Binaries in $APP_DIR"
    ;;
  rebuild)
    rebuild
    echo "Binaries in $APP_DIR"
    ;;
  list|list_cameras)
    run_app list_cameras "$@"
    ;;
  vision|main|vision_main)
    run_app vision_main "$@"
    ;;
  calibrate|calibrate_camera)
    run_app calibrate_camera "$@"
    ;;
  corners|capture|capture_board_corners)
    run_app capture_board_corners "$@"
    ;;
  pieces|detect|detect_pieces)
    run_app detect_pieces "$@"
    ;;
  check-occupancy|check_occupancy)
    run_app check_occupancy "$@"
    ;;
  -h|--help|help)
    usage
    ;;
  *)
    echo "Unknown command: $cmd" >&2
    usage >&2
    exit 2
    ;;
esac
