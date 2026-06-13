# Agent Notes

This repository is an LLVM monorepo fork with active AVR32 bring-up work.
Prefer focused, verifiable changes and keep unrelated repository state alone.

## Local Workflow

- Work from the repository root. On this Mac it is `/Users/ruslanmigirov/cozycactus/llvm-project`, but other machines may use a different checkout path.
- Current AVR32 work normally happens on `codex/add-avr32-support`.
- Use `rg` / `rg --files` for searches.
- Use `apply_patch` for source edits.
- Do not delete or modify unrelated local files such as the root `a.out` unless the user explicitly asks.
- If comparing with external projects, treat them as read-only unless the user asks otherwise.
- This is a minimal-diff LLVM fork. Keep changes surgical, avoid drive-by
  formatting/refactors, and make commits logically isolated.
- AVR32-owned areas are the normal place to work. Shared LLVM files such as
  target registration, generic MC/ELF/lld plumbing, or Clang target tables can
  be edited when the AVR32 task genuinely requires it, but keep those edits
  narrow and explain why they are needed.
- Prefer LLVM's existing patterns and TableGen definitions where the framework
  expects them. Do not invent hand-written C++ paths for instruction, register,
  or calling-convention data unless the surrounding target code already does so.

## Build Directory

Known local build directories used for AVR32 work:

```sh
/tmp/llvm-avr32-pr-build
build-avr32
```

On a new machine, first inspect local build directories and set:

```sh
BUILD_DIR=/path/to/llvm-avr32-build
```

Fresh AVR32-only CMake configure command:

```sh
cmake -S llvm -B "$BUILD_DIR" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DLLVM_ENABLE_ASSERTIONS=ON \
  -DLLVM_ENABLE_PROJECTS='clang;lld' \
  -DLLVM_TARGETS_TO_BUILD='' \
  -DLLVM_EXPERIMENTAL_TARGETS_TO_BUILD=AVR32
```

Useful build command:

```sh
ninja -C "$BUILD_DIR" clang llc llvm-mc llvm-readobj llvm-readelf \
  llvm-objdump llvm-dwarfdump yaml2obj obj2yaml FileCheck lld count not \
  split-file llvm-config
```

For Clang AVR32 CodeGen only:

```sh
ninja -C "$BUILD_DIR" check-clang-codegen-avr32
```

Prefer direct `llvm-lit` invocations for focused validation instead of broad
check targets when possible.

## Focused AVR32 Test Subset

Use this subset after AVR32 target, Clang, MC, object, or lld changes:

```sh
"$BUILD_DIR/bin/llvm-lit" -sv \
  clang/test/CodeGen/avr32 \
  clang/test/Driver/avr32-toolchain.c \
  clang/test/Preprocessor/avr32-parts.c \
  clang/test/Sema/avr32-attr-interrupt.c \
  clang/test/Sema/avr32-attr-naked.c \
  clang/test/Sema/avr32-interrupt-attr.c \
  llvm/test/CodeGen/AVR32 \
  llvm/test/MC/AVR32 \
  llvm/test/tools/llvm-objdump/ELF/AVR32 \
  llvm/test/tools/llvm-readobj/ELF/avr32-elf-headers.test \
  llvm/test/tools/llvm-readobj/ELF/reloc-types-avr32.test \
  llvm/test/tools/obj2yaml/ELF/avr32-relocs-and-flags.yaml \
  lld/test/ELF/avr32.test \
  lld/test/ELF/avr32-relax.s \
  lld/test/ELF/avr32-direct-data.s \
  lld/test/ELF/avr32-lddpc-relax.s
```

Last known result for this subset: 77/77 passed after the LLVM 22.1.7 sync
verification fixes.

## GitHub AVR32 CI

The fork-specific AVR32 workflow is `.github/workflows/avr32.yml`. It runs on
`ubuntu-latest`, `macos-15-intel`, and `macos-15`; GitHub documents
`macos-15-intel` as the current standard Intel macOS label and `macos-15` as
Apple Silicon arm64.

