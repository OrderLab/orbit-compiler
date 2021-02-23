add_executable(extractor extractor/main.cpp)
target_link_libraries(extractor
  PUBLIC Utils
  PUBLIC DefUse
)
target_link_libraries(extractor
  PRIVATE ${llvm_irreader}
  PRIVATE ${llvm_support}
  PRIVATE ${llvm_core}
)
