#!/usr/bin/env bash
set -euo pipefail

script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
repo_root=$(cd "${script_dir}/.." && pwd)

AVR32_GNU_PREFIX=${AVR32_GNU_PREFIX:-/Users/cozy/cozycactus/avr32-toolchain-macos-arm64/avr32-tools-src/bin/avr32-}
AVR32_AS=${AVR32_AS:-${AVR32_GNU_PREFIX}as}
AVR32_LD=${AVR32_LD:-${AVR32_GNU_PREFIX}ld}
AVR32_OBJDUMP=${AVR32_OBJDUMP:-${AVR32_GNU_PREFIX}objdump}
AVR32_CLANG=${AVR32_CLANG:-${repo_root}/build-avr32/bin/clang}
AVR32_GCC=${AVR32_GCC:-${AVR32_GNU_PREFIX}gcc}
AVR32_GDB=${AVR32_GDB:-/tmp/avr32-gdb-build/gdb/gdb}
AVR32_QEMU=${AVR32_QEMU:-/tmp/qemu-avr32-build/qemu-system-avr32}
AVR32_GDB_PORT=${AVR32_GDB_PORT:-1234}
AVR32_FLASH_BASE=${AVR32_FLASH_BASE:-0x80000000}
AVR32_SRAM_BASE=${AVR32_SRAM_BASE:-0x00000000}
AVR32_SRAM_TOP=${AVR32_SRAM_TOP:-0x00010000}

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
require_executable "$AVR32_GCC"
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

  . = ${AVR32_SRAM_BASE};

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

symbol_address_from_objdump() {
  local symbol=$1
  local objdump=$2
  local address
  address=$(awk -v sym="$symbol" \
    'index($0, "<" sym ">:") { print "0x" $1; exit }' "$objdump")
  if [[ -z "$address" ]]; then
    echo "could not find symbol ${symbol} in ${objdump}" >&2
    exit 1
  fi
  echo "$address"
}

compile_memory_c() {
  local compiler=$1
  local dir=$2

  case "$compiler" in
  llvm)
    "$AVR32_CLANG" --target=avr32 -mpart=uc3a3256 -Oz \
      -ffreestanding -fno-builtin \
      -c "$dir/memory.c" -o "$dir/memory.o"
    ;;
  gcc)
    "$AVR32_GCC" -mpart=uc3a3256 -Os \
      -ffreestanding -fno-builtin \
      -c "$dir/memory.c" -o "$dir/memory.o"
    ;;
  *)
    echo "unknown memory smoke compiler: $compiler" >&2
    exit 1
    ;;
  esac
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
  done_pc=$(symbol_address_from_objdump done "$dir/objdump.txt")

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

run_memory_smoke_variant() {
  local compiler=$1
  local label=$2
  local dir="$tmpdir/${compiler}-memory"
  local done_pc
  local sram_top_hi
  local -a step_args=()
  mkdir -p "$dir"

  if (( (AVR32_SRAM_TOP & 0xffff) != 0 )); then
    echo "AVR32_SRAM_TOP must be 64 KiB aligned for the current start shim" >&2
    exit 1
  fi
  sram_top_hi=$(printf "0x%x" "$((AVR32_SRAM_TOP >> 16))")

  cat > "$dir/memory.c" <<'C'
volatile int sink;

__attribute__((noinline)) static int fill(int *p, int x) {
  p[0] = x + 1;
  p[1] = p[0] * 2;
  p[2] = p[1] + x;
  return p[2];
}

int memory_test(int x) {
  int local[3];
  int y = fill(local, x);
  sink = y + local[0];
  return sink - 1;
}
C

  cat > "$dir/start.S" <<ASM
        .section .text
        .global _start
        .type _start, @function
_start:
        movh    sp, ${sram_top_hi}
        mov     r12, 10
        rcall   memory_test
done:
        rjmp    done

        .section .data
ASM

  write_linker_script "$dir/smoke.ld"
  compile_memory_c "$compiler" "$dir"
  "$AVR32_AS" -o "$dir/start.o" "$dir/start.S"
  "$AVR32_LD" -T "$dir/smoke.ld" \
    -o "$dir/smoke.elf" "$dir/start.o" "$dir/memory.o"
  "$AVR32_OBJDUMP" -dr "$dir/smoke.elf" > "$dir/objdump.txt"
  done_pc=$(symbol_address_from_objdump done "$dir/objdump.txt")

  for _ in {1..60}; do
    step_args+=(-ex "si")
  done

  start_qemu "$dir/smoke.elf" "$dir/qemu.log"
  run_gdb "$dir/smoke.elf" "$dir/gdb.log" \
    -ex "x/20i \$pc" \
    "${step_args[@]}" \
    -ex "info registers pc sp r8 r9 r11 r12" \
    -ex "x/wx ${AVR32_SRAM_BASE}" \
    -ex "detach"
  stop_qemu

  if ! grep -Eq "r12[[:space:]]+0x2a[[:space:]]+42" "$dir/gdb.log"; then
    dump_failure "AVR32 ${label} memory smoke failed: r12 did not become 42" \
      "$dir/gdb.log" "$dir/qemu.log" "$dir/objdump.txt"
    exit 1
  fi

  if ! grep -Eq "pc[[:space:]]+${done_pc}[[:space:]]" "$dir/gdb.log"; then
    dump_failure "AVR32 ${label} memory smoke failed: PC did not reach done loop ${done_pc}" \
      "$dir/gdb.log" "$dir/qemu.log" "$dir/objdump.txt"
    exit 1
  fi

  if ! grep -Eq "0x0000002b" "$dir/gdb.log"; then
    dump_failure "AVR32 ${label} memory smoke failed: SRAM sink did not become 43" \
      "$dir/gdb.log" "$dir/qemu.log" "$dir/objdump.txt"
    exit 1
  fi

  echo "AVR32 ${label} memory smoke passed: stack, call, lddpc, and SRAM sink at ${AVR32_SRAM_BASE}"
}

run_memory_comparison_smoke() {
  run_memory_smoke_variant llvm LLVM
  run_memory_smoke_variant gcc GCC
  echo "AVR32 LLVM/GCC memory comparison passed: both returned 42 and stored 43 in SRAM"
}

run_asm_smoke
run_llvm_c_smoke
run_memory_comparison_smoke
