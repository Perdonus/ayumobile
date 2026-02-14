#!/usr/bin/env bash
set -euxo pipefail

: "${ANDROID_NDK_ROOT:?ANDROID_NDK_ROOT is not set}"

DEPS_ROOT="${DEPS_ROOT:-/content/android-deps}"
SRC_ROOT="${SRC_ROOT:-/content/android-deps-src}"
ANDROID_API="${ANDROID_API:-29}"
NPROC="${NPROC:-$(nproc)}"

PREFIX="${DEPS_ROOT}/prefix-arm64"
FFMPEG_PREFIX="${DEPS_ROOT}/ffmpeg-android-arm64"

TOOLCHAIN="${ANDROID_NDK_ROOT}/toolchains/llvm/prebuilt/linux-x86_64"
SYSROOT="${TOOLCHAIN}/sysroot"
TARGET_HOST="aarch64-linux-android"
CC="${TOOLCHAIN}/bin/${TARGET_HOST}${ANDROID_API}-clang"
CXX="${TOOLCHAIN}/bin/${TARGET_HOST}${ANDROID_API}-clang++"
AR="${TOOLCHAIN}/bin/llvm-ar"
RANLIB="${TOOLCHAIN}/bin/llvm-ranlib"
STRIP="${TOOLCHAIN}/bin/llvm-strip"
NM="${TOOLCHAIN}/bin/llvm-nm"

WRAP_DIR="${DEPS_ROOT}/android-toolchain-wrap/bin"

git_no_proxy() {
  env -u HTTP_PROXY -u HTTPS_PROXY -u ALL_PROXY -u http_proxy -u https_proxy -u all_proxy git "$@"
}

msg() {
  echo
  echo "[DEPS] $*"
}

ensure_repo() {
  local name="$1"
  local url="$2"
  local commit="$3"
  local with_submodules="${4:-0}"
  local repo_dir="${SRC_ROOT}/${name}"

  if [ ! -d "${repo_dir}/.git" ]; then
    msg "clone ${name}"
    git_no_proxy clone "${url}" "${repo_dir}"
  fi

  msg "checkout ${name} @ ${commit}"
  git_no_proxy -C "${repo_dir}" fetch --tags --force origin
  git_no_proxy -C "${repo_dir}" checkout -f "${commit}"

  if [ "${with_submodules}" = "1" ]; then
    git_no_proxy -C "${repo_dir}" submodule sync --recursive
    git_no_proxy -C "${repo_dir}" submodule update --init --recursive
  fi
}

prepare_toolchain_wrappers() {
  mkdir -p "${WRAP_DIR}"
  cat > "${WRAP_DIR}/aarch64-linux-android-gcc" <<EOF
#!/usr/bin/env bash
exec "${CC}" "\$@"
EOF
  cat > "${WRAP_DIR}/aarch64-linux-android-g++" <<EOF
#!/usr/bin/env bash
exec "${CXX}" "\$@"
EOF
  chmod +x "${WRAP_DIR}/aarch64-linux-android-gcc" "${WRAP_DIR}/aarch64-linux-android-g++"
}

build_libjpeg() {
  if [ -f "${PREFIX}/lib/libjpeg.a" ] && [ -f "${PREFIX}/lib/pkgconfig/libjpeg.pc" ]; then
    msg "libjpeg-turbo already built"
    return
  fi
  ensure_repo "libjpeg-turbo" "https://github.com/libjpeg-turbo/libjpeg-turbo.git" "7fa4b5b762c9a99b46b0b7838f5fd55071b92ea5"
  rm -rf "${SRC_ROOT}/libjpeg-turbo/build-android-arm64"
  cmake -S "${SRC_ROOT}/libjpeg-turbo" -B "${SRC_ROOT}/libjpeg-turbo/build-android-arm64" -GNinja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE="${ANDROID_NDK_ROOT}/build/cmake/android.toolchain.cmake" \
    -DANDROID_ABI=arm64-v8a \
    -DANDROID_PLATFORM="${ANDROID_API}" \
    -DCMAKE_INSTALL_PREFIX="${PREFIX}" \
    -DENABLE_SHARED=OFF \
    -DENABLE_STATIC=ON \
    -DWITH_JAVA=OFF \
    -DWITH_TURBOJPEG=ON
  cmake --build "${SRC_ROOT}/libjpeg-turbo/build-android-arm64" -j"${NPROC}"
  cmake --install "${SRC_ROOT}/libjpeg-turbo/build-android-arm64"
}

