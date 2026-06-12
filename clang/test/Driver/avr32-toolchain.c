// RUN: %clang -### --target=avr32 --sysroot=%S/Inputs/basic_avr32_tree \
// RUN:   -mpart=uc3a3256 -nostartfiles %s 2>&1 \
// RUN:   | FileCheck --check-prefix=LINK %s
// LINK: "{{.*}}ld.lld"
// LINK-SAME: "-m" "avr32elf_uc3a3256"
// LINK-SAME: "--gc-sections"
// LINK-SAME: "-T{{.*}}basic_avr32_tree{{/|\\\\}}lib{{/|\\\\}}ldscripts{{/|\\\\}}avr32elf_uc3a3256.x"
// LINK-SAME: "-lgcc" "-lc" "-lgcc"

// RUN: %clang -### --target=avr32 --sysroot=%S/Inputs/basic_avr32_tree \
// RUN:   -mpart=uc3a3256 -nostartfiles -T custom.ld %s 2>&1 \
// RUN:   | FileCheck --check-prefix=CUSTOM %s
// CUSTOM: "{{.*}}ld.lld"
// CUSTOM-SAME: "-T" "custom.ld"
// CUSTOM-NOT: avr32elf_uc3a3256.x

// RUN: %clang -### --target=avr32 --sysroot=%S/Inputs/basic_avr32_tree \
// RUN:   -mpart=uc3a3256 -nostartfiles --rodata-writable %s 2>&1 \
// RUN:   | FileCheck --check-prefix=WRITABLE-RODATA %s
// WRITABLE-RODATA: "{{.*}}ld.lld"
// WRITABLE-RODATA-SAME: "-T{{.*}}basic_avr32_tree{{/|\\\\}}lib{{/|\\\\}}ldscripts{{/|\\\\}}avr32elf_uc3a3256.xwr"

// RUN: %clang -### --target=avr32 --sysroot=%S/Inputs/basic_avr32_tree \
// RUN:   -mmcu=uc3a3256 -nostartfiles -r %s 2>&1 \
// RUN:   | FileCheck --check-prefix=RELOC %s
// RELOC: "{{.*}}ld.lld"
// RELOC-SAME: "-r"
// RELOC-SAME: "-m" "avr32elf_uc3a3256"
// RELOC-NOT: "--gc-sections"
// RELOC-NOT: "-T
// RELOC-NOT: "-lgcc"

// RUN: %clang -### --target=avr32 -mcpu=uc3a3256 --rodata-writable \
// RUN:   -c %s 2>&1 | FileCheck --check-prefix=CPU %s
// CPU-NOT: warning: argument unused during compilation
// CPU: "-target-cpu" "uc3a3256"

// RUN: %clang -### --target=avr32 -mmcu=uc3a3revd -mpart=uc3a3128 \
// RUN:   -masm-addr-pseudos -c %s 2>&1 | FileCheck --check-prefix=PART %s
// PART-NOT: warning: argument unused during compilation
// PART: "-target-cpu" "uc3a3128"

// RUN: %clang -### --target=avr32 -mmcu=uc3a3revd \
// RUN:   -c %s 2>&1 | FileCheck --check-prefix=MMCU %s
// MMCU-NOT: warning: argument unused during compilation
// MMCU: "-target-cpu" "uc3a3revd"

// RUN: %clang -### --target=avr32 -mcpu=uc3a3128 \
// RUN:   -c %s 2>&1 | FileCheck --check-prefix=CHAR %s
// CHAR: "-cc1"
// CHAR-DAG: "-fno-signed-char"
// CHAR-DAG: "-target-cpu" "uc3a3128"

// RUN: %clang -### --target=avr32 -mpart=uc3a3256 -mrelax \
// RUN:   -c %s 2>&1 | FileCheck --check-prefix=RELAX-CC1 %s
// RELAX-CC1-NOT: warning: argument unused
// RELAX-CC1: "-target-feature" "+relax"

// RUN: %clang -### --target=avr32 -mpart=uc3a3256 -mno-pic \
// RUN:   -c %s 2>&1 | FileCheck --check-prefix=NO-PIC %s
// NO-PIC-NOT: unknown argument
// NO-PIC-NOT: warning: argument unused
// NO-PIC: "-cc1"

