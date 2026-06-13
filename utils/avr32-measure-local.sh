#!/usr/bin/env bash
set -euo pipefail

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
default_llvm_repo=$(cd -- "$script_dir/.." && pwd)

LLVM_REPO=${LLVM_REPO:-$default_llvm_repo}
BASE=${BASE:-$(cd -- "$LLVM_REPO/.." && pwd)}
BUILD_DIR=${BUILD_DIR:-$LLVM_REPO/build-avr32}
SDR_WIDGET_ROOT=${SDR_WIDGET_ROOT:-$BASE/sdr-widget}
SDR_LLVM_BUILD=${SDR_LLVM_BUILD:-$SDR_WIDGET_ROOT/Release-llvm-relax}
SDR_LLVM_ELF=${SDR_LLVM_ELF:-$SDR_LLVM_BUILD/widget.elf}
SDR_GCC_ELF=${SDR_GCC_ELF:-}
LINUX_SRC=${LINUX_SRC:-$BASE/linux-avr32}
LINUX_LLVM_BUILD=${LINUX_LLVM_BUILD:-$BASE/linux-avr32-build-llvm-shell}
LINUX_GCC_BUILD=${LINUX_GCC_BUILD:-$BASE/linux-avr32-build-gcc-shell}
ROOTFS=${ROOTFS:-$BASE/avr32-linux-rootfs}
QEMU=${QEMU:-$BASE/qemu-avr32-v11-sync/build/qemu-system-avr32}
HOST_SHIM=${HOST_SHIM:-$BASE/linux-avr32-host/include}
OUT_DIR=${OUT_DIR:-/tmp/avr32-measure-local}
TOP=${TOP:-12}
SCALE=${SCALE:-100}

detect_jobs() {
  if command -v sysctl >/dev/null 2>&1; then
    sysctl -n hw.ncpu 2>/dev/null && return
  fi
  if command -v nproc >/dev/null 2>&1; then
    nproc 2>/dev/null && return
  fi
  echo 4
}

detect_brew_prefix() {
  if command -v brew >/dev/null 2>&1; then
    brew --prefix 2>/dev/null && return
  fi
  if [[ -d /opt/homebrew ]]; then
    echo /opt/homebrew
    return
  fi
  if [[ -d /usr/local/Homebrew || -d /usr/local/Cellar ]]; then
    echo /usr/local
    return
  fi
}

first_existing() {
  local path
  for path in "$@"; do
    [[ -n "$path" && -e "$path" ]] && {
      echo "$path"
      return 0
    }
  done
  return 1
}

brew_prefix=${BREW_PREFIX:-$(detect_brew_prefix || true)}
COREUTILS=${COREUTILS:-$(first_existing \
  "$brew_prefix/opt/coreutils/libexec/gnubin" \
  /opt/homebrew/opt/coreutils/libexec/gnubin \
  /usr/local/opt/coreutils/libexec/gnubin || true)}
AVR32_TOOLS=${AVR32_TOOLS:-$(first_existing \
  "$BASE/avr32-toolchain-macos-arm64/avr32-tools-src/bin" \
  "$BASE/avr32-toolchain/avr32-tools-src/bin" \
  "$brew_prefix/opt/avr32-toolchain/bin" \
  /opt/homebrew/opt/avr32-toolchain/bin \
  /usr/local/opt/avr32-toolchain/bin || true)}
AVR32_SYSROOT=${AVR32_SYSROOT:-$(first_existing \
  "$BASE/avr32-toolchain-macos-arm64/avr32-tools-src/avr32" \
  "$BASE/avr32-toolchain/avr32-tools-src/avr32" \
  "$brew_prefix/opt/avr32-toolchain/avr32" \
  /opt/homebrew/opt/avr32-toolchain/avr32 \
  /usr/local/opt/avr32-toolchain/avr32 || true)}
JOBS=${JOBS:-$(detect_jobs)}
ACTION=${1:-status}

die() {
  echo "error: $*" >&2
  exit 1
}

require_file() {
  [[ -f "$1" ]] || die "missing file: $1"
}

require_dir() {
  [[ -d "$1" ]] || die "missing directory: $1"
}

require_exec() {
  [[ -x "$1" ]] || die "missing executable: $1"
}

tool() {
  local name=$1
  if [[ -n "$AVR32_TOOLS" && -x "$AVR32_TOOLS/$name" ]]; then
    echo "$AVR32_TOOLS/$name"
    return
  fi
  command -v "$name" || die "missing tool: $name"
}

llvm_bin() {
  echo "$BUILD_DIR/bin/$1"
}