build_opus() {
  if [ -f "${PREFIX}/lib/libopus.a" ] && [ -f "${PREFIX}/lib/pkgconfig/opus.pc" ]; then
    msg "opus already built"
    return
  fi
  ensure_repo "opus" "https://github.com/xiph/opus.git" "ddbe48383984d56acd9e1ab6a090c54ca6b735a6"
  rm -rf "${SRC_ROOT}/opus/build-android-arm64"
  cmake -S "${SRC_ROOT}/opus" -B "${SRC_ROOT}/opus/build-android-arm64" -GNinja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE="${ANDROID_NDK_ROOT}/build/cmake/android.toolchain.cmake" \
    -DANDROID_ABI=arm64-v8a \
    -DANDROID_PLATFORM="${ANDROID_API}" \
    -DCMAKE_INSTALL_PREFIX="${PREFIX}" \
    -DBUILD_SHARED_LIBS=OFF \
    -DOPUS_BUILD_PROGRAMS=OFF \
    -DOPUS_BUILD_SHARED_LIBRARY=OFF \
    -DOPUS_BUILD_TESTING=OFF \
    -DOPUS_INSTALL_CMAKE_CONFIG_MODULE=ON \
    -DOPUS_INSTALL_PKG_CONFIG_MODULE=ON
  cmake --build "${SRC_ROOT}/opus/build-android-arm64" -j"${NPROC}"
  cmake --install "${SRC_ROOT}/opus/build-android-arm64"
}

build_openal() {
  if [ -f "${PREFIX}/lib/libopenal.a" ] && [ -f "${PREFIX}/lib/cmake/OpenAL/OpenALConfig.cmake" ]; then
    msg "openal-soft already built"
    return
  fi
  ensure_repo "openal-soft" "https://github.com/telegramdesktop/openal-soft.git" "9c136bb3b4e7651cd67c1661c31ac7ed730c4646"
  rm -rf "${SRC_ROOT}/openal-soft/build-android-arm64"
  cmake -S "${SRC_ROOT}/openal-soft" -B "${SRC_ROOT}/openal-soft/build-android-arm64" -GNinja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE="${ANDROID_NDK_ROOT}/build/cmake/android.toolchain.cmake" \
    -DANDROID_ABI=arm64-v8a \
    -DANDROID_PLATFORM="${ANDROID_API}" \
    -DCMAKE_INSTALL_PREFIX="${PREFIX}" \
    -DLIBTYPE=STATIC \
    -DALSOFT_TESTS=OFF \
    -DALSOFT_UTILS=OFF \
    -DALSOFT_EXAMPLES=OFF \
    -DALSOFT_NO_CONFIG_UTIL=ON \
    -DALSOFT_BACKEND_OPENSL=ON
  cmake --build "${SRC_ROOT}/openal-soft/build-android-arm64" -j"${NPROC}"
  cmake --install "${SRC_ROOT}/openal-soft/build-android-arm64"
}

build_openh264() {
  if [ -f "${PREFIX}/lib/libopenh264.a" ] && [ -f "${PREFIX}/lib/pkgconfig/openh264.pc" ]; then
    msg "openh264 already built"
    return
  fi
  ensure_repo "openh264" "https://github.com/cisco/openh264.git" "8c7008aeb6335e7d36ab0d9a023a63f82a8eaac0"
  make -C "${SRC_ROOT}/openh264" clean || true
  make -C "${SRC_ROOT}/openh264" \
    OS=android ARCH=arm64 \
    NDKROOT="${ANDROID_NDK_ROOT}" TARGET="android-${ANDROID_API}" \
    -j"${NPROC}" libraries
  make -C "${SRC_ROOT}/openh264" \
    OS=android ARCH=arm64 \
    NDKROOT="${ANDROID_NDK_ROOT}" TARGET="android-${ANDROID_API}" \
    PREFIX="${PREFIX}" install-static
}

