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

// RUN: %clang -### --target=avr32 -mpart=uc3a3256 -O2 \
// RUN:   -c %s 2>&1 | FileCheck --check-prefix=OPT-RELAX-CC1 %s
// OPT-RELAX-CC1: "-target-feature" "+relax"

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