usage() {
  cat <<'EOF'
usage: utils/avr32-measure-local.sh <action>

actions:
  status        Show detected paths, versions, and repo state.
  build-tools   Build AVR32 LLVM tools needed for the measurement.
  lit-core      Run focused AVR32 CodeGen/MC/lld tests.
  sdr-relink    Relink SDR-widget LLVM ELF with current ld.lld and compare sizes.
  sdr-size      Compare existing SDR-widget GCC and LLVM ELFs.
  linux-relink  Force-relink LLVM AVR32 Linux vmlinux and compare sizes.
  linux-size    Compare existing Linux GCC and LLVM vmlinux files.
  smoke-both    Boot smoke-test current LLVM and GCC Linux kernels in QEMU.
  all           Run build-tools, lit-core, sdr-relink, linux-relink, smoke-both.

common overrides:
  BASE LLVM_REPO BUILD_DIR AVR32_TOOLS AVR32_SYSROOT SDR_WIDGET_ROOT
  LINUX_SRC LINUX_LLVM_BUILD LINUX_GCC_BUILD ROOTFS QEMU HOST_SHIM OUT_DIR
EOF
}

section_size() {
  local file=$1
  local section=$2
  "$(tool avr32-size)" -A "$file" |
    awk -v wanted="$section" '$1 == wanted { print $2; found=1 } END { if (!found) print 0 }'
}

section_prefix_total() {
  local file=$1
  local base=$2
  "$(tool avr32-size)" -A "$file" |
    awk -v base="$base" '
      $1 == base || index($1, base ".") == 1 { total += $2 }
      END { print total + 0 }
    '
}

sdr_flash() {
  local file=$1
  local text rodata data
  text=$(section_prefix_total "$file" .text)
  rodata=$(section_prefix_total "$file" .rodata)
  data=$(section_prefix_total "$file" .data)
  echo $((text + rodata + data))
}

linux_flash_like() {
  local file=$1
  "$(tool avr32-size)" -A "$file" |
    awk '
      $1 ~ /^(\.init|\.init\.data|\.text|\.note\.gnu\.build-id|__ex_table|\.rodata|\.rodata1|__bug_table|\.pci_fixup|\.builtin_fw|__ksymtab|__ksymtab_gpl|__ksymtab_unused|__ksymtab_unused_gpl|__ksymtab_gpl_future|__kcrctab|__kcrctab_gpl|__kcrctab_unused|__kcrctab_unused_gpl|__kcrctab_gpl_future|__ksymtab_strings|__init_rodata|__param|__modver|\.data)$/ {
        total += $2
      }
      END { print total + 0 }
    '
}

print_delta() {
  local label=$1
  local old=$2
  local new=$3
  local delta=$((new - old))
  printf "%-24s %10d %10d %+10d\n" "$label" "$old" "$new" "$delta"
}

default_sdr_gcc_elf() {
  first_existing \
    "$SDR_WIDGET_ROOT/Release-gcc-sweep/widget-gcc-Os.elf" \
    "$SDR_WIDGET_ROOT/Release/widget.elf" || true
}

show_status() {
  echo "host=$(hostname)"
  echo "machine=$(uname -m)"
  echo "macos=$(sw_vers -productVersion 2>/dev/null || uname -r)"
  echo "brew_prefix=${brew_prefix:-}"
  echo "BASE=$BASE"
  echo "LLVM_REPO=$LLVM_REPO"
  echo "BUILD_DIR=$BUILD_DIR"
  echo "JOBS=$JOBS"
  echo "AVR32_TOOLS=${AVR32_TOOLS:-}"
  echo "AVR32_SYSROOT=${AVR32_SYSROOT:-}"
  echo "COREUTILS=${COREUTILS:-}"
  echo "SDR_WIDGET_ROOT=$SDR_WIDGET_ROOT"
  echo "LINUX_SRC=$LINUX_SRC"
  echo "LINUX_LLVM_BUILD=$LINUX_LLVM_BUILD"
  echo "LINUX_GCC_BUILD=$LINUX_GCC_BUILD"
  echo "ROOTFS=$ROOTFS"
  echo "QEMU=$QEMU"
  git -C "$LLVM_REPO" status --short --branch
  git -C "$LLVM_REPO" rev-parse --short=12 HEAD
  [[ -x "$(llvm_bin clang)" ]] && "$(llvm_bin clang)" --version | sed -n '1p'
  [[ -x "$(llvm_bin ld.lld)" ]] && "$(llvm_bin ld.lld)" --version | sed -n '1p'
  [[ -n "${AVR32_TOOLS:-}" && -x "$AVR32_TOOLS/avr32-gcc" ]] &&
    "$AVR32_TOOLS/avr32-gcc" --version | sed -n '1p'
}

