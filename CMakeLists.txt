cmake_minimum_required(VERSION 3.28)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(video)

set(CMAKE_CXX_STANDARD          23)
set(CMAKE_CXX_EXTENSIONS        ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/dist)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/dist)

if (WIN32)
set(FFMPEG_DOWNLOAD_URL https://www.gyan.dev/ffmpeg/builds/ffmpeg-release-full-shared.7z)
else()
set(FFMPEG_DOWNLOAD_URL https://github.com/BtbN/FFmpeg-Builds/releases/download/latest/ffmpeg-master-latest-linux64-gpl-shared.tar.xz)
endif()

set(NODE_DOWNLOAD_URL https://nodejs.org/dist/v24.8.0/win-x64/node.lib)
set(NODE_API_INCLUDE_URL https://github.com/nodejs/node-api-headers/raw/refs/heads/main/include)
set(NODE_ADDON_API_INCLUDE_URL https://raw.githubusercontent.com/nodejs/node-addon-api/refs/heads/main/)

include(FetchContent)

FetchContent_Declare(
  ffmpeg
  URL ${FFMPEG_DOWNLOAD_URL}
)

FetchContent_MakeAvailable(ffmpeg)

# node headers download

if (WIN32)
    file(DOWNLOAD
        ${NODE_DOWNLOAD_URL}
        ${CMAKE_SOURCE_DIR}/lib/node.lib)
endif()

file(DOWNLOAD
    ${NODE_API_INCLUDE_URL}/js_native_api.h
    ${CMAKE_SOURCE_DIR}/include/js_native_api.h)

file(DOWNLOAD
    ${NODE_API_INCLUDE_URL}/js_native_api_types.h
    ${CMAKE_SOURCE_DIR}/include/js_native_api_types.h)

file(DOWNLOAD
    ${NODE_API_INCLUDE_URL}/node_api.h
    ${CMAKE_SOURCE_DIR}/include/node_api.h)

file(DOWNLOAD
    ${NODE_API_INCLUDE_URL}/node_api_types.h
    ${CMAKE_SOURCE_DIR}/include/node_api_types.h)

file(DOWNLOAD
    ${NODE_ADDON_API_INCLUDE_URL}/napi-inl.h
    ${CMAKE_SOURCE_DIR}/include/napi-inl.h)

file(DOWNLOAD
    ${NODE_ADDON_API_INCLUDE_URL}/napi.h
    ${CMAKE_SOURCE_DIR}/include/napi.h)

add_compile_definitions(NODE_ADDON_API_DISABLE_DEPRECATED)

FetchContent_GetProperties(ffmpeg)
if(NOT ffmpeg_POPULATED)
  FetchContent_Populate(ffmpeg)
endif()

set(FFMPEG_INCLUDE_DIR ${ffmpeg_SOURCE_DIR}/include)
set(FFMPEG_LIB_DIR     ${ffmpeg_SOURCE_DIR}/lib)
set(FFMPEG_BIN_DIR     ${ffmpeg_SOURCE_DIR}/bin)

include_directories(include)
include_directories(src/video)
include_directories(${FFMPEG_INCLUDE_DIR})

link_directories(lib)
link_directories(${FFMPEG_LIB_DIR})

add_subdirectory(src)