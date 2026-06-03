add_llvm_component_library(LLVMAVR32AsmParser
  AVR32AsmParser.cpp

  LINK_COMPONENTS
  AVR32Desc
  AVR32Info
  MC
  MCParser
  Support

  ADD_TO_COMPONENT
  AVR32
  )
