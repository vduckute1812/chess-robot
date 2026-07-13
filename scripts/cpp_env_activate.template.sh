# Activate chess-robot C++ environment (like: source env/bin/activate)
# Usage: source cpp-env/activate.sh

_CHESS_ROBOT_ENV_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
export CHESS_ROBOT_ENV="${_CHESS_ROBOT_ENV_ROOT}"
export CHESS_ROBOT_PREFIX="${_CHESS_ROBOT_ENV_ROOT}/prefix"
export CHESS_ROBOT_ENV_MODE="@CHESS_ROBOT_ENV_MODE@"

_chess_prepend() {
  local var="$1"
  local val="$2"
  local cur
  eval "cur=\"\${$var-}\""
  case ":${cur}:" in
    *":${val}:"*) ;;
    *) eval "export ${var}=\"${val}${cur:+:$cur}\"" ;;
  esac
}

_chess_prepend CMAKE_PREFIX_PATH "$CHESS_ROBOT_PREFIX"
_chess_prepend PKG_CONFIG_PATH "$CHESS_ROBOT_PREFIX/lib/pkgconfig"
_chess_prepend PKG_CONFIG_PATH "$CHESS_ROBOT_PREFIX/lib/x86_64-linux-gnu/pkgconfig"
_chess_prepend LD_LIBRARY_PATH "$CHESS_ROBOT_PREFIX/lib"
_chess_prepend LD_LIBRARY_PATH "$CHESS_ROBOT_PREFIX/lib/x86_64-linux-gnu"
_chess_prepend PATH "$CHESS_ROBOT_PREFIX/bin"

export CHESS_ROBOT_USE_SYSROOT=0
unset OpenCV_DIR 2>/dev/null || true

if [[ -f "$CHESS_ROBOT_PREFIX/lib/cmake/opencv4/OpenCVConfig.cmake" ]]; then
  export OpenCV_DIR="$CHESS_ROBOT_PREFIX/lib/cmake/opencv4"
elif [[ -f "$CHESS_ROBOT_PREFIX/lib64/cmake/opencv4/OpenCVConfig.cmake" ]]; then
  export OpenCV_DIR="$CHESS_ROBOT_PREFIX/lib64/cmake/opencv4"
fi

deactivate_chess_cpp() {
  unset CHESS_ROBOT_ENV CHESS_ROBOT_PREFIX CHESS_ROBOT_ENV_MODE OpenCV_DIR
  unset -f deactivate_chess_cpp _chess_prepend 2>/dev/null || true
  echo "Deactivated chess-robot C++ env (restart shell to fully reset PATH/LD_LIBRARY_PATH)."
}

echo "Activated chess-robot C++ env ($CHESS_ROBOT_ENV_MODE)"
echo "  prefix: $CHESS_ROBOT_PREFIX"
echo "  run:    ./scripts/run.sh rebuild && ./scripts/run.sh vision"
echo "  leave:  deactivate_chess_cpp"
