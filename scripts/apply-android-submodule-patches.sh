#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PATCH_DIR="$ROOT_DIR/patches/android-submodules"

if [[ ! -d "$PATCH_DIR" ]]; then
  echo "Patch directory is missing: $PATCH_DIR" >&2
  exit 1
fi

apply_patch_file() {
  local submodule_path="$1"
  local patch_file="$2"

  if [[ ! -d "$ROOT_DIR/$submodule_path" ]]; then
    echo "Submodule directory is missing: $submodule_path" >&2
    exit 1
  fi
  if [[ ! -f "$patch_file" ]]; then
    echo "Patch file is missing: $patch_file" >&2
    exit 1
  fi

  if git -C "$ROOT_DIR/$submodule_path" apply --check --reverse "$patch_file" >/dev/null 2>&1; then
    echo "Already applied: $submodule_path"
    return
  fi

  if git -C "$ROOT_DIR/$submodule_path" apply --check "$patch_file" >/dev/null 2>&1; then
    git -C "$ROOT_DIR/$submodule_path" apply "$patch_file"
    echo "Applied: $submodule_path"
    return
  fi

  git -C "$ROOT_DIR/$submodule_path" apply --3way --whitespace=nowarn "$patch_file"
  echo "Applied (3way): $submodule_path"
}

apply_patch_file "Telegram/ThirdParty/tgcalls" "$PATCH_DIR/Telegram__ThirdParty__tgcalls.patch"
apply_patch_file "Telegram/lib_base" "$PATCH_DIR/Telegram__lib_base.patch"
apply_patch_file "Telegram/lib_spellcheck" "$PATCH_DIR/Telegram__lib_spellcheck.patch"
apply_patch_file "Telegram/lib_storage" "$PATCH_DIR/Telegram__lib_storage.patch"
apply_patch_file "Telegram/lib_ui" "$PATCH_DIR/Telegram__lib_ui.patch"
apply_patch_file "Telegram/lib_webrtc" "$PATCH_DIR/Telegram__lib_webrtc.patch"
apply_patch_file "Telegram/lib_webview" "$PATCH_DIR/Telegram__lib_webview.patch"
apply_patch_file "cmake" "$PATCH_DIR/cmake.patch"

echo "All Android submodule patches are applied."