build_libvpx() {
  if [ -f "${PREFIX}/lib/libvpx.a" ] && [ -f "${PREFIX}/lib/pkgconfig/vpx.pc" ]; then
    msg "libvpx already built"
    return
  fi
  ensure_repo "libvpx" "https://chromium.googlesource.com/webm/libvpx" "12f3a2ac603e8f10742105519e0cd03c3b8f71dd"
  (
    cd "${SRC_ROOT}/libvpx"
    make clean || true
    make distclean || true
    PATH="${WRAP_DIR}:${TOOLCHAIN}/bin:${PATH}" ./configure \
      --target=arm64-android-gcc \
      --prefix="${PREFIX}" \
      --disable-examples \
      --disable-unit-tests \
      --disable-tools \
      --disable-docs \
      --disable-webm-io \
      --disable-install-bins \
      --enable-static \
      --disable-shared
    make -j"${NPROC}"
    make install
  )
}

build_rnnoise() {
  if [ -f "${PREFIX}/lib/librnnoise.a" ] && [ -f "${PREFIX}/lib/pkgconfig/rnnoise.pc" ]; then
    msg "rnnoise already built"
    return
  fi
  ensure_repo "rnnoise" "https://github.com/xiph/rnnoise.git" "cdf196b1e9de2f8ff1003328ebf9a4316477429d"
  (
    cd "${SRC_ROOT}/rnnoise"
    if [ ! -x "./configure" ]; then
      ./autogen.sh
    fi
    make distclean || true
    CC="${CC}" CXX="${CXX}" AR="${AR}" RANLIB="${RANLIB}" \
      ./configure \
      --host="${TARGET_HOST}" \
      --prefix="${PREFIX}" \
      --enable-static \
      --disable-shared \
      CFLAGS="-fPIC -O3"
    make -j"${NPROC}"
    make install
  )
}

build_openssl() {
  if [ -f "${PREFIX}/lib/libssl.a" ] && [ -f "${PREFIX}/lib/libcrypto.a" ] && [ -f "${PREFIX}/lib/pkgconfig/openssl.pc" ]; then
    msg "openssl already built"
    return
  fi
  ensure_repo "openssl-android" "https://github.com/openssl/openssl" "a7e992847de83aa36be0c399c89db3fb827b0be2"
  (
    cd "${SRC_ROOT}/openssl-android"
    make clean || true
    PATH="${TOOLCHAIN}/bin:${PATH}" \
      perl ./Configure android-arm64 no-shared no-tests \
      --prefix="${PREFIX}" \
      --openssldir="${PREFIX}/ssl" \
      "-D__ANDROID_API__=${ANDROID_API}"
    make -j"${NPROC}"
    make install_sw
  )
}

build_ffmpeg() {
  if [ -f "${FFMPEG_PREFIX}/lib/libavcodec.a" ] && [ -f "${FFMPEG_PREFIX}/lib/pkgconfig/libavcodec.pc" ]; then
    msg "ffmpeg already built"
    return
  fi
  ensure_repo "ffmpeg" "https://github.com/FFmpeg/FFmpeg.git" "d07d2a4ee132e8349fb290ca460e28488f4a6a1c"
  rm -rf "${FFMPEG_PREFIX}"
  mkdir -p "${FFMPEG_PREFIX}"
  (
    cd "${SRC_ROOT}/ffmpeg"
    make distclean || true
    ./configure \
      --prefix="${FFMPEG_PREFIX}" \
      --target-os=android \
      --arch=aarch64 \
      --cpu=armv8-a \
      --enable-cross-compile \
      --sysroot="${SYSROOT}" \
      --cc="${CC}" \
      --cxx="${CXX}" \
      --ar="${AR}" \
      --ranlib="${RANLIB}" \
      --strip="${STRIP}" \
      --nm="${NM}" \
      --enable-pic \
      --disable-shared \
      --enable-static \
      --disable-programs \
      --disable-doc \
      --disable-debug
    make -j"${NPROC}"
    make install
  )
}

