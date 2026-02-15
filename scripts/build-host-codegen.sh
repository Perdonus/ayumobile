#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

if [[ -z "${QT_HOST_PATH:-}" ]]; then
  echo "QT_HOST_PATH is required." >&2
  exit 1
fi

ensure_opengl_dev_packages() {
  if [[ -f /usr/include/GL/gl.h ]] && [[ -f /usr/lib/x86_64-linux-gnu/libOpenGL.so ]]; then
    return
  fi

  export DEBIAN_FRONTEND=noninteractive
  apt-get update
  apt-get install -y --no-install-recommends \
    libgl1-mesa-dev \
    libopengl-dev \
    libglx-dev \
    mesa-common-dev
}

HOST_CODEGEN_DIR="${HOST_CODEGEN_DIR:-/content/host-codegen}"
HOST_CODEGEN_SRC_DIR="${HOST_CODEGEN_DIR}/src"
HOST_CODEGEN_BUILD_DIR="${HOST_CODEGEN_DIR}/build"
HOST_CODEGEN_BIN_DIR="${HOST_CODEGEN_DIR}/bin"

ensure_opengl_dev_packages
mkdir -p "${HOST_CODEGEN_SRC_DIR}" "${HOST_CODEGEN_BUILD_DIR}" "${HOST_CODEGEN_BIN_DIR}"

cat > "${HOST_CODEGEN_SRC_DIR}/CMakeLists.txt" <<'CMAKE_EOF'
cmake_minimum_required(VERSION 3.25)
project(tdesktop_host_codegen LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

if (NOT TELEGRAM_ROOT)
  message(FATAL_ERROR "TELEGRAM_ROOT is required")
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Gui Svg)

set(CODEGEN_ROOT "${TELEGRAM_ROOT}/codegen")
set(LIB_BASE_ROOT "${TELEGRAM_ROOT}/lib_base")

add_library(codegen_common STATIC
  ${CODEGEN_ROOT}/codegen/common/basic_tokenized_file.cpp
  ${CODEGEN_ROOT}/codegen/common/checked_utf8_string.cpp
  ${CODEGEN_ROOT}/codegen/common/clean_file.cpp
  ${CODEGEN_ROOT}/codegen/common/cpp_file.cpp
  ${CODEGEN_ROOT}/codegen/common/logging.cpp
)
target_include_directories(codegen_common PUBLIC ${CODEGEN_ROOT} ${LIB_BASE_ROOT})
target_link_libraries(codegen_common PUBLIC Qt6::Core)

add_executable(codegen_lang
  ${CODEGEN_ROOT}/codegen/lang/generator.cpp
  ${CODEGEN_ROOT}/codegen/lang/main.cpp
  ${CODEGEN_ROOT}/codegen/lang/options.cpp
  ${CODEGEN_ROOT}/codegen/lang/parsed_file.cpp
  ${CODEGEN_ROOT}/codegen/lang/processor.cpp
)
target_include_directories(codegen_lang PRIVATE ${CODEGEN_ROOT} ${LIB_BASE_ROOT})
target_link_libraries(codegen_lang PRIVATE codegen_common Qt6::Core Qt6::Gui)

add_executable(codegen_style
  ${CODEGEN_ROOT}/codegen/style/generator.cpp
  ${CODEGEN_ROOT}/codegen/style/main.cpp
  ${CODEGEN_ROOT}/codegen/style/module.cpp
  ${CODEGEN_ROOT}/codegen/style/options.cpp
  ${CODEGEN_ROOT}/codegen/style/parsed_file.cpp
  ${CODEGEN_ROOT}/codegen/style/processor.cpp
  ${CODEGEN_ROOT}/codegen/style/structure_types.cpp
  ${LIB_BASE_ROOT}/base/crc32hash.cpp
)
target_include_directories(codegen_style PRIVATE ${CODEGEN_ROOT} ${LIB_BASE_ROOT})
target_link_libraries(codegen_style PRIVATE codegen_common Qt6::Core Qt6::Gui Qt6::Svg)

add_executable(codegen_emoji
  ${CODEGEN_ROOT}/codegen/emoji/data.cpp
  ${CODEGEN_ROOT}/codegen/emoji/data_old.cpp
  ${CODEGEN_ROOT}/codegen/emoji/data_read.cpp
  ${CODEGEN_ROOT}/codegen/emoji/generator.cpp
  ${CODEGEN_ROOT}/codegen/emoji/main.cpp
  ${CODEGEN_ROOT}/codegen/emoji/options.cpp
  ${CODEGEN_ROOT}/codegen/emoji/replaces.cpp
)
target_include_directories(codegen_emoji PRIVATE ${CODEGEN_ROOT} ${LIB_BASE_ROOT})
target_link_libraries(codegen_emoji PRIVATE codegen_common Qt6::Core Qt6::Gui)
CMAKE_EOF

cmake -S "${HOST_CODEGEN_SRC_DIR}" -B "${HOST_CODEGEN_BUILD_DIR}" -GNinja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="${QT_HOST_PATH}" \
  -DTELEGRAM_ROOT="${ROOT_DIR}/Telegram"

cmake --build "${HOST_CODEGEN_BUILD_DIR}" --target codegen_emoji codegen_lang codegen_style -j"$(nproc)"

cp "${HOST_CODEGEN_BUILD_DIR}/codegen_emoji" "${HOST_CODEGEN_BIN_DIR}/codegen_emoji"
cp "${HOST_CODEGEN_BUILD_DIR}/codegen_lang" "${HOST_CODEGEN_BIN_DIR}/codegen_lang"
cp "${HOST_CODEGEN_BUILD_DIR}/codegen_style" "${HOST_CODEGEN_BIN_DIR}/codegen_style"

echo "[HOST CODEGEN] ready in ${HOST_CODEGEN_BIN_DIR}"
