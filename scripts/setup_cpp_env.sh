#!/usr/bin/env bash
# Create a project-local C++ environment.
#
#   ./scripts/setup_cpp_env.sh
#   source cpp-env/activate.sh
#   ./scripts/run.sh rebuild
#
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ENV_DIR="${CHESS_ROBOT_ENV_DIR:-$ROOT/cpp-env}"
PREFIX="$ENV_DIR/prefix"
SRC_DIR="$ENV_DIR/src"
TEMPLATE="$ROOT/scripts/cpp_env_activate.template.sh"
JOBS="${JOBS:-$(nproc 2>/dev/null || echo 4)}"
FROM_SOURCE=0
FORCE=0

usage() {
  cat <<EOF
Usage: $(basename "$0") [options]

Create cpp-env/ — a local C++ toolchain prefix.

Options:
  --from-source   Build OpenCV into cpp-env (slow; if apt OpenCV is unavailable)
  --force         Rebuild dependencies even if already installed
  -h, --help      Show help

After setup:
  source cpp-env/activate.sh
  ./scripts/run.sh rebuild
  ./scripts/run.sh vision
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --from-source) FROM_SOURCE=1 ;;
    --force) FORCE=1 ;;
    -h|--help) usage; exit 0 ;;
    *) echo "Unknown option: $1" >&2; usage >&2; exit 2 ;;
  esac
  shift
done

need_cmd() {
  if ! command -v "$1" >/dev/null 2>&1; then
    echo "error: missing command '$1'" >&2
    echo "Install build tools:" >&2
    echo "  sudo apt install build-essential cmake pkg-config git" >&2
    exit 1
  fi
}

need_cmd cmake
need_cmd g++
need_cmd pkg-config
need_cmd git

mkdir -p "$PREFIX" "$SRC_DIR"
echo "==> C++ env directory: $ENV_DIR"
echo "==> Install prefix:    $PREFIX"

install_yaml_cpp() {
  local marker="$ENV_DIR/.yaml-cpp-installed"
  if [[ -f "$marker" && $FORCE -eq 0 && -f "$PREFIX/lib/cmake/yaml-cpp/yaml-cpp-config.cmake" ]]; then
    echo "==> yaml-cpp already installed in cpp-env"
    return
  fi

  echo "==> Installing yaml-cpp into cpp-env ..."
  local src="$SRC_DIR/yaml-cpp"
  if [[ ! -d "$src/.git" ]]; then
    rm -rf "$src"
    git clone --depth 1 --branch 0.8.0 https://github.com/jbeder/yaml-cpp.git "$src"
  fi
  cmake -S "$src" -B "$src/build" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$PREFIX" \
    -DYAML_CPP_BUILD_TESTS=OFF \
    -DYAML_CPP_BUILD_TOOLS=OFF \
    -DYAML_CPP_BUILD_CONTRIB=OFF
  cmake --build "$src/build" -j"$JOBS"
  cmake --install "$src/build"
  touch "$marker"
  echo "==> yaml-cpp installed"
}

find_system_opencv() {
  pkg-config --exists opencv4 2>/dev/null && return 0
  [[ -f /usr/lib/x86_64-linux-gnu/cmake/opencv4/OpenCVConfig.cmake ]] && return 0
  [[ -f /usr/local/lib/cmake/opencv4/OpenCVConfig.cmake ]] && return 0
  return 1
}

install_opencv_from_source() {
  local marker="$ENV_DIR/.opencv-installed"
  if [[ -f "$marker" && $FORCE -eq 0 ]]; then
    if [[ -f "$PREFIX/lib/cmake/opencv4/OpenCVConfig.cmake" ||
          -f "$PREFIX/lib64/cmake/opencv4/OpenCVConfig.cmake" ]]; then
      echo "==> OpenCV already installed in cpp-env"
      return
    fi
  fi

  echo "==> Building OpenCV into cpp-env (this can take a while) ..."
  local ver="4.10.0"
  local src="$SRC_DIR/opencv"
  if [[ ! -d "$src/.git" ]]; then
    rm -rf "$src"
    git clone --depth 1 --branch "$ver" https://github.com/opencv/opencv.git "$src"
  fi

  cmake -S "$src" -B "$src/build" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$PREFIX" \
    -DBUILD_LIST=core,imgproc,imgcodecs,videoio,highgui,calib3d,features2d,flann \
    -DBUILD_TESTS=OFF \
    -DBUILD_PERF_TESTS=OFF \
    -DBUILD_EXAMPLES=OFF \
    -DBUILD_opencv_apps=OFF \
    -DWITH_QT=OFF \
    -DWITH_GTK=ON \
    -DWITH_FFMPEG=ON \
    -DWITH_CUDA=OFF \
    -DOPENCV_GENERATE_PKGCONFIG=ON
  cmake --build "$src/build" -j"$JOBS"
  cmake --install "$src/build"
  touch "$marker"
  echo "==> OpenCV installed into cpp-env"
}

write_activate() {
  local mode="$1"
  sed "s|@CHESS_ROBOT_ENV_MODE@|${mode}|g" "$TEMPLATE" > "$ENV_DIR/activate.sh"
}

install_yaml_cpp

OPENCV_MODE="system"
if [[ $FROM_SOURCE -eq 1 ]]; then
  install_opencv_from_source
  OPENCV_MODE="local"
elif find_system_opencv; then
  echo "==> Using system OpenCV (apt/local install)"
  OPENCV_MODE="system"
else
  echo "==> System OpenCV not found."
  echo "    Option A (recommended): sudo apt install libopencv-dev"
  echo "    Option B: ./scripts/setup_cpp_env.sh --from-source"
  echo ""
  echo "Continuing with yaml-cpp in cpp-env only; install OpenCV then re-run setup or rebuild."
  OPENCV_MODE="partial"
fi

write_activate "$OPENCV_MODE"

cat > "$ENV_DIR/README.md" <<EOF
# cpp-env

Local C++ environment for chess-robot.

\`\`\`bash
source cpp-env/activate.sh
./scripts/run.sh rebuild
./scripts/run.sh vision
deactivate_chess_cpp
\`\`\`

Mode: **${OPENCV_MODE}**
- \`system\` — OpenCV from apt/OS; yaml-cpp in this prefix
- \`local\` — OpenCV + yaml-cpp built into \`prefix/\`
- \`partial\` — yaml-cpp only; install OpenCV then rebuild
EOF

echo ""
echo "Setup complete (${OPENCV_MODE})."
echo "  source ${ENV_DIR}/activate.sh"
echo "  ./scripts/run.sh rebuild"