The workflow is intentionally a smoke build, not the full AVR32 validation
suite: configure an AVR32-only `lld` build, build the small tool set, and run
AVR32 MC/object/lld tests. Keep it green before widening it. The broader
local focused subset above is still the better pre-commit signal for CodeGen or
Clang target changes when the local build is known to match the source tree.
Do not use `${{ runner.* }}` in job-level `env`; set runner-derived paths inside
a step or use `${{ github.workspace }}`. If GitHub shows a failed workflow with
zero jobs/logs, run `gh workflow run <id> --ref <branch>` to expose parse
errors.

## AVR32 Code Map

- LLVM target backend: `llvm/lib/Target/AVR32`
- AVR32 MC tests: `llvm/test/MC/AVR32`
- AVR32 CodeGen tests: `llvm/test/CodeGen/AVR32` and `clang/test/CodeGen/avr32`
- Clang AVR32 target codegen: `clang/lib/CodeGen/Targets/AVR32.cpp`
- Clang AVR32 Sema/driver/preprocessor tests: `clang/test/Sema/avr32-*`, `clang/test/Driver/avr32-toolchain.c`, `clang/test/Preprocessor/avr32-parts.c`
- lld AVR32 implementation: `lld/ELF/Arch/AVR32.cpp`
- lld AVR32 tests: `lld/test/ELF/avr32*.s`, `lld/test/ELF/avr32.test`

## AVR32 Target Notes

- Target triple: `avr32`
- Common part/CPU for SDR-widget work: `uc3a3256`
- Common Clang flags: `--target=avr32 -mpart=uc3a3256`
- AVR32 is big-endian in LLVM MC (`IsLittleEndian = false`).
- Register roles used by the current backend: `r0-r7` are normal
  call-preserved registers; `r8-r12` are caller-saved integer argument/return
  registers; `r13=sp`, `r14=lr`, `r15=pc`. `sp`, `lr`, and `pc` are reserved
  from allocation; `r7` is also reserved when used as the frame pointer.
- Current integer argument and return register order is `r12, r11, r10, r9,
  r8`. Verify ABI-sensitive changes against AVR32 GCC/binutils or the provided
  PDFs before changing calling convention, callee-save, stack, or return rules.
- Calls define `lr`; frame lowering saves/restores `lr` with `pushm/popm` when a
  function makes calls.
- AVR32 relaxation work uses `+relax` and lld relaxation tests.
- Prefer Clang's integrated assembler for LLVM end-to-end validation. Use `avr32-gcc` / GNU binutils only as comparison references.
- `LDA_W` is a codegen pseudo that emits one full `lddpc` at the instruction point and a `CPENT` constant-pool entry later. Its TableGen size must stay `4`, otherwise branch compaction can underestimate distances and produce `fixup_9h_pcrel` overflows.
- `R_AVR32_ALIGN` addends are alignment orders, not padding byte counts. Addend `2` means align to 4 bytes. This differs from RISC-V/LoongArch-style byte-count align relocs.
- LLVM MC currently marks full `lddpc` (`fixup_16b_pcrel`) as linker-relaxable under `+relax`, so later `.p2align` can emit `R_AVR32_ALIGN`. Do not blindly mark full branch/call fixups linker-relaxable: doing that broke SDR-widget `exception.x` because `_intN - _evba` table expressions stopped folding and produced `expected relocatable expression`.
- If MC linker-relaxable marking changes, compile the SDR-widget assembly `.x` files as part of validation, not just lit tests.
- When touching return instruction aliases, check printer/parser behavior separately. Some printed return forms may not round-trip through the parser unless the alias is supported.
- LLVM 22's generated subtarget info emits `LLVM_DEBUG` from the generated
  parser, so AVR32 subtarget sources need `DEBUG_TYPE` defined before including
  `AVR32GenSubtargetInfo.inc`.
- Keep `Triple::avr32` in the default DWARF CFI exception-handling list to
  match `AVR32MCAsmInfo`; otherwise Clang/llc assert during target machine
  setup.
- In SelectionDAG code, use `getSignedTargetConstant` for signed AVR32
  immediates such as displacement operands and `MOVri21`; newer LLVM asserts
  when negative values are passed through unsigned target-constant APIs.
- For hand-written AVR32 MIR that goes through normal object emission, set
  liveness metadata accurately (`tracksRegLiveness: true` and needed per-block
  `liveins`) because LLVM 22 machine passes now assert on inconsistent
  live-out queries.
