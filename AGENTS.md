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
  lld/test/ELF/avr32-direct-data.s
```

Last known result for this subset: 37/37 passed after commit `364bd865d`.

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
- When touching return instruction aliases, check printer/parser behavior separately. Some printed return forms may not round-trip through the parser unless the alias is supported.

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
  -fdata-sections -ffunction-sections -Wall -Wno-expansion-to-defined
```

Why `-std=gnu89` matters: Atmel ASF headers use `extern __inline__`; without
GNU89 semantics Clang may emit extra out-of-line helper bodies that old
`avr32-gcc` does not emit.

Current practical compile reference:

- SDR-widget `Release/src`: 44/44 C files compile with LLVM using the flags above.
- Comparable GCC object set: 16 matched objects. Existing GCC-only objects without matching source in `Release/src`: `newlib_compat.o`, `pcm5142.o`, `wm8804.o`.
- Current flash-size comparison for the 16 matched objects with `-std=gnu89`:
  - GCC flash (`.text + .rodata + .data`): 20,128 bytes
  - LLVM flash (`.text + .rodata + .data`): 17,134 bytes
  - LLVM delta: -2,994 bytes (-14.9%)

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

## Git Hygiene

- Keep commits small and focused.
- Before committing, run `git diff --check`.
- Stage only files related to the current change.
- If committing/pushing, report the exact commit hash and validation commands.
