add_library(DefUse SHARED
  DefUse/DefUse.cpp
)

add_library(HelloWorld SHARED
  HelloWorld/HelloWorld.cpp
)

add_library(ArgumentCounter SHARED
  ArgumentCounter/ArgumentCounter.cpp
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
