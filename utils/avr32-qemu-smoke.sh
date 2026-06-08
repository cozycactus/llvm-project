#!/usr/bin/env bash
set -euo pipefail

script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
repo_root=$(cd "${script_dir}/.." && pwd)

AVR32_GNU_PREFIX=${AVR32_GNU_PREFIX:-/Users/cozy/cozycactus/avr32-toolchain-macos-arm64/avr32-tools-src/bin/avr32-}
AVR32_AS=${AVR32_AS:-${AVR32_GNU_PREFIX}as}
AVR32_LD=${AVR32_LD:-${AVR32_GNU_PREFIX}ld}
AVR32_NM=${AVR32_NM:-${AVR32_GNU_PREFIX}nm}
AVR32_OBJDUMP=${AVR32_OBJDUMP:-${AVR32_GNU_PREFIX}objdump}
AVR32_CLANG=${AVR32_CLANG:-${repo_root}/build-avr32/bin/clang}
AVR32_GCC=${AVR32_GCC:-${AVR32_GNU_PREFIX}gcc}
AVR32_GDB=${AVR32_GDB:-/tmp/avr32-gdb-build/gdb/gdb}
AVR32_QEMU=${AVR32_QEMU:-/tmp/qemu-avr32-build/qemu-system-avr32}
SDR_WIDGET_ROOT=${SDR_WIDGET_ROOT:-/Users/cozy/cozycactus/sdr-widget}
AVR32_LLVM_OPT=${AVR32_LLVM_OPT:--Oz}
AVR32_GCC_OPT=${AVR32_GCC_OPT:--Os}
AVR32_GDB_PORT=${AVR32_GDB_PORT:-1234}
AVR32_FLASH_BASE=${AVR32_FLASH_BASE:-0x80000000}
AVR32_SRAM_BASE=${AVR32_SRAM_BASE:-0x00000000}
AVR32_SRAM_TOP=${AVR32_SRAM_TOP:-0x00010000}
AVR32_KEEP_TMP=${AVR32_KEEP_TMP:-0}

require_executable() {
  local path=$1
  if [[ ! -x "$path" ]]; then
    echo "missing executable: $path" >&2
    exit 1
  fi
}

require_executable "$AVR32_AS"
require_executable "$AVR32_LD"
require_executable "$AVR32_NM"
require_executable "$AVR32_OBJDUMP"
require_executable "$AVR32_CLANG"
require_executable "$AVR32_GCC"
require_executable "$AVR32_GDB"
require_executable "$AVR32_QEMU"
AVR32_SYSROOT=${AVR32_SYSROOT:-$("$AVR32_GCC" -print-sysroot)}

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
  if [[ "$AVR32_KEEP_TMP" == 1 ]]; then
    echo "Keeping AVR32 QEMU smoke artifacts in ${tmpdir}" >&2
  else
    rm -rf "$tmpdir"
  fi
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

symbol_address_from_nm() {
  local symbol=$1
  local elf=$2
  local address
  address=$("$AVR32_NM" "$elf" | awk -v sym="$symbol" '$3 == sym {
    print "0x" $1
    exit
  }')
  if [[ -z "$address" ]]; then
    echo "could not find symbol ${symbol} in ${elf}" >&2
    exit 1
  fi
  echo "$address"
}

compile_case_c() {
  local compiler=$1
  local dir=$2

  case "$compiler" in
  llvm)
    "$AVR32_CLANG" --target=avr32 -mpart=uc3a3256 "$AVR32_LLVM_OPT" \
      -ffreestanding -fno-builtin \
      -c "$dir/case.c" -o "$dir/case.o"
    ;;
  gcc)
    "$AVR32_GCC" -mpart=uc3a3256 "$AVR32_GCC_OPT" \
      -ffreestanding -fno-builtin \
      -c "$dir/case.c" -o "$dir/case.o"
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

write_start_shim() {
  local path=$1
  local entry=$2
  local sram_top_hi

  if (( (AVR32_SRAM_TOP & 0xffff) != 0 )); then
    echo "AVR32_SRAM_TOP must be 64 KiB aligned for the current start shim" >&2
    exit 1
  fi
  sram_top_hi=$(printf "0x%x" "$((AVR32_SRAM_TOP >> 16))")

  cat > "$path" <<ASM
        .section .text
        .global _start
        .type _start, @function
_start:
        movh    sp, ${sram_top_hi}
        rcall   ${entry}
done:
        rjmp    done

        .section .data
ASM
}

run_compare_case_variant() {
  local compiler=$1
  local label=$2
  local name=$3
  local source=$4
  local expected_return=$5
  local expected_sink=$6
  local step_count=$7
  local dir="$tmpdir/${compiler}-${name}"
  local done_pc
  local sink_addr
  local expected_return_hex
  local expected_sink_hex
  local -a step_args=()
  mkdir -p "$dir"

  cp "$source" "$dir/case.c"
  write_linker_script "$dir/smoke.ld"
  write_start_shim "$dir/start.S" test_entry
  compile_case_c "$compiler" "$dir"
  "$AVR32_AS" -o "$dir/start.o" "$dir/start.S"
  "$AVR32_LD" -T "$dir/smoke.ld" \
    -o "$dir/smoke.elf" "$dir/start.o" "$dir/case.o"
  "$AVR32_OBJDUMP" -dr "$dir/smoke.elf" > "$dir/objdump.txt"
  done_pc=$(symbol_address_from_objdump done "$dir/objdump.txt")
  sink_addr=$(symbol_address_from_nm sink "$dir/smoke.elf")
  expected_return_hex=$(printf "0x%x" "$expected_return")
  expected_sink_hex=$(printf "0x%08x" "$expected_sink")

  for ((i = 0; i < step_count; ++i)); do
    step_args+=(-ex "si")
  done

  start_qemu "$dir/smoke.elf" "$dir/qemu.log"
  run_gdb "$dir/smoke.elf" "$dir/gdb.log" \
    -ex "x/24i \$pc" \
    "${step_args[@]}" \
    -ex "info registers pc sp r8 r9 r10 r11 r12" \
    -ex "x/wx ${sink_addr}" \
    -ex "detach"
  stop_qemu

  if ! grep -Eq "r12[[:space:]]+${expected_return_hex}[[:space:]]+${expected_return}" "$dir/gdb.log"; then
    dump_failure "AVR32 ${label} ${name} failed: r12 did not become ${expected_return}" \
      "$dir/gdb.log" "$dir/qemu.log" "$dir/objdump.txt"
    exit 1
  fi

  if ! grep -Eq "pc[[:space:]]+${done_pc}[[:space:]]" "$dir/gdb.log"; then
    dump_failure "AVR32 ${label} ${name} failed: PC did not reach done loop ${done_pc}" \
      "$dir/gdb.log" "$dir/qemu.log" "$dir/objdump.txt"
    exit 1
  fi

  if ! grep -Eq "${expected_sink_hex}" "$dir/gdb.log"; then
    dump_failure "AVR32 ${label} ${name} failed: sink did not become ${expected_sink}" \
      "$dir/gdb.log" "$dir/qemu.log" "$dir/objdump.txt"
    exit 1
  fi

  echo "AVR32 ${label} ${name} passed: r12=${expected_return}, sink=${expected_sink}"
}

run_compare_case() {
  local name=$1
  local expected_return=$2
  local expected_sink=$3
  local step_count=$4
  local source="$tmpdir/${name}.c"

  cat > "$source"
  run_compare_case_variant llvm LLVM "$name" "$source" \
    "$expected_return" "$expected_sink" "$step_count"
  run_compare_case_variant gcc GCC "$name" "$source" \
    "$expected_return" "$expected_sink" "$step_count"
}

compile_real_sdr_c() {
  local compiler=$1
  local dir=$2
  local source=$3
  local object=$4
  local -a common_flags=(
    -std=gnu89
    -fcommon
    -ffreestanding
    -fno-builtin
    -Wno-expansion-to-defined
    -I "$dir/include"
  )

  if [[ -n "$AVR32_SYSROOT" ]]; then
    common_flags+=("--sysroot=$AVR32_SYSROOT")
  fi

  case "$compiler" in
  llvm)
    "$AVR32_CLANG" --target=avr32 -mpart=uc3a3256 "$AVR32_LLVM_OPT" \
      "${common_flags[@]}" -c "$source" -o "$object"
    ;;
  gcc)
    "$AVR32_GCC" -mpart=uc3a3256 "$AVR32_GCC_OPT" \
      "${common_flags[@]}" -c "$source" -o "$object"
    ;;
  *)
    echo "unknown real SDR smoke compiler: $compiler" >&2
    exit 1
    ;;
  esac
}

