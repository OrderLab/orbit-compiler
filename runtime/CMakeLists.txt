set(libsrc
  gobj_tracker.c
)


add_library(OrbitTracker SHARED ${libsrc})
add_library(OrbitTracker-static STATIC ${libsrc})

set_property(TARGET OrbitTracker PROPERTY POSITION_INDEPENDENT_CODE TRUE)
set_target_properties(OrbitTracker-static PROPERTIES OUTPUT_NAME OrbitTracker)

