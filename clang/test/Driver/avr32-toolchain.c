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
