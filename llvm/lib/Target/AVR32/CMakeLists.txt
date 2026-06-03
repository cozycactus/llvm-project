add_llvm_component_group(AVR32)

add_llvm_component_library(LLVMAVR32
  AVR32Target.cpp

  LINK_COMPONENTS
  AVR32Info
  Support

  ADD_TO_COMPONENT
  AVR32
)

add_subdirectory(TargetInfo)