- `SELECT_CC` lowers to `cp` plus a conditional register move for normal optimization, but keeps the old branch/PHI lowering under `optsize`/`minsize`. Current SDR-widget `-Oz` measurement showed conditional moves can grow size because AVR32 compact branches may relax smaller and register allocation can need extra copies.
- `popm ..., pc, r12=<imm>` is supported for immediate return values `-1`, `0`, and `1`. The special form cannot pop `lr` or `r12`; codegen folds a final `mov r12, -1/0/1` into `popm` when the epilogue is already returning through a saved `pc`.
- Next code-size targets should be measured per SDR object/function before changing codegen. A good candidate from GCC comparisons is predicated stores.

## AVR32 Linux/QEMU Validation

Known local Linux/QEMU checkouts on this Mac:

```sh
/Users/cozy/cozycactus/linux-avr32
/Users/cozy/cozycactus/linux-avr32-build-llvm-shell
/Users/cozy/cozycactus/avr32-linux-rootfs
/Users/cozy/cozycactus/qemu-avr32-v11-sync
```

The Linux checkout can be dirty with unrelated user work. Do not reset it or
revert files. Use the existing out-of-tree build directory when validating LLVM
or lld changes. Do not start by rebuilding QEMU or recreating the kernel config
from scratch; first check these existing paths and reuse the known-good build
directory.

Fast path for a new Linux validation run:

1. Confirm the four paths above still exist and check `git status` in
   `linux-avr32`; do not fetch or reset unless the user asks.
2. Rebuild the local LLVM tools from `build-avr32` with all CPU cores.
3. Rebuild `/Users/cozy/cozycactus/avr32-linux-rootfs` only if `/init` or its
   command set changed.
4. Rebuild only `vmlinux` in `linux-avr32-build-llvm-shell`.
5. Run `smoke-shell.py` and keep the log under `/tmp`.

This path is much faster than rediscovering the port: the current useful
correctness signal is "kernel links, QEMU reaches the initramfs `#` prompt, and
`pwd`, `ps`, `uname`, and `bench` run". Do not count a build-only result as a
Linux regression pass.

This checkout's `build-avr32` is a Unix Makefiles build, not Ninja. Rebuild LLVM
tools with:

```sh
make -C /Users/cozy/cozycactus/llvm-project/build-avr32 -j"$(sysctl -n hw.ncpu)" \
  clang lld llvm-mc llvm-readobj FileCheck
```

Before comparing kernel or SDR-widget results, check both compiler and linker
versions. A stale `clang` with a fresh `ld.lld` caused misleading measurements
once:

```sh
/Users/cozy/cozycactus/llvm-project/build-avr32/bin/clang --version
/Users/cozy/cozycactus/llvm-project/build-avr32/bin/ld.lld --version
```

For the LLVM Linux shell build, the previous working link path used:

```sh
K=/Users/cozy/cozycactus/linux-avr32
B=/Users/cozy/cozycactus/linux-avr32-build-llvm-shell
LLVM=/Users/cozy/cozycactus/llvm-project/build-avr32
AVR=/Users/cozy/cozycactus/avr32-toolchain-macos-arm64/avr32-tools-src/bin
GNU=/usr/local/opt/coreutils/libexec/gnubin

PATH="$GNU:$AVR:$PATH" LLVM_AVR32_LD_LLD="$LLVM/bin/ld.lld" AVR32_AS="$AVR/avr32-as" \
make -C "$K" O="$B" ARCH=avr32 CROSS_COMPILE=avr32- \
  HOSTCFLAGS="-fno-pie -I/Users/cozy/cozycactus/linux-avr32-host/include -I$B/arch/avr32/include/generated/uapi -I$B/arch/avr32/include/generated -I$B/include/generated/uapi -I$B/include/generated -I$K/arch/avr32/include/uapi -I$K/include/uapi -I$K/include" \
  HOSTLDFLAGS="-Wl,-no_pie" \
  CC="$LLVM/bin/clang --target=avr32 -fno-integrated-as -Wa,--linkrelax -B$AVR/" \
  LD=/Users/cozy/cozycactus/llvm-project/utils/avr32-linux-ld-lld-wrapper.sh \
  -j"$(sysctl -n hw.ncpu)" vmlinux
```

