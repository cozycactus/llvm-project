add_llvm_component_group(AVR32)

set(LLVM_TARGET_DEFINITIONS AVR32.td)

tablegen(LLVM AVR32GenInstrInfo.inc -gen-instr-info)
tablegen(LLVM AVR32GenAsmWriter.inc -gen-asm-writer)
tablegen(LLVM AVR32GenMCCodeEmitter.inc -gen-emitter)
tablegen(LLVM AVR32GenRegisterInfo.inc -gen-register-info)
tablegen(LLVM AVR32GenSubtargetInfo.inc -gen-subtarget)

add_public_tablegen_target(AVR32CommonTableGen)

add_llvm_component_library(LLVMAVR32
  AVR32Target.cpp

  LINK_COMPONENTS
  AVR32Desc
  AVR32Info
  Support

  ADD_TO_COMPONENT
  AVR32
)

add_subdirectory(MCTargetDesc)
add_subdirectory(TargetInfo)