write_real_sdr_headers() {
  local dir=$1
  local include="$dir/include"
  mkdir -p "$include"

  for header in wdt.h taskLCD.h usb_standard_request.h preprocessor.h usbb.h conf_usb.h; do
    : > "$include/$header"
  done

  cat > "$include/compiler.h" <<'C'
#ifndef QEMU_SDR_COMPILER_H
#define QEMU_SDR_COMPILER_H

#include <stddef.h>
#include <stdint.h>

typedef unsigned char Bool;
#ifndef __cplusplus
#if !defined(__bool_true_false_are_defined)
typedef unsigned char bool;
#endif
#endif
typedef signed char S8;
typedef unsigned char U8;
typedef signed short S16;
typedef unsigned short U16;
typedef signed long S32;
typedef unsigned long U32;
typedef signed long long S64;
typedef unsigned long long U64;
typedef Bool Status_bool_t;
typedef U8 Status_t;

typedef union {
  S16 s16;
  U16 u16;
  S8 s8[2];
  U8 u8[2];
} Union16;

typedef union {
  S32 s32;
  U32 u32;
  S16 s16[2];
  U16 u16[2];
  S8 s8[4];
  U8 u8[4];
} Union32;

typedef union {
  S64 s64;
  U64 u64;
  S32 s32[2];
  U32 u32[2];
  S16 s16[4];
  U16 u16[4];
  S8 s8[8];
  U8 u8[8];
} Union64;

typedef union {
  S64 *s64ptr;
  U64 *u64ptr;
  S32 *s32ptr;
  U32 *u32ptr;
  S16 *s16ptr;
  U16 *u16ptr;
  S8 *s8ptr;
  U8 *u8ptr;
} UnionPtr;

typedef union {
  volatile S64 *s64ptr;
  volatile U64 *u64ptr;
  volatile S32 *s32ptr;
  volatile U32 *u32ptr;
  volatile S16 *s16ptr;
  volatile U16 *u16ptr;
  volatile S8 *s8ptr;
  volatile U8 *u8ptr;
} UnionVPtr;

typedef union {
  const S64 *s64ptr;
  const U64 *u64ptr;
  const S32 *s32ptr;
  const U32 *u32ptr;
  const S16 *s16ptr;
  const U16 *u16ptr;
  const S8 *s8ptr;
  const U8 *u8ptr;
} UnionCPtr;

typedef union {
  const volatile S64 *s64ptr;
  const volatile U64 *u64ptr;
  const volatile S32 *s32ptr;
  const volatile U32 *u32ptr;
  const volatile S16 *s16ptr;
  const volatile U16 *u16ptr;
  const volatile S8 *s8ptr;
  const volatile U8 *u8ptr;
} UnionCVPtr;

typedef struct {
  S64 *s64ptr;
  U64 *u64ptr;
  S32 *s32ptr;
  U32 *u32ptr;
  S16 *s16ptr;
  U16 *u16ptr;
  S8 *s8ptr;
  U8 *u8ptr;
} StructPtr;

typedef struct {
  volatile S64 *s64ptr;
  volatile U64 *u64ptr;
  volatile S32 *s32ptr;
  volatile U32 *u32ptr;
  volatile S16 *s16ptr;
  volatile U16 *u16ptr;
  volatile S8 *s8ptr;
  volatile U8 *u8ptr;
} StructVPtr;

typedef struct {
  const S64 *s64ptr;
  const U64 *u64ptr;
  const S32 *s32ptr;
  const U32 *u32ptr;
  const S16 *s16ptr;
  const U16 *u16ptr;
  const S8 *s8ptr;
  const U8 *u8ptr;
} StructCPtr;

typedef struct {
  const volatile S64 *s64ptr;
  const volatile U64 *u64ptr;
  const volatile S32 *s32ptr;
  const volatile U32 *u32ptr;
  const volatile S16 *s16ptr;
  const volatile U16 *u16ptr;
  const volatile S8 *s8ptr;
  const volatile U8 *u8ptr;
} StructCVPtr;

#define DISABLED 0
#define ENABLED 1
#define FALSE 0
#define TRUE 1
#define KO 0
#define OK 1

#ifndef false
#define false FALSE
#endif
#ifndef true
#define true TRUE
#endif

#define Rd_bits(value, mask) ((value) & (mask))
#define Tst_bits(value, mask) (Rd_bits((value), (mask)) != 0)
#define Test_align(value, align) (!Tst_bits((uintptr_t)(value), (uintptr_t)((align) - 1U)))
#define Get_align(value, align) (Rd_bits((uintptr_t)(value), (uintptr_t)((align) - 1U)))
#define Align_down(value, align) ((uintptr_t)(value) & ~(uintptr_t)((align) - 1U))
#define min(a, b) ((a) < (b) ? (a) : (b))

#endif
C

  cat > "$include/usb_drv.h" <<'C'
#ifndef QEMU_SDR_USB_DRV_H
#define QEMU_SDR_USB_DRV_H

#include "compiler.h"

#define USB_DEVICE_FEATURE ENABLED
#define USB_HOST_FEATURE DISABLED
#define MAX_PEP_NB 8

#define EP_CONTROL 0
#define TYPE_CONTROL 0
#define DIRECTION_OUT 0
#define SINGLE_BANK 0

extern UnionVPtr pep_fifo[MAX_PEP_NB];
extern U32 qemu_usb_endpoint_size[MAX_PEP_NB];
extern U32 qemu_usb_byte_count[MAX_PEP_NB];

#define Usb_get_endpoint_size(ep) (qemu_usb_endpoint_size[(ep)])
#define Usb_byte_count(ep) (qemu_usb_byte_count[(ep)])
#define Is_usb_id_device() TRUE
#define Is_usb_endpoint_enabled(ep) FALSE
static inline Status_bool_t qemu_usb_configure_endpoint(
    U8 ep, U8 type, U8 dir, U16 size, U8 banks, U8 flags) {
  (void)ep;
  (void)type;
  (void)dir;
  (void)size;
  (void)banks;
  (void)flags;
  return TRUE;
}
#define Usb_configure_endpoint(ep, type, dir, size, banks, flags) \
  qemu_usb_configure_endpoint((ep), (type), (dir), (size), (banks), (flags))

Status_bool_t usb_init_device(void);
U32 usb_set_ep_txpacket(U8 ep, U8 txbyte, U32 data_length);
U32 usb_write_ep_txpacket(U8 ep, const void *txbuf, U32 data_length, const void **ptxbuf);
U32 usb_read_ep_rxpacket(U8 ep, void *rxbuf, U32 data_length, void **prxbuf);

#endif
C

  cat > "$include/usb_descriptors.h" <<'C'
#ifndef QEMU_SDR_USB_DESCRIPTORS_H
#define QEMU_SDR_USB_DESCRIPTORS_H
#define EP_CONTROL_LENGTH 64
#endif
C

  cat > "$include/flashc.h" <<'C'
#ifndef QEMU_SDR_FLASHC_H
#define QEMU_SDR_FLASHC_H

#include <stddef.h>
#include "compiler.h"

volatile void *flashc_memset8(volatile void *dst, U8 src, size_t nbytes, Bool erase);
volatile void *flashc_memset16(volatile void *dst, U16 src, size_t nbytes, Bool erase);
volatile void *flashc_memset32(volatile void *dst, U32 src, size_t nbytes, Bool erase);

#endif
C

  cat > "$include/gpio.h" <<'C'
#ifndef QEMU_SDR_GPIO_H
#define QEMU_SDR_GPIO_H

#define GPIO_CW_KEY_1 0
#define GPIO_CW_KEY_2 1
#define GPIO_PTT_INPUT 2
#define PTT_1 3
#define PTT_2 4
#define PTT_3 5

int gpio_get_pin_value(unsigned int pin);

#endif
C

  cat > "$include/Si570.h" <<'C'
#ifndef QEMU_SDR_SI570_H
#define QEMU_SDR_SI570_H

#include "compiler.h"

extern U8 si570reg[6];
void GetRegFromSi570(U8 addr);

#endif
C

  cat > "$include/AD7991.h" <<'C'
#ifndef QEMU_SDR_AD7991_H
#define QEMU_SDR_AD7991_H

#include "compiler.h"

extern U16 ad7991_adc[4];

#endif
C

  cat > "$include/TMP100.h" <<'C'
#ifndef QEMU_SDR_TMP100_H
#define QEMU_SDR_TMP100_H

#include "compiler.h"

extern S16 tmp100_data;

#endif
C

  cat > "$include/features.h" <<'C'
#ifndef QEMU_SDR_FEATURES_H
#define QEMU_SDR_FEATURES_H

#include "compiler.h"

typedef enum {
  feature_major_index = 0,
  feature_minor_index,
  feature_board_index,
  feature_image_index,
  feature_in_index,
  feature_out_index,
  feature_adc_index,
  feature_dac_index,
  feature_lcd_index,
  feature_log_index,
  feature_end_index
} feature_index_t;

typedef enum {
  feature_board_none = 0,
  feature_board_widget,
  feature_board_usbi2s,
  feature_board_usbdac,
  feature_board_test,
  feature_end_board,
  feature_image_flashyblinky,
  feature_image_uac1_audio,
  feature_image_uac1_dg8saq,
  feature_image_uac2_audio,
  feature_image_uac2_dg8saq,
  feature_image_hpsdr,
  feature_image_test,
  feature_end_image,
  feature_in_normal,
  feature_in_swapped,
  feature_end_in,
  feature_out_normal,
  feature_out_swapped,
  feature_end_out,
  feature_adc_none,
  feature_adc_ak5394a,
  feature_end_adc,
  feature_dac_none,
  feature_dac_cs4344,
  feature_dac_es9022,
  feature_end_dac,
  feature_lcd_none,
  feature_lcd_hd44780,
  feature_lcd_ks0073,
  feature_end_lcd,
  feature_log_none,
  feature_log_250ms,
  feature_log_500ms,
  feature_log_1sec,
  feature_log_2sec,
  feature_end_log,
  feature_end_values
} feature_values_t;

typedef U8 features_t[feature_end_index];

extern features_t features_nvram;
extern features_t features;
extern const features_t features_default;
extern const char * const feature_value_names[];
extern const char * const feature_index_names[];

U8 feature_set(U8 index, U8 value);
U8 feature_get(U8 index);
U8 feature_set_nvram(U8 index, U8 value);
U8 feature_get_nvram(U8 index);
U8 feature_get_default(U8 index);

void qemu_feature_reset(void);

#endif
C

  cat > "$include/usb_specific_request.h" <<'C'
#ifndef QEMU_SDR_USB_SPECIFIC_REQUEST_H
#define QEMU_SDR_USB_SPECIFIC_REQUEST_H

#include "compiler.h"

typedef union {
  U32 frequency;
  U8 freq_bytes[4];
} S_freq;

extern S_freq current_freq;
extern Bool freq_changed;

#endif
C

  cat > "$include/widget.h" <<'C'
#ifndef QEMU_SDR_WIDGET_H
#define QEMU_SDR_WIDGET_H

void widget_reset(void);
void widget_factory_reset(void);

#endif
C

  cat > "$include/Mobo_config.h" <<'C'
#ifndef QEMU_SDR_MOBO_CONFIG_H
#define QEMU_SDR_MOBO_CONFIG_H

#include <stdint.h>
#include "compiler.h"

#define VERSION_MAJOR 16
#define VERSION_MINOR 100

#define CALC_FREQ_MUL_ADD 0
#define CALC_BAND_MUL_ADD 0
#define SCRAMBLED_FILTERS 1
#define PCF_LPF 0
#define PCF_16LPF 0
#define PCF_FILTER_IO 0
#define M0RZF_FILTER_IO 0
#define TXF 16

#define REG_CWSHORT (1 << 5)
#define REG_CWLONG (1 << 1)
#define REG_PTT_1 (1 << 2)
#define REG_PTT_2 (1 << 3)
#define REG_PTT_3 (1 << 4)
#define REG_TX_state (1 << 6)
#define REG_PTT_INPUT (1 << 7)

extern volatile bool MENU_mode;
extern bool TX_state;
extern bool TX_flag;
extern bool SWR_alarm;
extern bool TMP_alarm;
extern bool PA_cal_lo;
extern bool PA_cal_hi;
extern bool PA_cal;

typedef struct {
  bool si570;
  bool tmp100;
  bool ad5301;
  bool ad7991;
  bool pcfmobo;
  bool pcflpf1;
  bool pcflpf2;
  bool pcfext;
  bool pcf0x20;
  bool pcf0x21;
  bool pcf0x22;
  bool pcf0x23;
  bool pcf0x24;
  bool pcf0x25;
  bool pcf0x26;
  bool pcf0x27;
  bool pcf0x38;
  bool pcf0x39;
  bool pcf0x3a;
  bool pcf0x3b;
  bool pcf0x3c;
  bool pcf0x3d;
  bool pcf0x3e;
  bool pcf0x3f;
} i2c_avail;

extern i2c_avail i2c;

typedef struct {
  U8 EEPROM_init_check;
  U8 UAC2_Audio;
  U8 Si570_I2C_addr;
  U8 TMP100_I2C_addr;
  U8 AD5301_I2C_addr;
  U8 AD7991_I2C_addr;
  U8 PCF_I2C_Mobo_addr;
  U8 PCF_I2C_lpf1_addr;
  U8 PCF_I2C_lpf2_addr;
  U8 PCF_I2C_Ext_addr;
  U8 hi_tmp_trigger;
  U16 P_Min_Trigger;
  U16 SWR_Protect_Timer;
  U16 SWR_Trigger;
  U16 PWR_Calibrate;
  U8 Bias_Select;
  U8 Bias_LO;
  U8 Bias_HI;
  U8 cal_LO;
  U8 cal_HI;
  U32 FreqXtal;
  U16 SmoothTunePPM;
  U32 Freq[10];
  U8 SwitchFreq;
  U16 FilterCrossOver[8];
  U16 TXFilterCrossOver[TXF];
  U8 PWR_fullscale;
  U8 SWR_fullscale;
  U8 PEP_samples;
  U16 Resolvable_States;
  U8 VFO_resolution;
  S8 LCD_RX_Offset;
  U8 Fan_On;
  U8 Fan_Off;
  U8 PCF_fan_bit;
  U8 FilterNumber[8];
  U8 TXFilterNumber[16];
} mobo_data_t;

extern mobo_data_t cdata;
extern mobo_data_t nvram_cdata;

U8 pcf8574_out_byte(U8 i2c_address, U8 data);
U8 pcf8574_in_byte(U8 i2c_address, U8 *data_to_return);

#endif
C

  cat > "$include/DG8SAQ_cmd.h" <<'C'
#ifndef QEMU_SDR_DG8SAQ_CMD_H
#define QEMU_SDR_DG8SAQ_CMD_H

#include <stdint.h>
#include "compiler.h"

void dg8saqFunctionWrite(U8 type, U16 wValue, U16 wIndex, U8 *Buffer, U8 len);
uint8_t dg8saqFunctionSetup(uint8_t type, uint16_t wValue, uint16_t wIndex, U8 *Buffer);

extern volatile uint32_t freq_from_usb;
extern volatile bool FRQ_fromusbreg;
extern volatile bool FRQ_fromusb;
extern volatile bool FRQ_lcdupdate;

#endif
C
}

