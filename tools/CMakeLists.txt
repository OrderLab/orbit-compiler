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
add_executable(instrumentor instrumentor/main.cpp)
target_link_libraries(instrumentor
  PUBLIC  DefUse
  PRIVATE Instrumenter
  PRIVATE OrbitTracker
  PRIVATE Utils
  PRIVATE Matcher
)
target_link_libraries(instrumentor
  PRIVATE ${llvm_irreader}
  PRIVATE ${llvm_support}
  PRIVATE ${llvm_core}
  PRIVATE ${llvm_bitwriter}
)