The `LD` wrapper is intentional. It adds `-m avr32elf` when the kernel does not
pass an emulation, supplies an empty AVR32 object for empty partial links, and
keeps partial links using `ld.lld` instead of GNU `ld`.

Do not replace the wrapper with raw `ld.lld` just to simplify the command: the
kernel build has empty partial links and missing-emulation partial links that
worked only after the wrapper handled them. If `ld.lld` fails before the final
link, first check whether the failing command bypassed
`utils/avr32-linux-ld-lld-wrapper.sh`.

Why those host flags matter on macOS:

- macOS has no system `<elf.h>`. Use
  `/Users/cozy/cozycactus/linux-avr32-host/include/elf.h`, not Homebrew
  `libelf/elf_repl.h`; the latter misses Linux `modpost` constants/macros.
- Host `modpost` may fail with `Found illegal text-relocations` unless host
  tools are built with `-fno-pie` and linked with `-Wl,-no_pie`.
- `scripts/link-vmlinux.sh` uses GNU `stat -c`; put Homebrew coreutils
  `/usr/local/opt/coreutils/libexec/gnubin` before `/usr/bin` in `PATH`.
- If host scripts need `strtonum`, use Homebrew `gawk` (`/usr/local/bin/gawk`)
  rather than macOS `awk`.

Common failure signatures and the known fix:

- `fatal error: 'elf.h' file not found`: the host include path is missing
  `/Users/cozy/cozycactus/linux-avr32-host/include`.
- missing `R_AVR32_*` constants during `modpost`: the wrong replacement
  `elf.h` was used; use the project-local Linux-oriented header above.
- `Found illegal text-relocations`: rebuild host tools with `-fno-pie` and
  link them with `-Wl,-no_pie`.
- `stat: illegal option -- c`: GNU coreutils is not first in `PATH`.
- `function strtonum never defined` from awk scripts: use Homebrew `gawk`.
- `improper alignment for relocation R_AVR32_18W_PCREL`: suspect AVR32 lld
  relaxation in a mixed relaxable/non-relaxable Linux link before changing the
  kernel.

The current LLVM shell build config already has:

```sh
CONFIG_INITRAMFS_SOURCE="/Users/cozy/cozycactus/avr32-linux-rootfs/initramfs.list"
```

That rootfs is a tiny custom `/init`, not BusyBox. It is enough for smoke tests
and basic syscall/performance checks. Rebuild it with:

```sh
make -C /Users/cozy/cozycactus/avr32-linux-rootfs
```

Then rebuild `vmlinux`; the kernel embeds the new `/init` through
`initramfs.list`. The current shell builtins are `help`, `pwd`, `cd`, `ls`,
`cat`, `ps`, `bench`, `uname`, `id`, `echo`, and `exit`. `/init` mounts `/proc`
itself, so `ps`, `cat /proc/version`, and `bench` should work without extra
boot-time setup.

After a successful `vmlinux` link, boot it with QEMU and run at least `pwd`,
`ps`, and `uname -a` at the initramfs shell:

```sh
QEMU=/Users/cozy/cozycactus/qemu-avr32-v11-sync/build/qemu-system-avr32
VMLINUX=/Users/cozy/cozycactus/linux-avr32-build-llvm-shell/vmlinux
"$QEMU" -M avr32example-board -kernel "$VMLINUX" -nographic -no-reboot \
  -append 'console=ttyS0,115200n8 rdinit=/init lpj=100000 ignore_loglevel loglevel=8'
```

For manual shell work, this helper uses the same QEMU arguments:

```sh
/Users/cozy/cozycactus/avr32-linux-rootfs/run-shell.sh \
  /Users/cozy/cozycactus/linux-avr32-build-llvm-shell/vmlinux
```

For automated smoke checks, prefer the rootfs `smoke-shell.py` helper when it
matches the command set being tested. A real pass means QEMU reaches the `#`
prompt and commands such as `pwd`, `ps`, and `uname` return sensible output.
The benchmark lines from `bench` are useful for same-machine before/after
comparisons, but they are QEMU/user-shell timings rather than AVR32 hardware
cycle measurements.