write_real_sdr_stubs() {
  local dir=$1

  cat > "$dir/stubs.c" <<'C'
#include <stddef.h>
#include "compiler.h"
#include "Mobo_config.h"
#include "Si570.h"
#include "AD7991.h"
#include "TMP100.h"
#include "usb_drv.h"
#include "usb_specific_request.h"
#include "features.h"
#include "DG8SAQ_cmd.h"

volatile int sink;

mobo_data_t cdata;
mobo_data_t nvram_cdata;
i2c_avail i2c;
volatile bool MENU_mode;
bool TX_state;
bool TX_flag;
bool SWR_alarm;
bool TMP_alarm;
bool PA_cal_lo;
bool PA_cal_hi;
bool PA_cal;

U8 si570reg[6];
U16 ad7991_adc[4];
S16 tmp100_data;
S_freq current_freq;
Bool freq_changed;

U32 qemu_usb_endpoint_size[MAX_PEP_NB];
U32 qemu_usb_byte_count[MAX_PEP_NB];
U8 qemu_gpio_state[32];
U8 qemu_pcf_latch[16];
unsigned qemu_widget_resets;
unsigned qemu_factory_resets;

features_t features;
features_t features_nvram;
const features_t features_default = {
  feature_end_index,
  feature_end_values,
  feature_board_widget,
  feature_image_uac1_dg8saq,
  feature_in_normal,
  feature_out_normal,
  feature_adc_ak5394a,
  feature_dac_cs4344,
  feature_lcd_hd44780,
  feature_log_500ms
};
const char * const feature_index_names[] = {
  "major", "minor", "board", "image", "in", "out",
  "adc", "dac", "lcd", "log", "end"
};
const char * const feature_value_names[] = {
  "none", "widget", "usbi2s", "usbdac", "test", "end",
  "flashyblinky", "uac1_audio", "uac1_dg8saq", "uac2_audio",
  "uac2_dg8saq", "hpsdr", "test", "end", "normal", "swapped",
  "end", "normal", "swapped", "end", "none", "ak5394a", "end",
  "none", "cs4344", "es9022", "end", "none", "hd44780", "ks0073",
  "end", "none", "250ms", "500ms", "1sec", "2sec", "end", "end"
};

void *memset(void *dst, int value, size_t n) {
  U8 *p = (U8 *)dst;
  while (n-- != 0)
    *p++ = (U8)value;
  return dst;
}

void *memcpy(void *dst, const void *src, size_t n) {
  U8 *d = (U8 *)dst;
  const U8 *s = (const U8 *)src;
  while (n-- != 0)
    *d++ = *s++;
  return dst;
}

char *strcpy(char *dst, const char *src) {
  char *ret = dst;
  while ((*dst++ = *src++) != '\0')
    ;
  return ret;
}

size_t strlen(const char *s) {
  const char *p = s;
  while (*p != '\0')
    ++p;
  return (size_t)(p - s);
}

__attribute__((noinline)) U64 __avr32_mul64(U64 a, U64 b) {
  volatile U64 lhs = a;
  volatile U64 rhs = b;
  U64 result = 0;

  while (rhs != 0) {
    if (((U32)rhs & 1U) != 0)
      result += lhs;
    lhs += lhs;
    rhs >>= 1;
  }
  return result;
}

static void qemu_zero_bytes(volatile U8 *p, unsigned n) {
  unsigned i;
  for (i = 0; i < n; ++i)
    p[i] = 0;
}

void qemu_feature_reset(void) {
  int i;
  for (i = 0; i < feature_end_index; ++i) {
    features[i] = features_default[i];
    features_nvram[i] = features_default[i];
  }
}

void qemu_sdr_reset_state(void) {
  int i;

  qemu_zero_bytes((volatile U8 *)&cdata, sizeof(cdata));
  qemu_zero_bytes((volatile U8 *)&nvram_cdata, sizeof(nvram_cdata));
  qemu_zero_bytes((volatile U8 *)&i2c, sizeof(i2c));
  qemu_zero_bytes(si570reg, sizeof(si570reg));
  qemu_zero_bytes((volatile U8 *)ad7991_adc, sizeof(ad7991_adc));
  qemu_zero_bytes((volatile U8 *)&tmp100_data, sizeof(tmp100_data));
  qemu_zero_bytes((volatile U8 *)&current_freq, sizeof(current_freq));
  qemu_zero_bytes((volatile U8 *)qemu_usb_endpoint_size, sizeof(qemu_usb_endpoint_size));
  qemu_zero_bytes((volatile U8 *)qemu_usb_byte_count, sizeof(qemu_usb_byte_count));
  qemu_zero_bytes(qemu_gpio_state, sizeof(qemu_gpio_state));
  qemu_zero_bytes(qemu_pcf_latch, sizeof(qemu_pcf_latch));

  sink = 0;
  MENU_mode = FALSE;
  TX_state = FALSE;
  TX_flag = FALSE;
  SWR_alarm = FALSE;
  TMP_alarm = FALSE;
  PA_cal_lo = FALSE;
  PA_cal_hi = FALSE;
  PA_cal = FALSE;
  freq_changed = FALSE;
  qemu_widget_resets = 0;
  qemu_factory_resets = 0;

  freq_from_usb = 0;
  FRQ_fromusbreg = FALSE;
  FRQ_fromusb = FALSE;
  FRQ_lcdupdate = FALSE;

  cdata.Si570_I2C_addr = 0x55;
  cdata.FreqXtal = 114285000U;
  cdata.Freq[0] = 14000000U;
  cdata.SmoothTunePPM = 3500;
  cdata.P_Min_Trigger = 49;
  cdata.SWR_Protect_Timer = 200;
  cdata.SWR_Trigger = 30;
  cdata.PWR_Calibrate = 1000;
  cdata.PWR_fullscale = 4;
  cdata.SWR_fullscale = 4;
  cdata.PEP_samples = 10;
  cdata.Resolvable_States = 96;
  cdata.LCD_RX_Offset = 0;
  cdata.Fan_On = 45;
  cdata.Fan_Off = 40;
  cdata.PCF_fan_bit = 1;
  for (i = 0; i < 8; ++i) {
    cdata.FilterCrossOver[i] = (U16)((i + 1) * 3);
    cdata.FilterNumber[i] = (U8)(i + 1);
  }
  for (i = 0; i < TXF; ++i) {
    cdata.TXFilterCrossOver[i] = (U16)(20 + i);
    cdata.TXFilterNumber[i] = (U8)(15 - i);
  }
  nvram_cdata = cdata;

  i2c.pcf0x21 = TRUE;
  i2c.pcf0x3a = TRUE;
  current_freq.frequency = 48000;
  qemu_feature_reset();
}

volatile void *flashc_memset8(volatile void *dst, U8 src, size_t nbytes, Bool erase) {
  volatile U8 *p = (volatile U8 *)dst;
  (void)erase;
  while (nbytes-- != 0)
    *p++ = src;
  return dst;
}

volatile void *flashc_memset16(volatile void *dst, U16 src, size_t nbytes, Bool erase) {
  volatile U16 *p = (volatile U16 *)dst;
  (void)erase;
  while (nbytes >= sizeof(U16)) {
    *p++ = src;
    nbytes -= sizeof(U16);
  }
  return dst;
}

volatile void *flashc_memset32(volatile void *dst, U32 src, size_t nbytes, Bool erase) {
  volatile U32 *p = (volatile U32 *)dst;
  (void)erase;
  while (nbytes >= sizeof(U32)) {
    *p++ = src;
    nbytes -= sizeof(U32);
  }
  return dst;
}

int gpio_get_pin_value(unsigned int pin) {
  return pin < 32 ? qemu_gpio_state[pin] != 0 : 0;
}

void GetRegFromSi570(U8 addr) {
  int i;
  for (i = 0; i < 6; ++i)
    si570reg[i] = (U8)(addr + i + 1);
}

static int qemu_pcf_index(U8 addr) {
  switch (addr) {
  case 0x20: return i2c.pcf0x20 ? 0 : -1;
  case 0x21: return i2c.pcf0x21 ? 1 : -1;
  case 0x22: return i2c.pcf0x22 ? 2 : -1;
  case 0x23: return i2c.pcf0x23 ? 3 : -1;
  case 0x24: return i2c.pcf0x24 ? 4 : -1;
  case 0x25: return i2c.pcf0x25 ? 5 : -1;
  case 0x26: return i2c.pcf0x26 ? 6 : -1;
  case 0x27: return i2c.pcf0x27 ? 7 : -1;
  case 0x38: return i2c.pcf0x38 ? 8 : -1;
  case 0x39: return i2c.pcf0x39 ? 9 : -1;
  case 0x3a: return i2c.pcf0x3a ? 10 : -1;
  case 0x3b: return i2c.pcf0x3b ? 11 : -1;
  case 0x3c: return i2c.pcf0x3c ? 12 : -1;
  case 0x3d: return i2c.pcf0x3d ? 13 : -1;
  case 0x3e: return i2c.pcf0x3e ? 14 : -1;
  case 0x3f: return i2c.pcf0x3f ? 15 : -1;
  default: return -1;
  }
}

U8 pcf8574_out_byte(U8 i2c_address, U8 data) {
  int index = qemu_pcf_index(i2c_address);
  if (index >= 0)
    qemu_pcf_latch[index] = data;
  return index >= 0 ? OK : KO;
}

U8 pcf8574_in_byte(U8 i2c_address, U8 *data_to_return) {
  int index = qemu_pcf_index(i2c_address);
  if (index >= 0)
    *data_to_return = (U8)(qemu_pcf_latch[index] + i2c_address);
  else
    *data_to_return = 42;
  return index >= 0 ? OK : KO;
}

U8 feature_set(U8 index, U8 value) {
  if (index > feature_minor_index && index < feature_end_index &&
      value < feature_end_values) {
    features[index] = value;
    return features[index];
  }
  return 0xff;
}

U8 feature_get(U8 index) {
  return index < feature_end_index ? features[index] : 0xff;
}

U8 feature_set_nvram(U8 index, U8 value) {
  if (index > feature_minor_index && index < feature_end_index &&
      value < feature_end_values) {
    features_nvram[index] = value;
    return features_nvram[index];
  }
  return 0xff;
}

U8 feature_get_nvram(U8 index) {
  return index < feature_end_index ? features_nvram[index] : 0xff;
}

U8 feature_get_default(U8 index) {
  return index < feature_end_index ? features_default[index] : 0xff;
}

void widget_reset(void) {
  ++qemu_widget_resets;
}

void widget_factory_reset(void) {
  ++qemu_factory_resets;
}
C
}

