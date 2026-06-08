#!/usr/bin/env bash
set -euo pipefail

script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
repo_root=$(cd "${script_dir}/.." && pwd)

AVR32_GNU_PREFIX=${AVR32_GNU_PREFIX:-/Users/cozy/cozycactus/avr32-toolchain-macos-arm64/avr32-tools-src/bin/avr32-}
AVR32_AS=${AVR32_AS:-${AVR32_GNU_PREFIX}as}
AVR32_LD=${AVR32_LD:-${AVR32_GNU_PREFIX}ld}
AVR32_OBJDUMP=${AVR32_OBJDUMP:-${AVR32_GNU_PREFIX}objdump}
AVR32_CLANG=${AVR32_CLANG:-${repo_root}/build-avr32/bin/clang}
AVR32_GDB=${AVR32_GDB:-/tmp/avr32-gdb-build/gdb/gdb}
AVR32_QEMU=${AVR32_QEMU:-/tmp/qemu-avr32-build/qemu-system-avr32}
AVR32_GDB_PORT=${AVR32_GDB_PORT:-1234}
AVR32_FLASH_BASE=${AVR32_FLASH_BASE:-0x80000000}

require_executable() {
  local path=$1
  if [[ ! -x "$path" ]]; then
    echo "missing executable: $path" >&2
    exit 1
  fi
}

require_executable "$AVR32_AS"
require_executable "$AVR32_LD"
require_executable "$AVR32_OBJDUMP"
require_executable "$AVR32_CLANG"
require_executable "$AVR32_GDB"
require_executable "$AVR32_QEMU"

tmpdir=$(mktemp -d "${TMPDIR:-/tmp}/avr32-qemu-smoke.XXXXXX")
qemu_pid=

stop_qemu() {
  if [[ -n "$qemu_pid" ]] && kill -0 "$qemu_pid" 2>/dev/null; then
    kill "$qemu_pid" 2>/dev/null || true
    wait "$qemu_pid" 2>/dev/null || true
  fi
  qemu_pid=
}

cleanup() {
  stop_qemu
  rm -rf "$tmpdir"
}
trap cleanup EXIT

write_linker_script() {
  local path=$1
  cat > "$path" <<EOF
ENTRY(_start)

SECTIONS
{
  . = ${AVR32_FLASH_BASE};

  .text : {
    *(.text*)
  }

  .rodata : {
    *(.rodata*)
  }

  .data : {
    *(.data*)
  }

  .dalign : {
    *(.dalign*)
  }

  .bss : {
    *(.bss*)
    *(COMMON)
  }
}
EOF
}

start_qemu() {
  local elf=$1
  local log=$2

  "$AVR32_QEMU" \
    -M avr32example-board \
    -nographic \
    -S \
    -gdb "tcp::${AVR32_GDB_PORT}" \
    -bios "$elf" \
    > "$log" 2>&1 &
  qemu_pid=$!

  for _ in 1 2 3 4 5 6 7 8 9 10; do
    if grep -q "Loaded boot image successfully" "$log"; then
      return
    fi
    if ! kill -0 "$qemu_pid" 2>/dev/null; then
      echo "QEMU exited before GDB attach" >&2
      cat "$log" >&2
      exit 1
    fi
    sleep 0.2
  done

  echo "QEMU did not finish firmware load before timeout" >&2
  cat "$log" >&2
  exit 1
}

run_gdb() {
  local elf=$1
  local log=$2
  shift 2

  "$AVR32_GDB" --batch "$elf" \
    -ex "set confirm off" \
    -ex "set architecture avr32" \
    -ex "target remote :${AVR32_GDB_PORT}" \
    "$@" \
    > "$log" 2>&1
}

dump_failure() {
  local message=$1
  local gdb_log=$2
  local qemu_log=$3
  local objdump=$4

  echo "$message" >&2
  cat "$gdb_log" >&2
  echo "--- qemu log ---" >&2
  cat "$qemu_log" >&2
  echo "--- objdump ---" >&2
  cat "$objdump" >&2
}

