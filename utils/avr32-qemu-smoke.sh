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
require_executable "$AVR32_NM"
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
    "$AVR32_CLANG" --target=avr32 -mpart=uc3a3256 -Oz \
      -ffreestanding -fno-builtin \
      -c "$dir/case.c" -o "$dir/case.o"
    ;;
  gcc)
    "$AVR32_GCC" -mpart=uc3a3256 -Os \
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

  echo "AVR32 LLVM/GCC widget-shaped comparison passed: ${cases} cases returned 42 and stored 43 in SRAM"
}

run_asm_smoke
run_llvm_c_smoke
run_memory_comparison_smoke
run_widget_shape_comparison_smoke