run_real_sdr_case_variant() {
  local compiler=$1
  local label=$2
  local name=$3
  local source=$4
  local expected_return=$5
  local expected_sink=$6
  local step_count=$7
  local dir="$tmpdir/${compiler}-${name}"
  local done_pc
  local sink_addr
  local expected_return_hex
  local expected_sink_hex
  local -a step_args=()
  local -a objects=()
  mkdir -p "$dir"

  if [[ ! -f "$SDR_WIDGET_ROOT/src/DG8SAQ_cmd.c" ]]; then
    echo "missing SDR-widget source: $SDR_WIDGET_ROOT/src/DG8SAQ_cmd.c" >&2
    exit 1
  fi
  if [[ ! -f "$SDR_WIDGET_ROOT/src/SOFTWARE_FRAMEWORK/DRIVERS/USBB/usb_drv.c" ]]; then
    echo "missing SDR-widget source: $SDR_WIDGET_ROOT/src/SOFTWARE_FRAMEWORK/DRIVERS/USBB/usb_drv.c" >&2
    exit 1
  fi

  cp "$source" "$dir/case.c"
  cp "$SDR_WIDGET_ROOT/src/DG8SAQ_cmd.c" "$dir/DG8SAQ_cmd.c"
  cp "$SDR_WIDGET_ROOT/src/SOFTWARE_FRAMEWORK/DRIVERS/USBB/usb_drv.c" "$dir/usb_drv.c"
  write_real_sdr_headers "$dir"
  write_real_sdr_stubs "$dir"
  write_linker_script "$dir/smoke.ld"
  write_start_shim "$dir/start.S" test_entry

  compile_real_sdr_c "$compiler" "$dir" "$dir/case.c" "$dir/case.o"
  compile_real_sdr_c "$compiler" "$dir" "$dir/stubs.c" "$dir/stubs.o"
  compile_real_sdr_c "$compiler" "$dir" "$dir/DG8SAQ_cmd.c" "$dir/DG8SAQ_cmd.o"
  compile_real_sdr_c "$compiler" "$dir" "$dir/usb_drv.c" "$dir/usb_drv.o"
  "$AVR32_AS" -o "$dir/start.o" "$dir/start.S"
  objects=("$dir/start.o" "$dir/case.o" "$dir/stubs.o" "$dir/DG8SAQ_cmd.o" "$dir/usb_drv.o")
  "$AVR32_LD" -T "$dir/smoke.ld" -o "$dir/smoke.elf" "${objects[@]}"
  "$AVR32_OBJDUMP" -dr "$dir/smoke.elf" > "$dir/objdump.txt"
  done_pc=$(symbol_address_from_objdump done "$dir/objdump.txt")
  sink_addr=$(symbol_address_from_nm sink "$dir/smoke.elf")
  expected_return_hex=$(printf "0x%x" "$expected_return")
  expected_sink_hex=$(printf "0x%08x" "$expected_sink")

  for ((i = 0; i < step_count; ++i)); do
    step_args+=(-ex "si")
  done

  start_qemu "$dir/smoke.elf" "$dir/qemu.log"
  run_gdb "$dir/smoke.elf" "$dir/gdb.log" \
    -ex "x/32i \$pc" \
    "${step_args[@]}" \
    -ex "info registers pc sp r8 r9 r10 r11 r12" \
    -ex "x/wx ${sink_addr}" \
    -ex "detach"
  stop_qemu

  if ! grep -Eq "r12[[:space:]]+${expected_return_hex}[[:space:]]+${expected_return}" "$dir/gdb.log"; then
    dump_failure "AVR32 ${label} ${name} failed: r12 did not become ${expected_return}" \
      "$dir/gdb.log" "$dir/qemu.log" "$dir/objdump.txt"
    exit 1
  fi

  if ! grep -Eq "pc[[:space:]]+${done_pc}[[:space:]]" "$dir/gdb.log"; then
    dump_failure "AVR32 ${label} ${name} failed: PC did not reach done loop ${done_pc}" \
      "$dir/gdb.log" "$dir/qemu.log" "$dir/objdump.txt"
    exit 1
  fi

  if ! grep -Eq "${expected_sink_hex}" "$dir/gdb.log"; then
    dump_failure "AVR32 ${label} ${name} failed: sink did not become ${expected_sink}" \
      "$dir/gdb.log" "$dir/qemu.log" "$dir/objdump.txt"
    exit 1
  fi

  echo "AVR32 ${label} ${name} passed: r12=${expected_return}, sink=${expected_sink}"
}

run_real_sdr_case() {
  local name=$1
  local expected_return=$2
  local expected_sink=$3
  local step_count=$4
  local source="$tmpdir/${name}.c"

  cat > "$source"
  run_real_sdr_case_variant llvm LLVM "$name" "$source" \
    "$expected_return" "$expected_sink" "$step_count"
  run_real_sdr_case_variant gcc GCC "$name" "$source" \
    "$expected_return" "$expected_sink" "$step_count"
}

run_memory_comparison_smoke() {
  local cases=0

  run_compare_case stack_global 42 43 120 <<'C'
volatile int sink;

__attribute__((noinline)) static int fill(int *p, int x) {
  p[0] = x + 1;
  p[1] = p[0] * 2;
  p[2] = p[1] + x;
  return p[2];
}

int test_entry(void) {
  int local[3];
  int y;
  y = fill(local, 10);
  sink = y + local[0];
  return sink - 1;
}
C
  cases=$((cases + 1))

  run_compare_case branches 42 43 160 <<'C'
volatile int sink;

__attribute__((noinline)) static int branch_mix(int a, int b, unsigned int u) {
  int r;
  int i;
  r = 0;
  if (a < b)
    r += 11;
  else
    r += 100;
  if ((unsigned int)a > u)
    r += 13;
  else
    r -= 7;
  for (i = 0; i < 5; ++i) {
    if ((i & 1) == 0)
      r += b;
    else
      r += a;
  }
  return r;
}

int test_entry(void) {
  int v;
  v = branch_mix(-3, 8, 10U);
  sink = v + 1;
  return sink - 1;
}
C
  cases=$((cases + 1))

  run_compare_case mul_shift 42 43 120 <<'C'
volatile int sink;

__attribute__((noinline)) static int mul_shift(int x) {
  int y;
  int z;
  y = x - 1;
  z = x * y;
  z += ((unsigned int)z >> 5);
  z -= 1;
  return z;
}

int test_entry(void) {
  int v;
  v = mul_shift(7);
  sink = v + 1;
  return sink - 1;
}
C
  cases=$((cases + 1))

  run_compare_case pointer_volatile 42 43 220 <<'C'
volatile int sink;
volatile int data[4];

__attribute__((noinline)) static int ptr_walk(volatile int *p) {
  int i;
  int acc;
  acc = 0;
  for (i = 0; i < 4; ++i) {
    p[i] = i + 9;
    acc += p[i];
  }
  return acc;
}

int test_entry(void) {
  int v;
  v = ptr_walk(data);
  sink = v + 1;
  return sink - 1;
}
C
  cases=$((cases + 1))

  run_compare_case postinc_sum 42 43 220 <<'C'
volatile int sink;

__attribute__((noinline)) static int sum_inc(int *p, int n) {
  int s;
  s = 0;
  while (n > 0) {
    s += *p;
    ++p;
    --n;
  }
  return s;
}

int test_entry(void) {
  int buf[4];
  int v;
  buf[0] = 5;
  buf[1] = 8;
  buf[2] = 13;
  buf[3] = 16;
  v = sum_inc(buf, 4);
  sink = v + 1;
  return sink - 1;
}
C
  cases=$((cases + 1))

  echo "AVR32 LLVM/GCC differential comparison passed: ${cases} cases returned 42 and stored 43 in SRAM"
}

