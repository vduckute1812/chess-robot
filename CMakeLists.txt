cmake_minimum_required(VERSION 3.16)
project(chess_robot VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

# Prefer project-local cpp-env (created by scripts/setup_cpp_env.sh), like Python's env/.
set(_chess_env_prefix "")
if(DEFINED ENV{CHESS_ROBOT_PREFIX} AND EXISTS "$ENV{CHESS_ROBOT_PREFIX}")
  set(_chess_env_prefix "$ENV{CHESS_ROBOT_PREFIX}")
elseif(EXISTS "${CMAKE_SOURCE_DIR}/cpp-env/prefix")
  set(_chess_env_prefix "${CMAKE_SOURCE_DIR}/cpp-env/prefix")
endif()
if(_chess_env_prefix)
  list(PREPEND CMAKE_PREFIX_PATH "${_chess_env_prefix}")
  message(STATUS "Using C++ env prefix: ${_chess_env_prefix}")
endif()

option(CHESS_ROBOT_USE_SYSROOT
  "Use third_party/sysroot OpenCV/yaml-cpp (experimental; prefer cpp-env or system packages)"
  OFF)

set(CHESS_ROBOT_SYSROOT "${CMAKE_SOURCE_DIR}/third_party/sysroot" CACHE PATH
  "Optional local dependency prefix")

# Never let a partial third_party/sysroot hijack find_package(OpenCV).
if(NOT CHESS_ROBOT_USE_SYSROOT)
  if(DEFINED OpenCV_DIR AND OpenCV_DIR MATCHES "third_party/sysroot")
    message(STATUS "Ignoring OpenCV_DIR from incomplete sysroot: ${OpenCV_DIR}")
    unset(OpenCV_DIR CACHE)
    unset(OpenCV_DIR)
  endif()
  if(DEFINED CMAKE_PREFIX_PATH)
    set(_clean_prefix "")
    foreach(_p IN LISTS CMAKE_PREFIX_PATH)
      if(NOT _p MATCHES "third_party/sysroot")
        list(APPEND _clean_prefix "${_p}")
      endif()
    endforeach()
    set(CMAKE_PREFIX_PATH "${_clean_prefix}")
  endif()
  list(APPEND CMAKE_IGNORE_PATH "${CHESS_ROBOT_SYSROOT}")
  list(APPEND CMAKE_IGNORE_PREFIX_PATH "${CHESS_ROBOT_SYSROOT}")
endif()

if(CHESS_ROBOT_USE_SYSROOT AND EXISTS "${CHESS_ROBOT_SYSROOT}/usr/include/opencv4")
  set(OpenCV_INCLUDE_DIRS "${CHESS_ROBOT_SYSROOT}/usr/include/opencv4")
  set(OpenCV_LIB_DIR "${CHESS_ROBOT_SYSROOT}/usr/lib/x86_64-linux-gnu")
  set(OpenCV_LIBS
    ${OpenCV_LIB_DIR}/libopencv_core.so
    ${OpenCV_LIB_DIR}/libopencv_imgproc.so
    ${OpenCV_LIB_DIR}/libopencv_imgcodecs.so
    ${OpenCV_LIB_DIR}/libopencv_calib3d.so
    ${OpenCV_LIB_DIR}/libopencv_features2d.so
    ${OpenCV_LIB_DIR}/libopencv_flann.so
    ${OpenCV_LIB_DIR}/libopencv_highgui.so
    ${OpenCV_LIB_DIR}/libopencv_videoio.so
  )
  message(STATUS "Using OpenCV from sysroot: ${CHESS_ROBOT_SYSROOT}")
else()
  find_package(OpenCV QUIET COMPONENTS core imgproc imgcodecs calib3d highgui videoio)
  if(NOT OpenCV_FOUND)
    message(FATAL_ERROR
      "OpenCV not found.\n"
      "Set up the C++ env (recommended):\n"
      "  ./scripts/setup_cpp_env.sh\n"
      "  source cpp-env/activate.sh\n"
      "  ./scripts/run.sh rebuild\n"
      "Or install system packages:\n"
      "  sudo apt install libopencv-dev libyaml-cpp-dev\n"
      "Do not point OpenCV_DIR at third_party/sysroot unless you pass -DCHESS_ROBOT_USE_SYSROOT=ON.")
  endif()
endif()

find_package(yaml-cpp QUIET)
if(NOT yaml-cpp_FOUND AND CHESS_ROBOT_USE_SYSROOT
   AND EXISTS "${CHESS_ROBOT_SYSROOT}/usr/include/yaml-cpp")
  add_library(yaml-cpp SHARED IMPORTED)
  set_target_properties(yaml-cpp PROPERTIES
    IMPORTED_LOCATION "${CHESS_ROBOT_SYSROOT}/usr/lib/x86_64-linux-gnu/libyaml-cpp.so"
    INTERFACE_INCLUDE_DIRECTORIES "${CHESS_ROBOT_SYSROOT}/usr/include"
  )
  set(YAML_CPP_TARGET yaml-cpp)
  message(STATUS "Using yaml-cpp from sysroot")
elseif(NOT yaml-cpp_FOUND)
  message(STATUS "yaml-cpp not found on system; fetching via FetchContent")
  FetchContent_Declare(
    yaml-cpp
    GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
    GIT_TAG 0.8.0
  )
  set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
  set(YAML_CPP_BUILD_CONTRIB OFF CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(yaml-cpp)
  set(YAML_CPP_TARGET yaml-cpp)
else()
  if(TARGET yaml-cpp::yaml-cpp)
    set(YAML_CPP_TARGET yaml-cpp::yaml-cpp)
  else()
    set(YAML_CPP_TARGET yaml-cpp)
  endif()
endif()

add_library(chess_robot
  src/config.cpp
  src/camera/discovery.cpp
  src/camera/undistort.cpp
  src/camera/capture.cpp
  src/utils/chessboard.cpp
  src/calibration/checkerboard.cpp
  src/calibration/matrices.cpp
  src/calibration/calibrator.cpp
  src/board/corners.cpp
  src/board/detect_chessboard.cpp
  src/board/detect_contour.cpp
  src/board/warp.cpp
  src/board/detector.cpp
  src/pieces/reference.cpp
  src/pieces/classifier.cpp
  src/pieces/occupancy.cpp
)

target_include_directories(chess_robot
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(chess_robot
  PUBLIC
    ${OpenCV_LIBS}
    ${YAML_CPP_TARGET}
)

target_compile_options(chess_robot PRIVATE -Wall -Wextra -Wpedantic)

function(chess_robot_add_app name source)
  add_executable(${name} ${source} apps/common.cpp)
  target_link_libraries(${name} PRIVATE chess_robot)
  target_include_directories(${name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/apps)
  set_target_properties(${name} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/apps)
endfunction()

chess_robot_add_app(vision_main apps/vision_main.cpp)
chess_robot_add_app(calibrate_camera apps/calibrate_camera.cpp)
chess_robot_add_app(capture_board_corners apps/capture_board_corners.cpp)
chess_robot_add_app(list_cameras apps/list_cameras.cpp)
chess_robot_add_app(detect_pieces apps/detect_pieces.cpp)
chess_robot_add_app(check_occupancy apps/check_occupancy.cpp)

message(STATUS "OpenCV include dirs: ${OpenCV_INCLUDE_DIRS}")