build_tde2e() {
  if [ -f "${PREFIX}/lib/libtde2e.a" ] && [ -f "${PREFIX}/lib/cmake/tde2e/tde2eConfig.cmake" ]; then
    msg "tde2e already built"
    return
  fi
  ensure_repo "tde2e-td" "https://github.com/tdlib/td.git" "51743dfd01dff6179e2d8f7095729caa4e2222e9"

  rm -rf "${SRC_ROOT}/tde2e-td/out-host-gen" "${SRC_ROOT}/tde2e-td/out-android-arm64"

  env -u PKG_CONFIG_PATH -u PKG_CONFIG_LIBDIR \
    cmake -S "${SRC_ROOT}/tde2e-td" -B "${SRC_ROOT}/tde2e-td/out-host-gen" -GNinja \
      -DCMAKE_BUILD_TYPE=Release \
      -DOPENSSL_ROOT_DIR="/usr" \
      -DOPENSSL_INCLUDE_DIR="/usr/include" \
      -DOPENSSL_SSL_LIBRARY="/usr/lib/x86_64-linux-gnu/libssl.so" \
      -DOPENSSL_CRYPTO_LIBRARY="/usr/lib/x86_64-linux-gnu/libcrypto.so" \
      -DTD_E2E_ONLY=ON \
      -DTD_ENABLE_DOTNET=OFF \
      -DTD_ENABLE_JNI=OFF \
      -DTD_INSTALL_STATIC_LIBRARIES=ON \
      -DTDE2E_ENABLE_INSTALL=ON \
      -DTDE2E_INSTALL_INCLUDES=ON
  env -u PKG_CONFIG_PATH -u PKG_CONFIG_LIBDIR \
    cmake --build "${SRC_ROOT}/tde2e-td/out-host-gen" --target prepare_cross_compiling -j"${NPROC}"

  cmake -S "${SRC_ROOT}/tde2e-td" -B "${SRC_ROOT}/tde2e-td/out-android-arm64" -GNinja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE="${ANDROID_NDK_ROOT}/build/cmake/android.toolchain.cmake" \
    -DANDROID_ABI=arm64-v8a \
    -DANDROID_PLATFORM="${ANDROID_API}" \
    -DCMAKE_INSTALL_PREFIX="${PREFIX}" \
    -DCMAKE_PREFIX_PATH="${PREFIX}" \
    -DCMAKE_FIND_ROOT_PATH="${PREFIX}" \
    -DOPENSSL_FOUND=1 \
    -DOPENSSL_INCLUDE_DIR="${PREFIX}/include" \
    -DOPENSSL_SSL_LIBRARY="${PREFIX}/lib/libssl.a" \
    -DOPENSSL_CRYPTO_LIBRARY="${PREFIX}/lib/libcrypto.a" \
    -DTD_E2E_ONLY=ON \
    -DTD_ENABLE_DOTNET=OFF \
    -DTD_ENABLE_JNI=OFF \
    -DTD_INSTALL_STATIC_LIBRARIES=ON \
    -DTDE2E_ENABLE_INSTALL=ON \
    -DTDE2E_INSTALL_INCLUDES=ON
  cmake --build "${SRC_ROOT}/tde2e-td/out-android-arm64" --target install -j"${NPROC}"
}

build_tg_owt() {
  if [ -f "${PREFIX}/lib/libtg_owt.a" ] && [ -f "${PREFIX}/lib/cmake/tg_owt/tg_owtConfig.cmake" ]; then
    msg "tg_owt already built"
    return
  fi
  ensure_repo "tg_owt" "https://github.com/desktop-app/tg_owt.git" "5c5c71258777d0196dbb3a09cc37d2f56ead28ab" "1"

  local byte_order_file="${SRC_ROOT}/tg_owt/src/rtc_base/byte_order.h"
  if ! grep -q "WEBRTC_LINUX) || defined(__ANDROID__)" "${byte_order_file}"; then
    sed -i "s/#elif defined(WEBRTC_LINUX)/#elif defined(WEBRTC_LINUX) || defined(__ANDROID__)/" "${byte_order_file}"
  fi

  rm -rf "${SRC_ROOT}/tg_owt/out/android-arm64"
  PKG_CONFIG_PATH="${PREFIX}/lib/pkgconfig" \
  PKG_CONFIG_LIBDIR="${PREFIX}/lib/pkgconfig" \
  cmake -S "${SRC_ROOT}/tg_owt" -B "${SRC_ROOT}/tg_owt/out/android-arm64" -GNinja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE="${ANDROID_NDK_ROOT}/build/cmake/android.toolchain.cmake" \
    -DANDROID_ABI=arm64-v8a \
    -DANDROID_PLATFORM="${ANDROID_API}" \
    -DBUILD_SHARED_LIBS=OFF \
    -DCMAKE_INSTALL_PREFIX="${PREFIX}" \
    -DCMAKE_PREFIX_PATH="${PREFIX}" \
    -DCMAKE_FIND_ROOT_PATH="${PREFIX}" \
    -DTG_OWT_USE_X11=OFF \
    -DTG_OWT_USE_PIPEWIRE=OFF \
    -DJPEG_INCLUDE_DIR="${PREFIX}/include" \
    -DJPEG_LIBRARY="${PREFIX}/lib/libjpeg.a" \
    -DOPENSSL_ROOT_DIR="${PREFIX}" \
    -DOPENSSL_INCLUDE_DIR="${PREFIX}/include" \
    -DOPENSSL_SSL_LIBRARY="${PREFIX}/lib/libssl.a" \
    -DOPENSSL_CRYPTO_LIBRARY="${PREFIX}/lib/libcrypto.a"
  cmake --build "${SRC_ROOT}/tg_owt/out/android-arm64" --target install -j"${NPROC}"
}