run_synthesized_comparison_smoke() {
  local cases=0

  run_compare_case synth_cond_matrix 42 43 520 <<'C'
typedef unsigned int u32;

volatile int sink;
static int slots[12];

__attribute__((noinline)) static void store_eq(
    int *p, int lhs, int rhs, int value) {
  if (lhs == rhs)
    *p = value;
}

__attribute__((noinline)) static void store_ne(
    int *p, int lhs, int rhs, int value) {
  if (lhs != rhs)
    *p = value;
}

__attribute__((noinline)) static void store_slt(
    int *p, int lhs, int rhs, int value) {
  if (lhs < rhs)
    *p = value;
}

__attribute__((noinline)) static void store_sge(
    int *p, int lhs, int rhs, int value) {
  if (lhs >= rhs)
    *p = value;
}

__attribute__((noinline)) static void store_ugt(
    int *p, u32 lhs, u32 rhs, int value) {
  if (lhs > rhs)
    *p = value;
}

__attribute__((noinline)) static void store_ule(
    int *p, u32 lhs, u32 rhs, int value) {
  if (lhs <= rhs)
    *p = value;
}

int test_entry(void) {
  int acc;

  store_eq(&slots[0], 4, 4, 3);
  store_eq(&slots[1], 4, 5, 99);
  store_ne(&slots[2], 4, 5, 5);
  store_ne(&slots[3], 4, 4, 99);
  store_slt(&slots[4], -2, 3, 7);
  store_slt(&slots[5], 5, 1, 99);
  store_sge(&slots[6], 6, 6, 11);
  store_sge(&slots[7], -1, 8, 99);
  store_ugt(&slots[8], 9U, 2U, 13);
  store_ugt(&slots[9], 1U, 4U, 99);
  store_ule(&slots[10], 3U, 3U, 17);
  store_ule(&slots[11], 8U, 2U, 99);

  acc = slots[0] + slots[1] + slots[2] + slots[3];
  acc += slots[4] + slots[5] + slots[6] + slots[7];
  acc += slots[8] + slots[9] + slots[10] + slots[11];
  sink = acc - 13;
  return sink - 1;
}
C
  cases=$((cases + 1))

  run_compare_case synth_stack_args 42 43 360 <<'C'
volatile int sink;
static volatile int inputs[8];

__attribute__((noinline)) static int mix8(
    int a, int b, int c, int d, int e, int f, int g, int h) {
  int local[4];

  local[0] = a + b;
  local[1] = c + d;
  local[2] = e + f;
  local[3] = g + h;
  return local[0] + local[1] + local[2] + local[3] + f;
}

int test_entry(void) {
  int v;

  inputs[0] = 1;
  inputs[1] = 2;
  inputs[2] = 3;
  inputs[3] = 4;
  inputs[4] = 5;
  inputs[5] = 6;
  inputs[6] = 7;
  inputs[7] = 8;
  v = mix8(inputs[0], inputs[1], inputs[2], inputs[3],
           inputs[4], inputs[5], inputs[6], inputs[7]);
  sink = v + 1;
  return sink - 1;
}
C
  cases=$((cases + 1))

  run_compare_case synth_struct_lanes 42 43 280 <<'C'
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

struct packet {
  u8 tag;
  u16 code;
  u8 flags;
  u32 count;
};

volatile int sink;
static struct packet pkt;

__attribute__((noinline)) static void fill_packet(struct packet *p) {
  p->tag = 7;
  p->code = 11;
  p->flags = 13;
  p->count = 17;
}

__attribute__((noinline)) static int checksum_packet(const struct packet *p) {
  return p->tag + p->code + p->flags + (int)p->count;
}

int test_entry(void) {
  int v;

  fill_packet(&pkt);
  v = checksum_packet(&pkt);
  sink = v - 5;
  return sink - 1;
}
C
  cases=$((cases + 1))

  run_compare_case synth_switch_dispatch 42 43 320 <<'C'
volatile int sink;
static volatile int selector;

__attribute__((noinline)) static int dispatch(int x) {
  switch (x) {
  case 0:
    return 3;
  case 1:
    return 7;
  case 2:
    return 11;
  case 3:
    return 17;
  case 4:
    return 23;
  case 5:
    return 31;
  case 6:
    return 37;
  default:
    return 1;
  }
}

int test_entry(void) {
  int v;

  selector = 5;
  v = dispatch(selector);
  sink = v + 12;
  return sink - 1;
}
C
  cases=$((cases + 1))

  run_compare_case synth_global_index 42 43 360 <<'C'
volatile int sink;
static volatile int index_seed;
static const int table[6] = { 4, 8, 15, 16, 23, 42 };
static int out[3];

__attribute__((noinline)) static int gather(int index) {
  const int *p;

  p = table + index;
  out[0] = p[-1];
  out[1] = p[0];
  out[2] = p[2];
  return out[0] + out[1] + out[2];
}

int test_entry(void) {
  int v;

  index_seed = 2;
  v = gather(index_seed);
  sink = v - 3;
  return sink - 1;
}
C
  cases=$((cases + 1))

  echo "AVR32 LLVM/GCC synthesized comparison passed: ${cases} cases returned 42 and stored 43 in SRAM"
}