Current GCC vs LLVM shell-kernel comparison reference from June 12, 2026:

- Both builds used identical `linux-avr32-build-gcc-shell` and
  `linux-avr32-build-llvm-shell` configs with
  `CONFIG_INITRAMFS_SOURCE="/Users/cozy/cozycactus/avr32-linux-rootfs/initramfs.list"`,
  `CONFIG_CC_OPTIMIZE_FOR_SIZE=y`, and `CONFIG_FRAME_POINTER=y`.
- Rebuilt compilers before measuring; stale `clang`/`ld.lld` versions are a
  known source of false results.
- `avr32-size vmlinux` runtime totals:
  - GCC: text 2,348,472; data 649,780; bss 80,632; dec 3,078,884
  - LLVM/lld: text 2,429,036; data 799,680; bss 80,900; dec 3,309,616
  - LLVM delta: +230,732 bytes in text+data+bss, even though the raw `vmlinux`
    file is smaller because GCC carries a large `.debug_frame` section.
- Section deltas show the gap is mostly `.rodata` (+154,288) and `.text`
  (+64,184). Be careful interpreting this: GCC places many constant tables in
  text-like regions, so compare combined runtime sections and object/function
  deltas, not `.rodata` alone.
- Five-run QEMU smoke/bench medians showed both kernels boot and pass `pwd`,
  `ps`, `uname`, and `bench`. LLVM was slightly faster on `checksum` and
  `getdents`, roughly even on `openclose`, and slower on `getcwd`, `getpid`,
  and `readproc`. Treat this as a QEMU smoke/perf trend, not hardware timing.
- Largest matched object runtime deltas, LLVM minus GCC, included
  `fs/ext4/ext4.o` +7,773, `drivers/ata/libata.o` +3,647,
  `kernel/time/timekeeping.o` +2,971, `fs/ext4/super.o` +2,728,
  `fs/ext2/ext2.o` +2,330, `fs/fuse/fuse.o` +2,067, and
  `drivers/mmc/core/mmc_core.o` +1,993.
- Predicated stores are already implemented in the current branch, so do not
  restart there. The next Linux-size experiment should measure conditional
  select/move heuristics: GCC emits thousands of `mov*` conditional moves in
  the shell kernel, while the current LLVM size path usually lowers `select` to
  compact branch/PHI form. SDR-widget previously showed that unconditional
  conditional moves can grow `-Oz`, so make this a measured heuristic rather
  than a blanket reversal.

Post-`cpc` i64 compare update from commit `a5e7f792cc69`:

- The useful kernel runtime bucket is `.init + .init.data + .text + .rodata +
  .data`, not raw `avr32-size dec` and not the on-disk `vmlinux` size.
- Lowering i64 compares to `cp` low word plus `cpc` high word reduced the LLVM
  shell kernel runtime bucket from 3,002,912 to 2,989,120 bytes. The matching
  GCC reference is 2,958,984 bytes, so the remaining LLVM gap is 30,136 bytes.
- A one-object win can be misleading. The first `SETCC i64`-only attempt shrank
  `kernel/time/timekeeping.o` but grew the full kernel because branch users
  materialized boolean results. The fixed shape recognizes `BRCOND` of
  AVR32 `SET_CC`/`SET_CC_64` and branches directly from the compare glue.
- Always validate both a representative object/function and the full kernel.
  The final `timekeeping.o` total went 15,222 -> 14,666 bytes, while the full
  kernel runtime bucket improved by 13,792 bytes.
- This milestone was accepted only after QEMU reached the initramfs shell and
  `smoke-shell.py` passed `pwd`, `ls`, `/proc`, `ps`, `id`, `uname`, and
  `bench`. Keep that as the correctness floor for size changes.

For lld relaxation changes, always include the Linux final link in validation.
Mixed links are common: kernel objects may be link-relaxable while runtime or
host-generated objects are not. It is safe to delete 4-byte call/data pool
entries from relaxable input sections, but do not allow mixed-link 2-byte
removals. If the final Linux link reports
`improper alignment for relocation R_AVR32_18W_PCREL`, mixed-link halfword
relaxation probably shifted a word-PC relocation by two bytes. Also check for
non-paired halfword removals crossing same-section word-PC relocation sites and
targets; this can show up only during the Linux `vmlinux` link, not in small
unit tests.

