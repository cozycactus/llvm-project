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

Useful build command:

```sh
ninja -C "$BUILD_DIR" clang llc llvm-mc llvm-readobj llvm-objdump yaml2obj obj2yaml FileCheck lld
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

Last known result for this subset: 43/43 passed after the `popm` return-value
change.

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
- AVR32 relaxation work uses `+relax` and lld relaxation tests.
- Prefer Clang's integrated assembler for LLVM end-to-end validation. Use `avr32-gcc` / GNU binutils only as comparison references.
- `LDA_W` is a codegen pseudo that emits one full `lddpc` at the instruction point and a `CPENT` constant-pool entry later. Its TableGen size must stay `4`, otherwise branch compaction can underestimate distances and produce `fixup_9h_pcrel` overflows.
- `R_AVR32_ALIGN` addends are alignment orders, not padding byte counts. Addend `2` means align to 4 bytes. This differs from RISC-V/LoongArch-style byte-count align relocs.
- LLVM MC currently marks full `lddpc` (`fixup_16b_pcrel`) as linker-relaxable under `+relax`, so later `.p2align` can emit `R_AVR32_ALIGN`. Do not blindly mark full branch/call fixups linker-relaxable: doing that broke SDR-widget `exception.x` because `_intN - _evba` table expressions stopped folding and produced `expected relocatable expression`.
- If MC linker-relaxable marking changes, compile the SDR-widget assembly `.x` files as part of validation, not just lit tests.
- When touching return instruction aliases, check printer/parser behavior separately. Some printed return forms may not round-trip through the parser unless the alias is supported.
- `SELECT_CC` lowers to `cp` plus a conditional register move for normal optimization, but keeps the old branch/PHI lowering under `optsize`/`minsize`. Current SDR-widget `-Oz` measurement showed conditional moves can grow size because AVR32 compact branches may relax smaller and register allocation can need extra copies.
- `popm ..., pc, r12=<imm>` is supported for immediate return values `-1`, `0`, and `1`. The special form cannot pop `lr` or `r12`; codegen folds a final `mov r12, -1/0/1` into `popm` when the epilogue is already returning through a saved `pc`.
- Next code-size targets should be measured per SDR object/function before changing codegen. A good candidate from GCC comparisons is predicated stores.

## SDR-widget Benchmark

Known local SDR-widget checkout used for measurements on this Mac:

```sh
/Users/ruslanmigirov/cozycactus/sdr-widget
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

Use `Release/src/subdir.mk` as the source of truth for the SDR-widget source
list and include paths. Avoid reconstructing include flags by memory.

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

Current practical compile reference:

- SDR-widget `Release/src`: 44/44 C files compile with LLVM using the flags above.
- Comparable GCC object set: 16 matched objects. Existing GCC-only objects without matching source in `Release/src`: `newlib_compat.o`, `pcm5142.o`, `wm8804.o`.
- Current flash-size comparison for the 16 matched objects with `-std=gnu89`:
  - GCC flash (`.text + .rodata + .data`): 20,128 bytes
  - LLVM flash (`.text + .rodata + .data`): 17,134 bytes
  - LLVM delta: -2,994 bytes (-14.9%)
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