run_widget_shape_comparison_smoke() {
  local cases=0

  run_compare_case widget_features 42 43 1100 <<'C'
typedef unsigned char u8;

enum {
  feature_major_index = 0,
  feature_minor_index,
  feature_board_index,
  feature_image_index,
  feature_in_index,
  feature_out_index,
  feature_adc_index,
  feature_dac_index,
  feature_lcd_index,
  feature_log_index,
  feature_end_index
};

enum {
  feature_board_none = 0,
  feature_board_widget,
  feature_board_usbi2s,
  feature_board_usbdac,
  feature_board_test,
  feature_end_board,
  feature_image_flashyblinky,
  feature_image_uac1_audio,
  feature_image_uac1_dg8saq,
  feature_image_uac2_audio,
  feature_image_uac2_dg8saq,
  feature_image_hpsdr,
  feature_image_test,
  feature_end_image,
  feature_in_normal,
  feature_in_swapped,
  feature_end_in,
  feature_out_normal,
  feature_out_swapped,
  feature_end_out,
  feature_adc_none,
  feature_adc_ak5394a,
  feature_end_adc,
  feature_dac_none,
  feature_dac_cs4344,
  feature_dac_es9022,
  feature_end_dac,
  feature_lcd_none,
  feature_lcd_hd44780,
  feature_lcd_ks0073,
  feature_end_lcd,
  feature_log_none,
  feature_log_250ms,
  feature_log_500ms,
  feature_log_1sec,
  feature_log_2sec,
  feature_end_log,
  feature_end_values
};

volatile int sink;
static u8 features[feature_end_index];
static const u8 features_default[feature_end_index] = {
  feature_end_index,
  feature_end_values,
  feature_board_widget,
  feature_image_uac1_dg8saq,
  feature_in_normal,
  feature_out_normal,
  feature_adc_ak5394a,
  feature_dac_cs4344,
  feature_lcd_hd44780,
  feature_log_500ms
};
static const u8 value_is_end[feature_end_values] = {
  0, 0, 0, 0, 0, 1,
  0, 0, 0, 0, 0, 0, 0, 1,
  0, 0, 1,
  0, 0, 1,
  0, 0, 1,
  0, 0, 0, 1,
  0, 0, 0, 1,
  0, 0, 0, 0, 0, 1
};

__attribute__((noinline)) static void features_init_like(void) {
  int i;

  for (i = 0; i < feature_end_index; ++i)
    features[i] = features_default[i];
}

__attribute__((noinline)) static u8 feature_get(u8 index) {
  return index < feature_end_index ? features[index] : 0xff;
}

__attribute__((noinline)) static u8 feature_set(u8 index, u8 value) {
  if (index > feature_minor_index && index < feature_end_index &&
      value < feature_end_values) {
    features[index] = value;
    return features[index];
  }
  return 0xff;
}

__attribute__((noinline)) static int find_end(int start) {
  for (;;) {
    if (start + 1 == feature_end_values)
      return 0xff;
    if (value_is_end[start + 1])
      return start;
    start += 1;
  }
}

__attribute__((noinline)) static void feature_find_first_and_last_value(
    u8 index, u8 *firstp, u8 *lastp) {
  u8 this_index;
  u8 first;
  u8 last;

  if (index <= feature_minor_index || index >= feature_end_index) {
    first = 0xff;
    last = 0xff;
  } else {
    this_index = feature_minor_index + 1;
    first = 0;
    last = find_end(first);
    while (this_index < index) {
      this_index += 1;
      first = last + 2;
      last = find_end(first);
    }
  }

  *firstp = first;
  *lastp = last;
}

int test_entry(void) {
  u8 first;
  u8 last;
  int acc;

  features_init_like();
  acc = feature_get(feature_board_index);
  acc += feature_set(feature_log_index, feature_log_1sec);
  feature_find_first_and_last_value(feature_dac_index, &first, &last);
  acc += last - first;
  feature_find_first_and_last_value(feature_image_index, &first, &last);
  acc += last - first;

  sink = acc;
  return sink - 1;
}
C
  cases=$((cases + 1))

  run_compare_case widget_usb_packet 42 43 320 <<'C'
typedef unsigned char u8;
typedef unsigned int u32;

volatile int sink;
static volatile u8 ep_fifo[64];
static volatile u8 *pep_fifo;
static const u8 packet[18] = {
  1, 2, 3, 4, 5, 6, 7, 8, 9,
  10, 11, 12, 13, 14, 15, 16, 17, 18
};
static u8 captured[18];

__attribute__((noinline)) static u32 min_u32(u32 a, u32 b) {
  return a < b ? a : b;
}

__attribute__((noinline)) static u32 usb_write_packet_like(
    const u8 *txbuf, u32 data_length, const u8 **ptxbuf) {
  volatile u8 *fifo_cur;
  const u8 *txbuf_cur;
  const u8 *txbuf_end;

  fifo_cur = pep_fifo;
  txbuf_cur = txbuf;
  txbuf_end = txbuf_cur + min_u32(data_length, 15);
  while (txbuf_cur < txbuf_end)
    *fifo_cur++ = *txbuf_cur++;
  pep_fifo = fifo_cur;
  if (ptxbuf)
    *ptxbuf = txbuf_cur;
  return data_length - (u32)(txbuf_cur - txbuf);
}

__attribute__((noinline)) static u32 usb_read_packet_like(
    u8 *rxbuf, u32 data_length, u8 **prxbuf) {
  volatile u8 *fifo_cur;
  u8 *rxbuf_cur;
  u8 *rxbuf_end;

  fifo_cur = pep_fifo;
  rxbuf_cur = rxbuf;
  rxbuf_end = rxbuf_cur + min_u32(data_length, 15);
  while (rxbuf_cur < rxbuf_end)
    *rxbuf_cur++ = *fifo_cur++;
  if (prxbuf)
    *prxbuf = rxbuf_cur;
  return data_length - (u32)(rxbuf_cur - rxbuf);
}

int test_entry(void) {
  const u8 *txpos;
  u8 *rxpos;
  u32 unwritten;
  u32 unread;
  int acc;

  pep_fifo = ep_fifo;
  unwritten = usb_write_packet_like(packet, 18, &txpos);
  pep_fifo = ep_fifo;
  unread = usb_read_packet_like(captured, 15, &rxpos);

  acc = captured[0] + captured[2] + captured[6] + captured[13];
  acc += (int)unwritten;
  acc += (int)(txpos - packet);
  acc += (int)unread;
  sink = acc;
  return sink - 1;
}
C
  cases=$((cases + 1))

  run_compare_case widget_mobo_filter 42 43 320 <<'C'
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

struct mobo_data {
  u32 Freq[10];
  u8 SwitchFreq;
  u16 FilterCrossOver[8];
  u8 FilterNumber[8];
  u32 BandSub[8];
  u32 BandMul[8];
};

volatile int sink;
static const struct mobo_data cdata = {
  { 0, 0, 14U << 21, 0, 0, 0, 0, 0, 0, 0 },
  2,
  { 2, 4, 8, 11, 15, 20, 25, 30 },
  { 1, 2, 3, 4, 5, 6, 7, 8 },
  { 0, 0, 0, 0, 0, 1U << 21, 0, 0 },
  { 0, 0, 0, 0, 0, 3U << 21, 0, 0 }
};

__attribute__((noinline)) static u8 select_rx_filter(u32 freq) {
  u16 mhz;
  int i;

  mhz = (u16)(freq >> 21);
  for (i = 0; i < 8; ++i) {
    if (mhz < cdata.FilterCrossOver[i])
      return cdata.FilterNumber[i];
  }
  return cdata.FilterNumber[7];
}

__attribute__((noinline)) static u32 apply_band_calibration(u32 freq, u8 band) {
  return freq + cdata.BandMul[band] - cdata.BandSub[band];
}

int test_entry(void) {
  u32 freq;
  u32 tuned;
  u8 filter;
  int acc;

  freq = cdata.Freq[cdata.SwitchFreq];
  filter = select_rx_filter(freq);
  tuned = apply_band_calibration(freq, filter);

  acc = filter;
  acc += (int)(tuned >> 21);
  acc += cdata.FilterCrossOver[filter];
  acc += cdata.SwitchFreq;
  sink = acc;
  return sink - 1;
}
C
  cases=$((cases + 1))

  run_compare_case widget_cond_store 42 43 420 <<'C'
typedef unsigned int u32;

volatile int sink;
static int control_words[4];

__attribute__((noinline)) static void control_write_eq(
    int *slot, int lhs, int rhs, int value) {
  if (lhs == rhs)
    *slot = value;
}

__attribute__((noinline)) static void control_write_ugt(
    u32 *slot, u32 lhs, u32 rhs, u32 value) {
  if (lhs > rhs)
    *slot = value;
}

int test_entry(void) {
  control_write_eq(&control_words[0], 3, 3, 11);
  control_write_eq(&control_words[1], 3, 4, 99);
  control_write_ugt((u32 *)&control_words[2], 5, 2, 17);
  control_write_ugt((u32 *)&control_words[3], 1, 9, 99);

  sink = control_words[0] + control_words[1] +
         control_words[2] + control_words[3] + 15;
  return sink - 1;
}
C
  cases=$((cases + 1))

  run_compare_case widget_usb_aligned_packets 42 43 900 <<'C'
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned int uptr;

#define ALIGN_DOWN(value, align) ((value) & ~((align) - 1U))
#define GET_ALIGN(value, align) ((value) & ((align) - 1U))
#define TEST_ALIGN(value, align) (GET_ALIGN((value), (align)) == 0)

typedef union {
  volatile u8 *u8ptr;
  volatile u16 *u16ptr;
  volatile u32 *u32ptr;
} UnionVPtr;

typedef union {
  volatile const u8 *u8ptr;
  volatile const u16 *u16ptr;
  volatile const u32 *u32ptr;
} UnionCVPtr;

typedef union {
  const u8 *u8ptr;
  const u16 *u16ptr;
  const u32 *u32ptr;
} UnionCPtr;

typedef union {
  u8 *u8ptr;
  u16 *u16ptr;
  u32 *u32ptr;
} UnionPtr;

typedef struct {
  const u8 *u8ptr;
  const u16 *u16ptr;
  const u32 *u32ptr;
} StructCPtr;

typedef struct {
  u8 *u8ptr;
  u16 *u16ptr;
  u32 *u32ptr;
} StructPtr;

volatile int sink;
static volatile u8 ep_fifo[96] __attribute__((aligned(4)));
static volatile u8 *pep_fifo[2];
static volatile u16 endpoint_count[2];
static u8 captured[24] __attribute__((aligned(4)));
static const u16 endpoint_size[2] = { 18, 21 };
static const u8 packet[24] __attribute__((aligned(4))) = {
  1, 2, 3, 4, 5, 6, 7, 8,
  9, 10, 11, 12, 13, 14, 15, 16,
  17, 18, 19, 20, 21, 22, 23, 24
};

__attribute__((noinline)) static u32 min_u32(u32 a, u32 b) {
  return a < b ? a : b;
}

__attribute__((noinline)) static u32 usb_get_endpoint_size_like(u8 ep) {
  return endpoint_size[ep];
}

__attribute__((noinline)) static u32 usb_byte_count_like(u8 ep) {
  return endpoint_count[ep];
}

__attribute__((noinline)) static u32 usb_write_ep_txpacket_like(
    u8 ep, const void *txbuf, u32 data_length, const void **ptxbuf) {
  UnionVPtr ep_fifo_cur;
  UnionCPtr txbuf_cur;
  StructCPtr txbuf_end;
  u32 available;

  ep_fifo_cur.u8ptr = pep_fifo[ep];
  txbuf_cur.u8ptr = (const u8 *)txbuf;
  available = usb_get_endpoint_size_like(ep) - usb_byte_count_like(ep);
  txbuf_end.u8ptr = txbuf_cur.u8ptr + min_u32(data_length, available);
  txbuf_end.u16ptr = (const u16 *)ALIGN_DOWN((uptr)txbuf_end.u8ptr, 2U);
  txbuf_end.u32ptr = (const u32 *)ALIGN_DOWN((uptr)txbuf_end.u16ptr, 4U);

  if (GET_ALIGN((uptr)txbuf_cur.u8ptr, 2U) ==
      GET_ALIGN((uptr)ep_fifo_cur.u8ptr, 2U)) {
    if (!TEST_ALIGN((uptr)txbuf_cur.u8ptr, 2U)) {
      if (txbuf_cur.u8ptr < txbuf_end.u8ptr)
        *ep_fifo_cur.u8ptr++ = *txbuf_cur.u8ptr++;
    }

    if (GET_ALIGN((uptr)txbuf_cur.u16ptr, 4U) ==
        GET_ALIGN((uptr)ep_fifo_cur.u16ptr, 4U)) {
      if (!TEST_ALIGN((uptr)txbuf_cur.u16ptr, 4U)) {
        if (txbuf_cur.u16ptr < txbuf_end.u16ptr)
          *ep_fifo_cur.u16ptr++ = *txbuf_cur.u16ptr++;
      }

      while (txbuf_cur.u32ptr < txbuf_end.u32ptr)
        *ep_fifo_cur.u32ptr = *txbuf_cur.u32ptr++;

      if (txbuf_cur.u16ptr < txbuf_end.u16ptr)
        *ep_fifo_cur.u16ptr++ = *txbuf_cur.u16ptr++;
    }

    while (txbuf_cur.u16ptr < txbuf_end.u16ptr)
      *ep_fifo_cur.u16ptr++ = *txbuf_cur.u16ptr++;
  }

  while (txbuf_cur.u8ptr < txbuf_end.u8ptr)
    *ep_fifo_cur.u8ptr++ = *txbuf_cur.u8ptr++;

  pep_fifo[ep] = ep_fifo_cur.u8ptr;
  if (ptxbuf)
    *ptxbuf = txbuf_cur.u8ptr;
  return data_length - (u32)(txbuf_cur.u8ptr - (const u8 *)txbuf);
}

__attribute__((noinline)) static u32 usb_read_ep_rxpacket_like(
    u8 ep, void *rxbuf, u32 data_length, void **prxbuf) {
  UnionCVPtr ep_fifo_cur;
  UnionPtr rxbuf_cur;
  StructPtr rxbuf_end;

  ep_fifo_cur.u8ptr = pep_fifo[ep];
  rxbuf_cur.u8ptr = (u8 *)rxbuf;
  rxbuf_end.u8ptr = rxbuf_cur.u8ptr +
                    min_u32(data_length, usb_byte_count_like(ep));
  rxbuf_end.u16ptr = (u16 *)ALIGN_DOWN((uptr)rxbuf_end.u8ptr, 2U);
  rxbuf_end.u32ptr = (u32 *)ALIGN_DOWN((uptr)rxbuf_end.u16ptr, 4U);

  if (GET_ALIGN((uptr)rxbuf_cur.u8ptr, 2U) ==
      GET_ALIGN((uptr)ep_fifo_cur.u8ptr, 2U)) {
    if (!TEST_ALIGN((uptr)rxbuf_cur.u8ptr, 2U)) {
      if (rxbuf_cur.u8ptr < rxbuf_end.u8ptr)
        *rxbuf_cur.u8ptr++ = *ep_fifo_cur.u8ptr++;
    }

    if (GET_ALIGN((uptr)rxbuf_cur.u16ptr, 4U) ==
        GET_ALIGN((uptr)ep_fifo_cur.u16ptr, 4U)) {
      if (!TEST_ALIGN((uptr)rxbuf_cur.u16ptr, 4U)) {
        if (rxbuf_cur.u16ptr < rxbuf_end.u16ptr)
          *rxbuf_cur.u16ptr++ = *ep_fifo_cur.u16ptr++;
      }

      while (rxbuf_cur.u32ptr < rxbuf_end.u32ptr)
        *rxbuf_cur.u32ptr++ = *ep_fifo_cur.u32ptr;

      if (rxbuf_cur.u16ptr < rxbuf_end.u16ptr)
        *rxbuf_cur.u16ptr++ = *ep_fifo_cur.u16ptr++;
    }

    while (rxbuf_cur.u16ptr < rxbuf_end.u16ptr)
      *rxbuf_cur.u16ptr++ = *ep_fifo_cur.u16ptr++;
  }

  while (rxbuf_cur.u8ptr < rxbuf_end.u8ptr)
    *rxbuf_cur.u8ptr++ = *ep_fifo_cur.u8ptr++;

  pep_fifo[ep] = (volatile u8 *)ep_fifo_cur.u8ptr;
  if (prxbuf)
    *prxbuf = rxbuf_cur.u8ptr;
  return data_length - (u32)(rxbuf_cur.u8ptr - (u8 *)rxbuf);
}

int test_entry(void) {
  const void *txpos;
  void *rxpos;
  u32 unwritten;
  u32 unread;
  int i;
  int acc;

  pep_fifo[1] = ep_fifo + 1;
  endpoint_count[1] = 3;
  unwritten = usb_write_ep_txpacket_like(1, packet + 1, 20, &txpos);

  for (i = 0; i < 24; ++i)
    ep_fifo[32 + i] = (u8)(50 + i);
  pep_fifo[0] = ep_fifo + 33;
  endpoint_count[0] = 17;
  unread = usb_read_ep_rxpacket_like(0, captured + 1, 19, &rxpos);

  acc = 0;
  if (unwritten == 2)
    acc += 4;
  if (txpos == packet + 19)
    acc += 5;
  if (pep_fifo[1] == ep_fifo + 7)
    acc += 6;
  if (ep_fifo[1] == 2)
    acc += 7;
  if (ep_fifo[4] == 17)
    acc += 8;
  if (unread == 2)
    acc += 3;
  if (rxpos == captured + 18)
    acc += 4;
  if (pep_fifo[0] == ep_fifo + 38)
    acc += 2;
  if (captured[1] == 51)
    acc += 2;
  if (captured[8] == 54)
    acc += 2;

  sink = acc;
  return sink - 1;
}
C
  cases=$((cases + 1))

  run_compare_case widget_dg8saq_requests 42 43 3200 <<'C'
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

enum {
  feature_major_index = 0,
  feature_minor_index,
  feature_board_index,
  feature_image_index,
  feature_in_index,
  feature_out_index,
  feature_adc_index,
  feature_dac_index,
  feature_lcd_index,
  feature_log_index,
  feature_end_index
};

enum {
  feature_board_none = 0,
  feature_board_widget,
  feature_board_usbi2s,
  feature_board_usbdac,
  feature_board_test,
  feature_end_board,
  feature_image_flashyblinky,
  feature_image_uac1_audio,
  feature_image_uac1_dg8saq,
  feature_image_uac2_audio,
  feature_image_uac2_dg8saq,
  feature_image_hpsdr,
  feature_image_test,
  feature_end_image,
  feature_in_normal,
  feature_in_swapped,
  feature_end_in,
  feature_out_normal,
  feature_out_swapped,
  feature_end_out,
  feature_adc_none,
  feature_adc_ak5394a,
  feature_end_adc,
  feature_dac_none,
  feature_dac_cs4344,
  feature_dac_es9022,
  feature_end_dac,
  feature_lcd_none,
  feature_lcd_hd44780,
  feature_lcd_ks0073,
  feature_end_lcd,
  feature_log_none,
  feature_log_250ms,
  feature_log_500ms,
  feature_log_1sec,
  feature_log_2sec,
  feature_end_log,
  feature_end_values
};

#define TXF 16

struct config_data {
  u16 FilterCrossOver[8];
  u16 TXFilterCrossOver[TXF];
  u8 FilterNumber[8];
  u8 TXFilterNumber[TXF];
};

volatile int sink;
static struct config_data cdata;
static struct config_data nvram_cdata;
static u8 features[feature_end_index];
static u8 features_nvram[feature_end_index];
static u8 pcf20_present[8];
static u8 pcf38_present[8];
static u8 pcf_latch[16];
static u32 current_frequency;
static u8 freq_changed;
static u8 buffer[40] __attribute__((aligned(4)));
static const u8 features_default[feature_end_index] = {
  feature_end_index,
  feature_end_values,
  feature_board_widget,
  feature_image_uac1_dg8saq,
  feature_in_normal,
  feature_out_normal,
  feature_adc_ak5394a,
  feature_dac_cs4344,
  feature_lcd_hd44780,
  feature_log_500ms
};

__attribute__((noinline)) static void init_state(void) {
  int i;

  for (i = 0; i < 8; ++i) {
    cdata.FilterCrossOver[i] = (u16)((i + 1) * 3);
    cdata.FilterNumber[i] = (u8)(i + 1);
  }
  for (i = 0; i < TXF; ++i) {
    cdata.TXFilterCrossOver[i] = (u16)(20 + i);
    cdata.TXFilterNumber[i] = (u8)(15 - i);
  }
  for (i = 0; i < feature_end_index; ++i) {
    features[i] = features_default[i];
    features_nvram[i] = features_default[i];
  }
  pcf20_present[1] = 1;
  pcf38_present[2] = 1;
  current_frequency = 48000;
  freq_changed = 0;
}

__attribute__((noinline)) static void flashc_memset16_like(
    u16 *dst, u16 value) {
  *dst = value;
}

__attribute__((noinline)) static void flashc_memset8_like(
    u8 *dst, u8 value) {
  *dst = value;
}

__attribute__((noinline)) static u8 feature_set(u8 index, u8 value) {
  if (index > feature_minor_index && index < feature_end_index &&
      value < feature_end_values) {
    features[index] = value;
    return features[index];
  }
  return 0xff;
}

__attribute__((noinline)) static u8 feature_get(u8 index) {
  return index < feature_end_index ? features[index] : 0xff;
}

__attribute__((noinline)) static u8 feature_set_nvram(u8 index, u8 value) {
  if (index > feature_minor_index && index < feature_end_index &&
      value < feature_end_values) {
    flashc_memset8_like(&features_nvram[index], value);
    return features_nvram[index];
  }
  return 0xff;
}

__attribute__((noinline)) static u8 feature_get_nvram(u8 index) {
  return index < feature_end_index ? features_nvram[index] : 0xff;
}

__attribute__((noinline)) static u8 feature_get_default(u8 index) {
  return index < feature_end_index ? features_default[index] : 0xff;
}

__attribute__((noinline)) static int pcf_index(u16 addr) {
  if (addr >= 0x20 && addr <= 0x27 && pcf20_present[addr - 0x20])
    return (int)(addr - 0x20);
  if (addr >= 0x38 && addr <= 0x3f && pcf38_present[addr - 0x38])
    return (int)(8 + addr - 0x38);
  return -1;
}

__attribute__((noinline)) static void pcf8574_out_byte(u16 addr, u16 value) {
  int index;

  index = pcf_index(addr);
  if (index >= 0)
    pcf_latch[index] = (u8)value;
}

__attribute__((noinline)) static void pcf8574_in_byte(u16 addr, u8 *out) {
  int index;

  index = pcf_index(addr);
  if (index >= 0)
    *out = (u8)(pcf_latch[index] + addr);
  else
    *out = 42;
}

__attribute__((noinline)) static u8 dg8saq_setup_like(
    u8 type, u16 wValue, u16 wIndex, u8 *Buffer) {
  int x;
  u16 *Buf16;

  Buf16 = (u16 *)Buffer;
  *Buffer = 255;
  switch (type) {
  case 0x17:
    if (wIndex < 0x100) {
      if (wIndex < 8) {
        cdata.FilterCrossOver[wIndex] = wValue;
        flashc_memset16_like(&nvram_cdata.FilterCrossOver[wIndex], wValue);
      }
      for (x = 0; x < 8; ++x)
        Buf16[7 - x] = cdata.FilterCrossOver[x];
      return 8 * sizeof(u16);
    }

    wIndex = wIndex & 0xff;
    if (wIndex < TXF) {
      cdata.TXFilterCrossOver[wIndex] = wValue;
      flashc_memset16_like(&nvram_cdata.TXFilterCrossOver[wIndex], wValue);
    }
    for (x = 0; x < TXF; ++x)
      Buf16[(TXF - 1) - x] = cdata.TXFilterCrossOver[x];
    return TXF * sizeof(u16);

  case 0x19:
    if (wIndex < 8) {
      cdata.FilterNumber[wIndex] = (u8)wValue;
      flashc_memset8_like(&nvram_cdata.FilterNumber[wIndex], (u8)wValue);
    }
    for (x = 0; x < 8; ++x)
      Buffer[7 - x] = cdata.FilterNumber[x];
    return 8 * sizeof(u8);

  case 0x6e:
    if (pcf_index(wIndex) < 0) {
      *Buffer = 42;
      return sizeof(u8);
    }
    pcf8574_out_byte(wIndex, wValue);

  case 0x6f:
    if (pcf_index(wIndex) < 0) {
      *Buffer = 42;
      return sizeof(u8);
    }
    pcf8574_in_byte(wIndex, Buffer);
    return sizeof(u8);

  case 0x71:
    switch (wValue) {
    case 0:
      switch (wIndex) {
      case 0:
        if (current_frequency != 48000) {
          current_frequency = 48000;
          freq_changed = 1;
        }
        break;
      case 1:
        if (current_frequency != 96000) {
          current_frequency = 96000;
          freq_changed = 1;
        }
        break;
      case 2:
        if (current_frequency != 192000) {
          current_frequency = 192000;
          freq_changed = 1;
        }
        break;
      default:
        break;
      }
      *Buffer = 0;
      return sizeof(u8);
    case 3:
      Buffer[0] = feature_set_nvram(wIndex & 0xff, (wIndex >> 8) & 0xff);
      return sizeof(u8);
    case 4:
      Buffer[0] = feature_get_nvram(wIndex);
      return sizeof(u8);
    case 5:
      Buffer[0] = feature_set(wIndex & 0xff, (wIndex >> 8) & 0xff);
      return sizeof(u8);
    case 6:
      Buffer[0] = feature_get(wIndex);
      return sizeof(u8);
    case 9:
      Buffer[0] = feature_get_default(wIndex);
      return sizeof(u8);
    default:
      return sizeof(u8);
    }

  default:
    return 1;
  }
}

int test_entry(void) {
  u8 ret;
  u16 rx_filter;
  u16 tx_filter;
  u8 filter_number;
  u8 pcf_value;
  u8 missing_value;
  u8 feature_value;
  int acc;

  init_state();

  ret = dg8saq_setup_like(0x17, 44, 3, buffer);
  rx_filter = ((u16 *)buffer)[4];
  acc = ret == 16 ? 2 : 0;
  if (rx_filter == 44)
    acc += 3;

  ret = dg8saq_setup_like(0x17, 55, 0x0105, buffer);
  tx_filter = ((u16 *)buffer)[10];
  if (ret == 32)
    acc += 4;
  if (tx_filter == 55)
    acc += 5;

  ret = dg8saq_setup_like(0x19, 9, 2, buffer);
  filter_number = buffer[5];
  if (ret == 8)
    acc += 3;
  if (filter_number == 9)
    acc += 4;

  ret = dg8saq_setup_like(0x6e, 12, 0x21, buffer);
  pcf_value = buffer[0];
  if (ret == 1 && pcf_value == 45)
    acc += 5;

  ret = dg8saq_setup_like(0x6f, 0, 0x27, buffer);
  missing_value = buffer[0];
  if (ret == 1 && missing_value == 42)
    acc += 6;

  dg8saq_setup_like(0x71, 0, 1, buffer);
  if (current_frequency == 96000 && freq_changed)
    acc += 7;

  dg8saq_setup_like(0x71, 5,
                    (feature_log_1sec << 8) | feature_log_index, buffer);
  dg8saq_setup_like(0x71, 6, feature_log_index, buffer);
  feature_value = buffer[0];
  if (feature_value == feature_log_1sec)
    acc += 4;

  sink = acc;
  return sink - 1;
}
C
  cases=$((cases + 1))

  echo "AVR32 LLVM/GCC widget-shaped comparison passed: ${cases} cases returned 42 and stored 43 in SRAM"
}

