add_llvm_component_group(AVR32)

set(LLVM_TARGET_DEFINITIONS AVR32.td)

tablegen(LLVM AVR32GenInstrInfo.inc -gen-instr-info)
tablegen(LLVM AVR32GenAsmMatcher.inc -gen-asm-matcher)
tablegen(LLVM AVR32GenAsmWriter.inc -gen-asm-writer)
tablegen(LLVM AVR32GenDAGISel.inc -gen-dag-isel)
tablegen(LLVM AVR32GenMCCodeEmitter.inc -gen-emitter)
tablegen(LLVM AVR32GenRegisterInfo.inc -gen-register-info)
tablegen(LLVM AVR32GenSDNodeInfo.inc -gen-sd-node-info)
tablegen(LLVM AVR32GenSubtargetInfo.inc -gen-subtarget)

add_public_tablegen_target(AVR32CommonTableGen)

add_llvm_target(AVR32CodeGen
  AVR32AsmPrinter.cpp
  AVR32FrameLowering.cpp
  AVR32ISelDAGToDAG.cpp
  AVR32ISelLowering.cpp
  AVR32InstrInfo.cpp
  AVR32RegisterInfo.cpp
  AVR32SelectionDAGInfo.cpp
  AVR32Subtarget.cpp
  AVR32Target.cpp
  AVR32TargetMachine.cpp

  LINK_COMPONENTS
  AsmPrinter
  AVR32Desc
  AVR32Info
  CodeGen
  Core
  MC
  SelectionDAG
  Support
  Target

  ADD_TO_COMPONENT
  AVR32
)

add_subdirectory(MCTargetDesc)
add_subdirectory(AsmParser)
add_subdirectory(TargetInfo)