## SDR-widget Benchmark

Known local SDR-widget checkout used for measurements on this Mac:

```sh
/Users/ruslanmigirov/cozycactus/sdr-widget
/Users/cozy/cozycactus/sdr-widget
```

Other known locations that may exist:

```sh
/Users/ruslanmigirov/sdr-widget
/Users/ruslanmigirov/eclipse-workspace/audio_widget_v006
```

On another machine, locate the benchmark before using it:

```sh
find "$HOME" -maxdepth 4 -type d -name sdr-widget 2>/dev/null
```

The board docs/reference checkout under Downloads is documentation-heavy. Prefer
a source checkout with `Release/src/subdir.mk` for compile and size comparisons.

Use `Release/src/subdir.mk` as the source of truth for top-level SDR-widget C
compile checks and include paths. For full `Release/widget.elf` comparisons,
use all `Release/**/subdir.mk` files included by `Release/makefile`; the current
generated full graph has 74 objects from 23 `subdir.mk` files.

For GCC-compatible SDR-widget comparisons, include GNU89 inline semantics:

```sh
--target=avr32 -mpart=uc3a3256 -std=gnu89 -O2 \
  -fcommon -fdata-sections -ffunction-sections -Wall -Wno-expansion-to-defined
```

Why `-std=gnu89` matters: Atmel ASF headers use `extern __inline__`; without
GNU89 semantics Clang may emit extra out-of-line helper bodies that old
`avr32-gcc` does not emit.

Why `-fcommon` matters: SDR-widget has old-style tentative globals such as
`xStatus`; without `-fcommon`, a full link can fail with duplicate symbols.

For full SDR-widget LLVM links, compile with `--sysroot=/Users/ruslanmigirov/avr32-tools-src/avr32`
and link with Clang/lld using `-nostartfiles -mrelax -Wl,--gc-sections
-Wl,-e,_trampoline -Wl,--direct-data`, the object response file, and
`/Users/ruslanmigirov/cozycactus/sdr-widget/Release/src/newlib_compat.o`.
The sysroot is also needed for assembly-with-cpp `.x` files that include
`<avr32/io.h>`.

On the `/Users/cozy` Mac, the current generated scratch tree is:

```sh
/Users/cozy/cozycactus/sdr-widget/Release-llvm-relax
/tmp/sdr-llvm-relax-toolwrap/avr32-gcc
/Users/cozy/cozycactus/avr32-toolchain-macos-arm64/avr32-tools-src/avr32
```

Rebuild it with all cores through the wrapper and force `-Oz` with:

```sh
CORES=$(sysctl -n hw.ncpu)
AVR=/Users/cozy/cozycactus/avr32-toolchain-macos-arm64/avr32-tools-src/bin
PATH="/tmp/sdr-llvm-relax-toolwrap:$AVR:$PATH" AVR32_LLVM_OPT=-Oz make clean
PATH="/tmp/sdr-llvm-relax-toolwrap:$AVR:$PATH" AVR32_LLVM_OPT=-Oz make -j"$CORES" all
```

For the lld comparison from that scratch tree, let Clang's AVR32 driver supply
the linker script, library paths, and `ld.lld`; in zsh split the make `OBJS`
value before passing it to Clang:

```sh
CLANG=/Users/cozy/cozycactus/llvm-project/build-avr32/bin/clang
SYSROOT=/Users/cozy/cozycactus/avr32-toolchain-macos-arm64/avr32-tools-src/avr32
OBJS=$(make -pn all | awk '/^OBJS :=/ { sub(/^OBJS :=[[:space:]]*/, ""); print; exit }')
objs=(${=OBJS})
"$CLANG" --target=avr32 --sysroot="$SYSROOT" -mpart=uc3a3256 \
  -nostartfiles -mrelax -Wl,--gc-sections -Wl,-e,_trampoline \
  -Wl,--direct-data -o widget-llvm-current-Oz-mixedrelax-lld.elf $objs -lm
```

