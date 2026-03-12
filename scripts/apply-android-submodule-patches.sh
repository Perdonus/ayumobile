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
  local submodule_dir="$ROOT_DIR/$submodule_path"

  if [[ ! -d "$submodule_dir" ]]; then
    echo "Submodule directory is missing: $submodule_path" >&2
    exit 1
  fi
  if [[ ! -f "$patch_file" ]]; then
    echo "Patch file is missing: $patch_file" >&2
    exit 1
  fi

  if ! git -C "$submodule_dir" rev-parse --is-inside-work-tree >/dev/null 2>&1; then
    echo "Submodule is not a git worktree, reinitializing: $submodule_path"
    git -C "$ROOT_DIR" submodule sync -- "$submodule_path" || true
    git -C "$ROOT_DIR" submodule update --init --recursive "$submodule_path"
  fi

  if ! git -C "$submodule_dir" rev-parse --is-inside-work-tree >/dev/null 2>&1; then
    echo "Failed to initialize submodule git worktree: $submodule_path" >&2
    exit 1
  fi

  echo "Applying patch for: $submodule_path"
  if git -C "$submodule_dir" apply --check --reverse "$patch_file" >/dev/null 2>&1; then
    echo "Already applied: $submodule_path"
    return
  fi

  if git -C "$submodule_dir" apply --check "$patch_file" >/dev/null 2>&1; then
    git -C "$submodule_dir" apply "$patch_file"
    echo "Applied: $submodule_path"
    return
  fi

  git -C "$submodule_dir" apply --3way --whitespace=nowarn "$patch_file"
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