// RUN: %clang -### --target=avr32 -fno-integrated-as \
// RUN:   -march=ap -mpart=uc3a3256 -mrelax -mno-pic -Wa,-gdwarf-2 \
// RUN:   -c %s 2>&1 | FileCheck --check-prefix=EXT-AS %s
// EXT-AS: "{{.*}}avr32-as"
// EXT-AS-SAME: "-march=ap"
// EXT-AS-SAME: "-mpart=uc3a3256"
// EXT-AS-SAME: "--linkrelax"
// EXT-AS-SAME: "--no-pic"
// EXT-AS-SAME: "-gdwarf-2"

// RUN: %clang -### --target=avr32 -fno-integrated-as \
// RUN:   -march=ap -mpart=uc3a3256 -Os -c %s 2>&1 \
// RUN:   | FileCheck --check-prefix=EXT-AS-OPT-RELAX %s
// EXT-AS-OPT-RELAX: "{{.*}}avr32-as"
// EXT-AS-OPT-RELAX-SAME: "--linkrelax"

// RUN: %clang -### --target=avr32 -mpart=uc3a3256 -O2 \
// RUN:   -c %s 2>&1 | FileCheck --check-prefix=OPT-RELAX-CC1 %s
// OPT-RELAX-CC1: "-target-feature" "+relax"

// RUN: %clang -### --target=avr32 -march=ap \
// RUN:   -c %s 2>&1 | FileCheck --check-prefix=AP-CPU %s
// AP-CPU-NOT: warning: argument unused during compilation
// AP-CPU: "-target-cpu" "ap"

// RUN: %clang -### --target=avr32 -mpart=uc3a3256 -O0 \
// RUN:   -c %s 2>&1 | FileCheck --check-prefix=FP-O0 %s
// FP-O0: "-mframe-pointer=all"

// RUN: %clang -### --target=avr32 -mpart=uc3a3256 -O2 \
// RUN:   -c %s 2>&1 | FileCheck --check-prefix=FP-O2 %s
// FP-O2: "-mframe-pointer=none"

// RUN: %clang -### --target=avr32 -mpart=uc3a3256 -O2 \
// RUN:   -fno-omit-frame-pointer -c %s 2>&1 \
// RUN:   | FileCheck --check-prefix=FP-FORCED %s
// FP-FORCED: "-mframe-pointer=all"

// RUN: %clang -### --target=avr32 -mpart=uc3a3256 -O2 \
// RUN:   -c %s 2>&1 | FileCheck --check-prefix=O2-INLINE %s
// O2-INLINE: "-mllvm" "-inline-threshold=10"

// RUN: %clang -### --target=avr32 -mpart=uc3a3256 -Os \
// RUN:   -c %s 2>&1 \
// RUN:   | FileCheck --check-prefix=OS-INLINE \
// RUN:     --implicit-check-not="-inline-threshold=10" %s
// OS-INLINE: "-cc1"

// RUN: %clang -### --target=avr32 -mpart=uc3a3256 -O2 \
// RUN:   -mllvm -inline-threshold=45 -c %s 2>&1 \
// RUN:   | FileCheck --check-prefix=O2-INLINE-OVERRIDE \
// RUN:     --implicit-check-not="-inline-threshold=10" %s
// O2-INLINE-OVERRIDE: "-mllvm" "-inline-threshold=45"

// RUN: %clang -### --target=avr32 -mpart=uc3a3256 -mno-relax \
// RUN:   -c %s 2>&1 | FileCheck --check-prefix=NORELAX-CC1 %s
// NORELAX-CC1-NOT: warning: argument unused
// NORELAX-CC1: "-target-feature" "-relax"

// RUN: %clang -### --target=avr32 -mpart=uc3a3256 \
// RUN:   -mno-asm-addr-pseudos -c %s 2>&1 \
// RUN:   | FileCheck --check-prefix=NOADDRPSEUDO %s
// NOADDRPSEUDO-NOT: unknown argument
// NOADDRPSEUDO-NOT: warning: argument unused
// NOADDRPSEUDO: "-cc1"

// RUN: %clang -### --target=avr32 --sysroot=%S/Inputs/basic_avr32_tree \
// RUN:   -mpart=uc3a3256 -nostartfiles -O2 %s 2>&1 \
// RUN:   | FileCheck --check-prefix=RELAX-LINK %s
// RELAX-LINK: "{{.*}}ld.lld"
// RELAX-LINK-SAME: "--relax"

// RUN: %clang -### --target=avr32 --sysroot=%S/Inputs/basic_avr32_tree \
// RUN:   -mpart=uc3a3256 -nostartfiles -O2 -mno-relax %s 2>&1 \
// RUN:   | FileCheck --check-prefix=NORELAX-LINK %s
// NORELAX-LINK: "{{.*}}ld.lld"
// NORELAX-LINK-NOT: "--relax"