validate_outputs() {
  local required=(
    "${PREFIX}/lib/libjpeg.a"
    "${PREFIX}/lib/libopus.a"
    "${PREFIX}/lib/libopenal.a"
    "${PREFIX}/lib/libopenh264.a"
    "${PREFIX}/lib/libvpx.a"
    "${PREFIX}/lib/librnnoise.a"
    "${PREFIX}/lib/libssl.a"
    "${PREFIX}/lib/libcrypto.a"
    "${PREFIX}/lib/libtde2e.a"
    "${PREFIX}/lib/libtdutils.a"
    "${PREFIX}/lib/libtg_owt.a"
    "${PREFIX}/lib/pkgconfig/libjpeg.pc"
    "${PREFIX}/lib/pkgconfig/opus.pc"
    "${PREFIX}/lib/pkgconfig/openal.pc"
    "${PREFIX}/lib/pkgconfig/openh264.pc"
    "${PREFIX}/lib/pkgconfig/vpx.pc"
    "${PREFIX}/lib/pkgconfig/rnnoise.pc"
    "${PREFIX}/lib/pkgconfig/openssl.pc"
    "${PREFIX}/lib/cmake/OpenAL/OpenALConfig.cmake"
    "${PREFIX}/lib/cmake/tde2e/tde2eConfig.cmake"
    "${PREFIX}/lib/cmake/tg_owt/tg_owtConfig.cmake"
    "${FFMPEG_PREFIX}/lib/libavcodec.a"
    "${FFMPEG_PREFIX}/lib/libavformat.a"
    "${FFMPEG_PREFIX}/lib/libavutil.a"
    "${FFMPEG_PREFIX}/lib/libswresample.a"
    "${FFMPEG_PREFIX}/lib/libswscale.a"
    "${FFMPEG_PREFIX}/lib/libavfilter.a"
    "${FFMPEG_PREFIX}/lib/pkgconfig/libavcodec.pc"
  )

  for file in "${required[@]}"; do
    if [ ! -f "${file}" ]; then
      echo "[DEPS][ERROR] Missing required output: ${file}" >&2
      return 1
    fi
  done
}

main() {
  mkdir -p "${DEPS_ROOT}" "${SRC_ROOT}" "${PREFIX}" "${FFMPEG_PREFIX}"
  prepare_toolchain_wrappers
  export PATH="${WRAP_DIR}:${TOOLCHAIN}/bin:${PATH}"
  export PKG_CONFIG_PATH="${PREFIX}/lib/pkgconfig"
  export PKG_CONFIG_LIBDIR="${PREFIX}/lib/pkgconfig"

  build_libjpeg
  build_opus
  build_openal
  build_openh264
  build_libvpx
  build_rnnoise
  build_openssl
  build_ffmpeg
  build_tde2e
  build_tg_owt
  validate_outputs

  msg "ready: ${PREFIX} and ${FFMPEG_PREFIX}"
  ls -lh "${PREFIX}/lib" | sed -n '1,200p'
  ls -lh "${FFMPEG_PREFIX}/lib" | sed -n '1,200p'
}

main "$@"