build_tools() {
  local targets=(
    clang
    llc
    lld
    llvm-mc
    llvm-readobj
    llvm-readelf
    llvm-objdump
    llvm-dwarfdump
    yaml2obj
    obj2yaml
    FileCheck
    count
    not
    split-file
    llvm-config
  )
  if [[ -f "$BUILD_DIR/build.ninja" ]]; then
    command -v ninja >/dev/null 2>&1 || die "ninja is not in PATH"
    ninja -C "$BUILD_DIR" -j"$JOBS" "${targets[@]}"
  elif [[ -f "$BUILD_DIR/Makefile" ]]; then
    make -C "$BUILD_DIR" -j"$JOBS" "${targets[@]}"
  else
    die "missing supported LLVM build directory: $BUILD_DIR"
  fi
}

lit_core() {
  local lit
  lit=$(llvm_bin llvm-lit)
  require_exec "$lit"
  "$lit" -sv \
    "$LLVM_REPO/llvm/test/CodeGen/AVR32" \
    "$LLVM_REPO/llvm/test/MC/AVR32" \
    "$LLVM_REPO/lld/test/ELF/avr32.test" \
    "$LLVM_REPO/lld/test/ELF/avr32-relax.s" \
    "$LLVM_REPO/lld/test/ELF/avr32-direct-data.s" \
    "$LLVM_REPO/lld/test/ELF/avr32-lddpc-relax.s"
}

write_sdr_response_file() {
  local rsp=$1
  require_file "$SDR_LLVM_BUILD/makefile"
  : > "$rsp"
  (
    cd "$SDR_LLVM_BUILD"
    while IFS= read -r mk; do
      [[ -f "$mk" ]] || continue
      awk '
        /^OBJS \+=/ { capture=1; next }
        capture {
          line=$0
          sub(/[[:space:]]*\\[[:space:]]*$/, "", line)
          n=split(line, parts, /[[:space:]]+/)
          for (i = 1; i <= n; i++) if (parts[i] != "") print parts[i]
          if ($0 !~ /\\[[:space:]]*$/) capture=0
        }
      ' "$mk" >> "$rsp"
    done < <(awk '/^-include .*subdir\.mk|^-include objects\.mk/ { print $2 }' makefile)
  )
}

sdr_relink() {
  local clang rsp out old gcc
  require_dir "$SDR_LLVM_BUILD"
  require_exec "$(llvm_bin clang)"
  require_exec "$(llvm_bin ld.lld)"
  [[ -n "$AVR32_SYSROOT" ]] || die "AVR32_SYSROOT was not detected"
  mkdir -p "$OUT_DIR"
  clang=$(llvm_bin clang)
  rsp="$OUT_DIR/sdr-objects.rsp"
  out="$OUT_DIR/sdr-widget-llvm-current.elf"
  write_sdr_response_file "$rsp"
  old=$(wc -l < "$rsp" | tr -d ' ')
  [[ "$old" -gt 0 ]] || die "no SDR-widget objects found in $SDR_LLVM_BUILD"
  (
    cd "$SDR_LLVM_BUILD"
    "$clang" --target=avr32 -mpart=uc3a3256 --sysroot="$AVR32_SYSROOT" \
      -nostartfiles -mrelax \
      -Wl,--gc-sections -Wl,-e,_trampoline -Wl,--direct-data \
      -o "$out" @"$rsp" -lm
  )
  echo "sdr_objects=$old"
  echo "sdr_llvm_current=$out"
  if [[ -f "$SDR_LLVM_ELF" ]]; then
    compare_sdr_pair "sdr_llvm_before_current" "$SDR_LLVM_ELF" "$out"
  fi
  gcc=${SDR_GCC_ELF:-$(default_sdr_gcc_elf)}
  if [[ -n "$gcc" && -f "$gcc" ]]; then
    compare_sdr_pair "sdr_gcc_vs_llvm" "$gcc" "$out"
  fi
}

compare_sdr_pair() {
  local label=$1
  local old_file=$2
  local new_file=$3
  local old_flash new_flash
  old_flash=$(sdr_flash "$old_file")
  new_flash=$(sdr_flash "$new_file")
  echo
  echo "$label"
  printf "%-24s %10s %10s %10s\n" "metric" "old/gcc" "new/llvm" "delta"
  print_delta flash "$old_flash" "$new_flash"
  print_delta .text "$(section_prefix_total "$old_file" .text)" "$(section_prefix_total "$new_file" .text)"
  print_delta .exception "$(section_size "$old_file" .exception)" "$(section_size "$new_file" .exception)"
  print_delta .rodata "$(section_prefix_total "$old_file" .rodata)" "$(section_prefix_total "$new_file" .rodata)"
  print_delta .data "$(section_prefix_total "$old_file" .data)" "$(section_prefix_total "$new_file" .data)"
  print_delta .bss "$(section_prefix_total "$old_file" .bss)" "$(section_prefix_total "$new_file" .bss)"
}

linux_size() {
  local llvm="$LINUX_LLVM_BUILD/vmlinux"
  local gcc="$LINUX_GCC_BUILD/vmlinux"
  require_file "$llvm"
  require_file "$gcc"
  compare_linux_pair "linux_gcc_vs_llvm" "$gcc" "$llvm"
}

