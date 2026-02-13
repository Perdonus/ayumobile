## Android native APK build (Qt/C++)

This repository contains a native Android target for AyuGram Desktop (`DESKTOP_APP_SPECIAL_TARGET=android`).

## 1) Clone + submodules

```bash
git clone --recursive https://github.com/Perdonus/ayumobile.git
cd ayumobile
```

If you cloned without `--recursive`:

```bash
git submodule update --init --recursive
```

## 2) Apply submodule patches from this repository

The Android port includes local patch-sets for several submodules. Apply them before CMake configure:

```bash
./scripts/apply-android-submodule-patches.sh
```

## 3) Configure CMake for Android

Required:

- Qt for Android (host + android kit)
- Android SDK + NDK
- JDK 17+

Example variables:

```bash
export QT_HOST_PATH=/opt/qt/6.8.3/gcc_64
export QT_ANDROID_PATH=/opt/qt/6.8.3/android_arm64_v8a
export ANDROID_SDK_ROOT=/opt/android-sdk
export ANDROID_NDK_ROOT=$ANDROID_SDK_ROOT/ndk/26.1.10909125
```

Configure and build:

```bash
cmake -S . -B out/android-arm64-native \
  -GNinja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK_ROOT/build/cmake/android.toolchain.cmake \
  -DCMAKE_PREFIX_PATH=$QT_ANDROID_PATH \
  -DQT_HOST_PATH=$QT_HOST_PATH \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=android-29 \
  -DDESKTOP_APP_USE_PACKAGED=ON \
  -DDESKTOP_APP_SPECIAL_TARGET=android

cmake --build out/android-arm64-native --target Telegram
```

## 4) APK location

Expected APK path:

```text
out/android-arm64-native/bin/Telegram.apk
```

If Qt/CMake places artifacts differently on your setup, search for generated apk files:

```bash
find out/android-arm64-native -name "*.apk" -print
```