run_asm_smoke() {
  local dir="$tmpdir/asm"
  mkdir -p "$dir"

  cat > "$dir/smoke.S" <<'ASM'
        .section .text
        .global _start
        .type _start, @function
_start:
        mov     r12, 42
        nop
1:
        rjmp    1b

        .section .data
ASM

  write_linker_script "$dir/smoke.ld"
  "$AVR32_AS" -o "$dir/smoke.o" "$dir/smoke.S"
  "$AVR32_LD" -T "$dir/smoke.ld" -o "$dir/smoke.elf" "$dir/smoke.o"
  "$AVR32_OBJDUMP" -dr "$dir/smoke.elf" > "$dir/objdump.txt"

  start_qemu "$dir/smoke.elf" "$dir/qemu.log"
  run_gdb "$dir/smoke.elf" "$dir/gdb.log" \
    -ex "info registers pc r12" \
    -ex "x/4i \$pc" \
    -ex "si" \
    -ex "info registers pc r12" \
    -ex "detach"
  stop_qemu

  if ! grep -Eq "r12[[:space:]]+0x2a[[:space:]]+42" "$dir/gdb.log"; then
    dump_failure "AVR32 asm smoke failed: r12 did not become 42" \
      "$dir/gdb.log" "$dir/qemu.log" "$dir/objdump.txt"
    exit 1
  fi

  echo "AVR32 asm smoke passed: stepped mov r12,42 at ${AVR32_FLASH_BASE}"
}

run_llvm_c_smoke() {
  local dir="$tmpdir/llvm-c"
  local done_pc
  mkdir -p "$dir"
  done_pc=$(printf "0x%x" "$((AVR32_FLASH_BASE + 8))")

  cat > "$dir/answer.c" <<'C'
int answer(int a, int b) {
  return a + b + 9;
}
C

  cat > "$dir/start.S" <<'ASM'
        .section .text
        .global _start
        .type _start, @function
_start:
        mov     r12, 7
        mov     r11, 26
        rcall   answer
done:
        rjmp    done

        .section .data
ASM

  write_linker_script "$dir/smoke.ld"
  "$AVR32_CLANG" --target=avr32 -mpart=uc3a3256 -Oz \
    -ffreestanding -fno-builtin \
    -c "$dir/answer.c" -o "$dir/answer.o"
  "$AVR32_AS" -o "$dir/start.o" "$dir/start.S"
  "$AVR32_LD" -T "$dir/smoke.ld" \
    -o "$dir/smoke.elf" "$dir/start.o" "$dir/answer.o"
  "$AVR32_OBJDUMP" -dr "$dir/smoke.elf" > "$dir/objdump.txt"

  start_qemu "$dir/smoke.elf" "$dir/qemu.log"
  run_gdb "$dir/smoke.elf" "$dir/gdb.log" \
    -ex "x/10i \$pc" \
    -ex "si" \
    -ex "si" \
    -ex "si" \
    -ex "si" \
    -ex "si" \
    -ex "si" \
    -ex "si" \
    -ex "si" \
    -ex "info registers pc lr r8 r9 r11 r12" \
    -ex "detach"
  stop_qemu

  if ! grep -Eq "r12[[:space:]]+0x2a[[:space:]]+42" "$dir/gdb.log"; then
    dump_failure "AVR32 LLVM C smoke failed: r12 did not become 42" \
      "$dir/gdb.log" "$dir/qemu.log" "$dir/objdump.txt"
    exit 1
  fi

  if ! grep -Eq "pc[[:space:]]+${done_pc}[[:space:]]" "$dir/gdb.log"; then
    dump_failure "AVR32 LLVM C smoke failed: PC did not reach done loop ${done_pc}" \
      "$dir/gdb.log" "$dir/qemu.log" "$dir/objdump.txt"
    exit 1
  fi

  echo "AVR32 LLVM C smoke passed: answer(7,26) returned 42 at ${done_pc}"
}

run_asm_smoke
run_llvm_c_smoke
