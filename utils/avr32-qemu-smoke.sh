#!/usr/bin/env bash
set -euo pipefail

AVR32_GNU_PREFIX=${AVR32_GNU_PREFIX:-/Users/cozy/cozycactus/avr32-toolchain-macos-arm64/avr32-tools-src/bin/avr32-}
AVR32_AS=${AVR32_AS:-${AVR32_GNU_PREFIX}as}
AVR32_LD=${AVR32_LD:-${AVR32_GNU_PREFIX}ld}
AVR32_OBJDUMP=${AVR32_OBJDUMP:-${AVR32_GNU_PREFIX}objdump}
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
require_executable "$AVR32_GDB"
require_executable "$AVR32_QEMU"

tmpdir=$(mktemp -d "${TMPDIR:-/tmp}/avr32-qemu-smoke.XXXXXX")
qemu_pid=

cleanup() {
  if [[ -n "$qemu_pid" ]] && kill -0 "$qemu_pid" 2>/dev/null; then
    kill "$qemu_pid" 2>/dev/null || true
    wait "$qemu_pid" 2>/dev/null || true
  fi
  rm -rf "$tmpdir"
}
trap cleanup EXIT

cat > "$tmpdir/smoke.S" <<'ASM'
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

cat > "$tmpdir/smoke.ld" <<EOF
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

"$AVR32_AS" -o "$tmpdir/smoke.o" "$tmpdir/smoke.S"
"$AVR32_LD" -T "$tmpdir/smoke.ld" -o "$tmpdir/smoke.elf" "$tmpdir/smoke.o"
"$AVR32_OBJDUMP" -dr "$tmpdir/smoke.elf" > "$tmpdir/objdump.txt"

"$AVR32_QEMU" \
  -M avr32example-board \
  -nographic \
  -S \
  -gdb "tcp::${AVR32_GDB_PORT}" \
  -bios "$tmpdir/smoke.elf" \
  > "$tmpdir/qemu.log" 2>&1 &
qemu_pid=$!

for _ in 1 2 3 4 5 6 7 8 9 10; do
  if grep -q "Loaded boot image successfully" "$tmpdir/qemu.log"; then
    break
  fi
  if ! kill -0 "$qemu_pid" 2>/dev/null; then
    echo "QEMU exited before GDB attach" >&2
    cat "$tmpdir/qemu.log" >&2
    exit 1
  fi
  sleep 0.2
done

"$AVR32_GDB" --batch "$tmpdir/smoke.elf" \
  -ex "set confirm off" \
  -ex "set architecture avr32" \
  -ex "target remote :${AVR32_GDB_PORT}" \
  -ex "info registers pc r12" \
  -ex "x/4i \$pc" \
  -ex "si" \
  -ex "info registers pc r12" \
  -ex "detach" \
  > "$tmpdir/gdb.log" 2>&1

if ! grep -Eq "r12[[:space:]]+0x2a[[:space:]]+42" "$tmpdir/gdb.log"; then
  echo "AVR32 QEMU smoke failed: r12 did not become 42" >&2
  cat "$tmpdir/gdb.log" >&2
  echo "--- qemu log ---" >&2
  cat "$tmpdir/qemu.log" >&2
  echo "--- objdump ---" >&2
  cat "$tmpdir/objdump.txt" >&2
  exit 1
fi

echo "AVR32 QEMU smoke passed: stepped mov r12,42 at ${AVR32_FLASH_BASE}"
