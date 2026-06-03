set(LLVM_LINK_COMPONENTS
  AVR32Info
  MC
  Support
  Target
  TargetParser
  )

add_llvm_target_unittest(AVR32Tests
  TargetInfoTest.cpp
  )
