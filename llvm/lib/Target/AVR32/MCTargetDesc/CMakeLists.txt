add_llvm_component_library(LLVMAVR32Desc
  AVR32AsmBackend.cpp
  AVR32InstPrinter.cpp
  AVR32MCCodeEmitter.cpp
  AVR32ELFObjectWriter.cpp
  AVR32MCAsmInfo.cpp
  AVR32MCTargetDesc.cpp

  LINK_COMPONENTS
  AVR32Info
  MC
  Support
  TargetParser

  ADD_TO_COMPONENT
  AVR32
  )
