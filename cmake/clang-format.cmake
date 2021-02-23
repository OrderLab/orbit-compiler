# Make clang-format into a make target to enforce coding style
# http://mariobadr.com/using-clang-format-to-enforce-style.html
find_program(
  CLANG_FORMAT_EXE
  NAMES "clang-format"
  DOC "Path to clang-format executable"
)
if(NOT CLANG_FORMAT_EXE)
  message(STATUS "clang-format not found.")
else()
  message(STATUS "clang-format found: ${CLANG_FORMAT_EXE}")
  set(DO_CLANG_FORMAT "${CLANG_FORMAT_EXE}" "-i -style=file")
endif()

file(GLOB_RECURSE FILES_TO_FORMAT
  ${CMAKE_SOURCE_DIR}/*.cpp
  ${CMAKE_SOURCE_DIR}/*.h
  ${CMAKE_SOURCE_DIR}/*.c
)

set(EXCLUDE_DIRS 
  ${CMAKE_SOURCE_DIR}/test
  "/CMakeFiles/"
)

foreach (SOURCE_FILE ${FILES_TO_FORMAT})
  foreach (EXCLUDE_DIR ${EXCLUDE_DIRS})
    string (FIND ${SOURCE_FILE} ${EXCLUDE_DIR} EXCLUDE_DIR_FOUND)
    if (NOT ${EXCLUDE_DIR_FOUND} EQUAL -1)
      list (REMOVE_ITEM FILES_TO_FORMAT ${SOURCE_FILE})
    endif ()
  endforeach()
endforeach()

if(CLANG_FORMAT_EXE)
  add_custom_target(
    format-all
    COMMENT "Formatting all source files"
    COMMAND ${CLANG_FORMAT_EXE} -i -style=file ${FILES_TO_FORMAT}
  )
  add_custom_target(
    format
    COMMENT "Format code style of changed source files"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMAND scripts/clang-format-changed.py --in-place --clang-format-bin ${CLANG_FORMAT_EXE}
  )
  add_custom_target(
    format-check-all
    COMMENT "Checking code style format of all source files"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMAND ${CLANG_FORMAT_EXE} -style=file ${FILES_TO_FORMAT}
  )
  add_custom_target(
    format-check
    COMMENT "Checking code style format of changed source files"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMAND scripts/clang-format-changed.py --check-only --clang-format-bin ${CLANG_FORMAT_EXE}
  )
endif()