run_real_sdr_widget_source_smoke() {
  local cases=0

  run_real_sdr_case real_sdr_dg8saq_source 42 43 10000 <<'C'
#include <stdint.h>
#include "compiler.h"
#include "DG8SAQ_cmd.h"
#include "Mobo_config.h"
#include "features.h"
#include "gpio.h"
#include "Si570.h"
#include "usb_specific_request.h"

extern volatile int sink;
extern U8 qemu_gpio_state[32];
extern void qemu_sdr_reset_state(void);

static U8 buffer[64] __attribute__((aligned(4)));

int test_entry(void) {
  U8 ret;
  U8 feature_value;
  int acc;
  int i;

  qemu_sdr_reset_state();
  for (i = 0; i < 64; ++i)
    buffer[i] = 0;

  for (i = 0; i < 6; ++i)
    buffer[i] = (U8)(i + 1);
  dg8saqFunctionWrite(0x30, 0, 0, buffer, 6);
  acc = 0;
  if (si570reg[0] == 6 && si570reg[5] == 1 && FRQ_fromusbreg)
    acc += 3;

  ((U32 *)buffer)[0] = 0x00123456U;
  dg8saqFunctionWrite(0x32, 0, 0, buffer, 4);
  if (freq_from_usb == 0x00123456U && FRQ_fromusb)
    acc += 4;

  ret = dg8saqFunctionSetup(0x17, 44, 3, buffer);
  if (ret == 16 && ((U16 *)buffer)[4] == 44 &&
      nvram_cdata.FilterCrossOver[3] == 44)
    acc += 5;

  ret = dg8saqFunctionSetup(0x17, 55, 0x0105, buffer);
  if (ret == 32 && ((U16 *)buffer)[10] == 55 &&
      nvram_cdata.TXFilterCrossOver[5] == 55)
    acc += 4;

  ret = dg8saqFunctionSetup(0x18, 9, 2, buffer);
  if (ret == 8 && buffer[5] == 9 && nvram_cdata.FilterNumber[2] == 9)
    acc += 4;

  ret = dg8saqFunctionSetup(0x6e, 12, 0x21, buffer);
  if (ret == 1 && buffer[0] == 45)
    acc += 4;

  ret = dg8saqFunctionSetup(0x6f, 0, 0x27, buffer);
  if (ret == 1 && buffer[0] == 42)
    acc += 4;

  ret = dg8saqFunctionSetup(0x71, 0, 1, buffer);
  if (ret == 1 && current_freq.frequency == 96000 && freq_changed)
    acc += 5;

  dg8saqFunctionSetup(0x71, 5,
                      (U16)((feature_log_1sec << 8) | feature_log_index),
                      buffer);
  dg8saqFunctionSetup(0x71, 6, feature_log_index, buffer);
  feature_value = buffer[0];
  if (feature_value == feature_log_1sec)
    acc += 4;

  qemu_gpio_state[GPIO_CW_KEY_1] = 1;
  qemu_gpio_state[GPIO_PTT_INPUT] = 1;
  qemu_gpio_state[PTT_2] = 1;
  TX_state = TRUE;
  ret = dg8saqFunctionSetup(0x50, 1, 0, buffer);
  if (ret == 1 && TX_flag && FRQ_lcdupdate &&
      (buffer[0] & (REG_CWSHORT | REG_PTT_INPUT | REG_PTT_2 | REG_TX_state)) ==
          (REG_CWSHORT | REG_PTT_INPUT | REG_PTT_2 | REG_TX_state))
    acc += 6;

  sink = acc;
  return sink - 1;
}
C
  cases=$((cases + 1))

  run_real_sdr_case real_sdr_usb_drv_source 42 43 2200 <<'C'
#include "compiler.h"
#include "usb_drv.h"

extern volatile int sink;

static volatile U8 fifo[128] __attribute__((aligned(8)));
static U8 captured[32] __attribute__((aligned(8)));
static const U8 packet[24] __attribute__((aligned(8))) = {
  1, 2, 3, 4, 5, 6, 7, 8,
  9, 10, 11, 12, 13, 14, 15, 16,
  17, 18, 19, 20, 21, 22, 23, 24
};

int test_entry(void) {
  const void *txpos;
  void *rxpos;
  U32 unwritten;
  U32 unread;
  U32 unwritten_set;
  int i;
  int acc;

  pep_fifo[1].u8ptr = fifo + 1;
  qemu_usb_endpoint_size[1] = 21;
  qemu_usb_byte_count[1] = 3;
  unwritten = usb_write_ep_txpacket(1, packet + 1, 20, &txpos);

  for (i = 0; i < 24; ++i)
    fifo[32 + i] = (U8)(50 + i);
  pep_fifo[0].u8ptr = fifo + 33;
  qemu_usb_byte_count[0] = 17;
  unread = usb_read_ep_rxpacket(0, captured + 1, 19, &rxpos);

  pep_fifo[2].u8ptr = fifo + 65;
  qemu_usb_endpoint_size[2] = 16;
  qemu_usb_byte_count[2] = 5;
  unwritten_set = usb_set_ep_txpacket(2, 0xa5, 13);

  acc = 0;
  if (unwritten == 2)
    acc += 4;
  if (txpos == packet + 19)
    acc += 5;
  if (fifo[1] == 2)
    acc += 7;
  if ((pep_fifo[1].u8ptr == fifo + 19 && fifo[4] == 5) ||
      (pep_fifo[1].u8ptr == fifo + 7 && fifo[4] == 17))
    acc += 10;

  if (unread == 2)
    acc += 3;
  if (rxpos == captured + 18)
    acc += 4;
  if (captured[1] == 51)
    acc += 2;
  if ((pep_fifo[0].u8ptr == fifo + 50 && captured[8] == 58) ||
      (pep_fifo[0].u8ptr == fifo + 38 && captured[8] == 54))
    acc += 4;

  if (unwritten_set == 2)
    acc += 1;
  if (pep_fifo[2].u8ptr == fifo + 76)
    acc += 1;
  if (fifo[65] == 0xa5)
    acc += 1;
  if (fifo[72] == 0xa5)
    acc += 1;

  sink = acc;
  return sink - 1;
}
C
  cases=$((cases + 1))

  echo "AVR32 LLVM/GCC real SDR-widget source comparison passed: ${cases} cases returned 42 and stored 43 in SRAM"
}

run_asm_smoke
run_llvm_c_smoke
run_memory_comparison_smoke
run_synthesized_comparison_smoke
run_widget_shape_comparison_smoke
run_real_sdr_widget_source_smoke
