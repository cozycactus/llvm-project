add_llvm_component_library(LLVMAVR32Info
  AVR32TargetInfo.cpp

  LINK_COMPONENTS
  MC
  Support

  ADD_TO_COMPONENT
  AVR32
)