After commit `a5e7f792cc69`, this local lld-linked scratch artifact measured
flash `.text + .rodata + .data = 108,368` bytes (`.text` 97,196, `.rodata`
9,508, `.data` 1,664), versus the nearest saved local baseline
`widget-llvm-asm-fallthrough-Oz-mixedrelax-lld.elf` at 108,392 bytes. Use the
exact local artifact names in reports; this scratch tree has multiple historical
ELFs with different branch states.

Current practical compile reference:

- SDR-widget `Release/src`: 44/44 C files compile with LLVM using the flags above.
- Comparable GCC object set: 16 matched objects. Existing GCC-only objects without matching source in `Release/src`: `newlib_compat.o`, `pcm5142.o`, `wm8804.o`.
- Current flash-size comparison for the 16 matched objects with `-std=gnu89`:
  - GCC flash (`.text + .rodata + .data`): 20,128 bytes
  - LLVM flash (`.text + .rodata + .data`): 17,134 bytes
  - LLVM delta: -2,994 bytes (-14.9%)
- Current clean full generated `Release` graph after the LLVM 22.1.7 sync
  (SDR-widget branch `sdr-widget` at `bdfb13e9`; flash is `.text + .rodata +
  .data`, excludes `.reset`, `.exception`, `.userpage`, `.stack`, and debug):
  - GCC Os: flash 111,590; `.text` 99,662; `.exception` 512; `.rodata` 10,256; `.data` 1,672; `.bss` 22,176
  - GCC O2: flash 115,718; `.text` 103,346; `.exception` 512; `.rodata` 10,700; `.data` 1,672; `.bss` 22,176
  - LLVM Oz/lld after AVR32 post-increment load/store lowering: flash 116,536; `.text` 105,984; `.exception` 372; `.rodata` 8,888; `.data` 1,664; `.bss` 22,048
  - LLVM O2/lld: flash 128,100; `.text` 117,560; `.exception` 372; `.rodata` 8,876; `.data` 1,664; `.bss` 22,040
  - AVR32 post-increment lowering improved LLVM Oz/lld by 76 flash bytes from
    the post-sync baseline (`.text` 106,060 -> 105,984) and reduced
    `dg8saqFunctionSetup` from 3,444 to 3,424 bytes.
  - All four builds used 74 objects and `/Users/ruslanmigirov/cozycactus/sdr-widget/Release/src/newlib_compat.o`.
  - In temporary makefile-wrapper builds, use `make all` explicitly; bare `make`
    may select an included dependency target before `all`.
  - Clang wrapper details used for this comparison: pass `--target=avr32
    --sysroot=/Users/ruslanmigirov/avr32-tools-src/avr32`, rewrite the makefile
    `-O2` to the requested LLVM opt level, add `-std=gnu89 -fcommon
    -Wno-expansion-to-defined` for C compiles, drop assembler-only `-Wa,-g`,
    and add `-mrelax` for links.
  - Matched project objects explain +3,037 bytes of LLVM Oz vs GCC Os flash
    delta: LLVM objects flash 87,050 vs GCC objects flash 84,013. Runtime/libc
    and final link selection explain the remaining roughly +1.9 KB.
  - Largest current object flash deltas, LLVM Oz minus GCC Os: `taskPushButtonMenu.o` +460, `taskMoboCtrl.o` +410, `tasks.o` +385, `DG8SAQ_cmd.o` +362, `taskLCD.o` +355, `uac2_usb_specific_request.o` -346, `uac2_device_audio_task.o` +328, `gpio.o` +328, `tc.o` -304, `twim_patched.o` +284.
  - Largest current final text symbol deltas, LLVM Oz minus GCC Os: `dg8saqFunctionSetup` +500, `vtaskLCD` +452, `uac2_device_audio_task` +316, `uac2_user_read_request` -296, `vtaskMoboCtrl` +264, `menu_level0` +248, `pcf8574_menu` +212, `i2c_menu` +204.
