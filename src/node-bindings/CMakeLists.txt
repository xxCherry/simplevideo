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

set(EXTENSION ".dll")
set(LIB_PREFIX "")
set(FFMPEG_LIBRARY_DIR ${FFMPEG_BIN_DIR})

if (UNIX)
  set(EXTENSION ".so")
  set(LIB_PREFIX "lib")
  set(FFMPEG_LIBRARY_DIR ${FFMPEG_LIB_DIR})
endif()

set(COPY_POSTFIX "")

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  set(COPY_POSTFIX "/Release")
endif()

function(copy_ffmpeg_lib LIB_NAME)
  file(GLOB LIB_FILES
    "${FFMPEG_LIBRARY_DIR}/${LIB_PREFIX}${LIB_NAME}*${EXTENSION}"
  )

  list(GET LIB_FILES 0 LIB_FILE)

  add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            "${LIB_FILE}"
            "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}${COPY_POSTFIX}/"
  )
endfunction()

copy_ffmpeg_lib("avcodec")
copy_ffmpeg_lib("avdevice")
copy_ffmpeg_lib("avfilter")
copy_ffmpeg_lib("avformat")
copy_ffmpeg_lib("avutil")
copy_ffmpeg_lib("swresample")
copy_ffmpeg_lib("swscale")

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  target_link_options(${PROJECT_NAME} PUBLIC "/DELAYLOAD:node.exe")
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
    SUFFIX ".node"
)

target_include_directories(${PROJECT_NAME} PRIVATE src)

if (WIN32)
  target_link_libraries(${PROJECT_NAME} PRIVATE node)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE
    avcodec
    avformat
    avutil
    swscale
)