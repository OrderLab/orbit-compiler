add_library(DefUse SHARED
  DefUse/DefUse.cpp
  Utils/LLVM.cpp
  CallGraph/CallGraph.cpp
)

add_library(ObiWanAnalysis SHARED
  ObiWanAnalysis/ObiWanAnalysis.cpp
  DefUse/DefUse.cpp
  Utils/LLVM.cpp
  CallGraph/CallGraph.cpp
)

add_library(ObiWanAnalysisPass SHARED
  ObiWanAnalysis/ObiWanAnalysisPass.cpp
  ObiWanAnalysis/ObiWanAnalysis.cpp
  DefUse/DefUse.cpp
  Utils/LLVM.cpp
  CallGraph/CallGraph.cpp
  Instrument/AllocInstrumenter.cpp
)

add_library(CallGraph SHARED
  CallGraph/CallGraph.cpp
  Utils/LLVM.cpp
)

add_library(HelloWorld SHARED
  HelloWorld/HelloWorld.cpp
)

add_library(StaticCallCounter SHARED  
  StaticCallCounter/StaticCallCounter.cpp
)

add_library(Utils SHARED
  Utils/String.cpp
  Utils/Path.cpp
  Utils/LLVM.cpp
)

add_library(LLVMDefUse MODULE
  DefUse/DefUsePass.cpp
)

target_link_libraries(LLVMDefUse
  PUBLIC DefUse
  PUBLIC Utils
)

add_library(Matcher SHARED
  Matcher/Scope.cpp
  Matcher/Matcher.cpp
)

add_library(Instrumenter SHARED
  Instrument/AllocInstrumenter.cpp
  Utils/LLVM.cpp
)

target_link_libraries(Instrumenter
  PRIVATE OrbitTracker
  PRIVATE ${llvm_transformutils}
)

add_library(LLVMInstrument SHARED
  Instrument/InstrumentAllocPass.cpp
)

target_link_libraries(LLVMInstrument
  PUBLIC Instrumenter
)

if (APPLE)
  set_target_properties(DefUse PROPERTIES
    LINK_FLAGS "-undefined dynamic_lookup"
  )
endif(APPLE)

if (APPLE)
  target_link_libraries(LLVMDefUse
				PRIVATE ${llvm_analysis}
				PRIVATE ${llvm_support}
				PRIVATE ${llvm_core})
endif(APPLE)

