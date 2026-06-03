set(LLVM_LINK_COMPONENTS
  AVR32AsmParser
  AVR32Desc
  AVR32Info
  MC
  MCParser
  Support
  Target
  TargetParser
  )

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}/../../../lib/Target/AVR32
  ${CMAKE_CURRENT_BINARY_DIR}/../../../lib/Target/AVR32/MCTargetDesc
  ${LLVM_MAIN_SRC_DIR}/lib/Target/AVR32
  ${LLVM_MAIN_SRC_DIR}/lib/Target/AVR32/AsmParser
  ${LLVM_MAIN_SRC_DIR}/lib/Target/AVR32/MCTargetDesc
  )

add_llvm_target_unittest(AVR32Tests
  TargetInfoTest.cpp
  )