- Current clean full generated `Release` graph for SDR-widget branch
  `codex/macos-avr32-build-local` at `22c5349372` (`Use latest AVR32
  toolchain release in CI`), measured from `git archive HEAD` temp builds:
  - GCC Os: flash 111,590; `.text` 99,662; `.exception` 512; `.rodata` 10,256; `.data` 1,672; `.bss` 22,176
  - GCC O2: flash 115,718; `.text` 103,346; `.exception` 512; `.rodata` 10,700; `.data` 1,672; `.bss` 22,176
  - LLVM Oz/lld before redundant cross-block cast folding: flash 116,532; `.text` 105,360; `.exception` 356; `.rodata` 9,508; `.data` 1,664; `.bss` 22,040
  - LLVM Oz/lld after redundant cross-block cast folding: flash 116,412; `.text` 105,240; `.exception` 356; `.rodata` 9,508; `.data` 1,664; `.bss` 22,040
  - LLVM O2/lld before that fold: flash 119,944; `.text` 107,952; `.exception` 356; `.rodata` 10,328; `.data` 1,664; `.bss` 22,040
  - All builds used 75 objects; `src/newlib_compat.o` is part of this branch's generated object set.
  - For LLVM makefile-wrapper comparisons, pass `-mrelax` on compile,
    assembler-with-cpp, and link steps. Passing it only at link time measured
    a false LLVM Oz flash of 123,880 bytes because relaxable forms were not
    emitted during compilation.
  - The redundant cross-block cast fold reduced final `castu.b` instruction
    count from 357 to 311 and saved 120 linked flash bytes.
- Current full SDR-widget link reference after the `popm` return-value change
  (`.text + .rodata + .data`; current local artifacts):
  - LLVM Oz/lld: flash 107,964; `.text` 97,376; `.exception` 356; `.rodata` 8,924; `.data` 1,664; `.bss` 22,040
  - GCC Os: flash 111,626; `.text` 99,658; `.exception` 512; `.rodata` 10,296; `.data` 1,672; `.bss` 22,168
  - The `popm` return-value change folded 15 linked return sites and reduced the ordered LLVM `-Oz` flash size by 8 bytes from the `a424b63a0` reference.
- Prior full SDR-widget link reference after commit `a424b63a0`:
  - LLVM Oz/lld: flash 107,972; `.text` 97,384; `.exception` 356; `.rodata` 8,924; `.data` 1,664; `.bss` 22,040
  - GCC Os: flash 111,626; `.text` 99,658; `.exception` 512; `.rodata` 10,296; `.data` 1,672; `.bss` 22,168
  - The `SELECT_CC` conditional-move change was neutral for this LLVM `-Oz` linked size because size-optimized functions keep branch/PHI lowering.
- Prior full SDR-widget link reference after commit `75d9fc112`:
  - LLVM Oz/lld: flash 116,528; text 105,940; `.rodata` 8,924; `.data` 1,664; `.bss` 22,040
  - GCC Os: flash 120,338; text 108,370; `.rodata` 10,296; `.data` 1,672; `.bss` 22,168
  - LLVM instruction counts from GNU `avr32-objdump`: `ld.w total=2147`, `ld.w pc=139`, `lddpc=458`, `mcall=497`
  - GCC instruction counts: `ld.w total=2074`, `ld.w pc=19`, `lddpc=2502`, `mcall=2140`

For object-size comparisons, do not use raw `.o` file size as the main metric.
Report:

- flash bytes: `.text + .rodata + .data`
- `.text`
- `.rodata`
- `.data`
- `.bss` separately

`.bss` can differ sharply because GCC objects may use common symbols where LLVM
emits real `.bss` sections.

## Useful Reference PDFs

Local AVR32 references provided by the user on this Mac:

- `/Users/ruslanmigirov/Downloads/AVR32006 - Getting started with GCC for AVR32.pdf`
- `/Users/ruslanmigirov/Downloads/AVR32UC Technical Reference Manual.pdf`

On another machine, check `~/Downloads` or ask the user to provide the PDFs if
they are missing. Use them when instruction semantics, ABI details,
startup/linker behavior, or GCC compatibility are unclear.

## Communication Budget

- Prefer concise output.
- Do not paste full logs, full diffs, or full generated files unless the user asks.
- For long commands, write logs to `/tmp` and summarize only failures or final pass/fail counts.
- Final responses should include only changed files, commit hash if any, validation, push status, and next action.
- Do not mention unrelated untracked files unless they affect the task.

## Git Hygiene

- Keep commits small and focused.
- Before committing, run `git diff --check`.
- Stage only files related to the current change.
- If committing/pushing, report the exact commit hash and validation commands.