linux_relink_impl() {
  local before="$OUT_DIR/vmlinux-llvm-before-relink"
  local after="$LINUX_LLVM_BUILD/vmlinux"
  require_file "$LINUX_LLVM_BUILD/Makefile"
  require_file "$LINUX_SRC/Makefile"
  require_exec "$(llvm_bin clang)"
  require_exec "$(llvm_bin ld.lld)"
  require_file "$LLVM_REPO/utils/avr32-linux-ld-lld-wrapper.sh"
  [[ -n "$AVR32_TOOLS" ]] || die "AVR32_TOOLS was not detected"
  [[ -n "$COREUTILS" ]] || die "coreutils gnubin directory was not detected"
  mkdir -p "$OUT_DIR"
  [[ -f "$after" ]] && cp "$after" "$before"
  rm -f "$after"
  PATH="$COREUTILS:$AVR32_TOOLS:$PATH" \
  LLVM_AVR32_LD_LLD="$(llvm_bin ld.lld)" \
  make -C "$LINUX_LLVM_BUILD" -j"$JOBS" ARCH=avr32 \
    HOSTCFLAGS="-I$HOST_SHIM -I$LINUX_SRC/include/uapi -fno-pie" \
    HOSTLDFLAGS="-Wl,-no_pie" \
    CC="$(llvm_bin clang) --target=avr32 -fno-integrated-as -Wa,--linkrelax -B$AVR32_TOOLS/" \
    LD="$LLVM_REPO/utils/avr32-linux-ld-lld-wrapper.sh" \
    AR="$AVR32_TOOLS/avr32-ar" \
    NM="$AVR32_TOOLS/avr32-nm" \
    OBJCOPY="$AVR32_TOOLS/avr32-objcopy" \
    OBJDUMP="$AVR32_TOOLS/avr32-objdump" \
    READELF="$AVR32_TOOLS/avr32-readelf" \
    vmlinux
  require_file "$after"
  if [[ -f "$before" ]]; then
    compare_linux_pair "linux_llvm_before_current" "$before" "$after"
  fi
  if [[ -f "$LINUX_GCC_BUILD/vmlinux" ]]; then
    compare_linux_pair "linux_gcc_vs_llvm" "$LINUX_GCC_BUILD/vmlinux" "$after"
  fi
}

compare_linux_pair() {
  local label=$1
  local old_file=$2
  local new_file=$3
  echo
  echo "$label"
  printf "%-24s %10s %10s %10s\n" "metric" "old/gcc" "new/llvm" "delta"
  print_delta flash_like "$(linux_flash_like "$old_file")" "$(linux_flash_like "$new_file")"
  print_delta .init "$(section_size "$old_file" .init)" "$(section_size "$new_file" .init)"
  print_delta .text "$(section_size "$old_file" .text)" "$(section_size "$new_file" .text)"
  print_delta .rodata "$(section_size "$old_file" .rodata)" "$(section_size "$new_file" .rodata)"
  print_delta .data "$(section_size "$old_file" .data)" "$(section_size "$new_file" .data)"
  print_delta .bss "$(section_size "$old_file" .bss)" "$(section_size "$new_file" .bss)"
}

smoke_both() {
  require_exec "$ROOTFS/smoke-shell.py"
  require_exec "$QEMU"
  require_file "$LINUX_LLVM_BUILD/vmlinux"
  require_file "$LINUX_GCC_BUILD/vmlinux"
  "$ROOTFS/smoke-shell.py" "$LINUX_LLVM_BUILD/vmlinux" --qemu "$QEMU" \
    --log "$OUT_DIR/avr32-linux-llvm-smoke.log"
  "$ROOTFS/smoke-shell.py" "$LINUX_GCC_BUILD/vmlinux" --qemu "$QEMU" \
    --log "$OUT_DIR/avr32-linux-gcc-smoke.log"
}

case "$ACTION" in
  status)
    show_status
    ;;
  build-tools)
    build_tools
    ;;
  lit-core)
    lit_core
    ;;
  sdr-relink)
    sdr_relink
    ;;
  sdr-size)
    compare_sdr_pair "sdr_gcc_vs_llvm" "${SDR_GCC_ELF:-$(default_sdr_gcc_elf)}" "$SDR_LLVM_ELF"
    ;;
  linux-relink)
    linux_relink_impl
    ;;
  linux-size)
    linux_size
    ;;
  smoke-both)
    smoke_both
    ;;
  all)
    show_status
    build_tools
    lit_core
    sdr_relink
    linux_relink_impl
    smoke_both
    ;;
  -h|--help|help)
    usage
    ;;
  *)
    usage >&2
    die "unknown action: $ACTION"
    ;;
esac
