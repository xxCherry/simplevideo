project(node-bindings)

file(
  GLOB_RECURSE
  SOURCES
  src/*.cpp
)

if (NOT (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC"))
  add_compile_options(
    -Wall
    -Wextra
    -fno-rtti
    -Wno-missing-designated-field-initializers
  )
endif()

add_library(
  ${PROJECT_NAME}
  SHARED
  ${SOURCES}
)

add_custom_command(
  TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          ${FFMPEG_BIN_DIR}
          ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release
)

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  target_link_options(${PROJECT_NAME} PUBLIC "/DELAYLOAD:node.exe")
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
    SUFFIX ".node"
)

target_include_directories(${PROJECT_NAME} PRIVATE src)

target_link_libraries(${PROJECT_NAME} PRIVATE node)
target_link_libraries(${PROJECT_NAME} PRIVATE
    avcodec
    avformat
    avutil
    swscale
)