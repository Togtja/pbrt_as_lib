cmake_minimum_required(VERSION 3.12)

set(PROJECT_NAME "pbrt-cpp")
project(${PROJECT_NAME})
set(SOURCE_FILES main.cpp)

include_directories(
  external/pbrt-v4/src/pbrt
  ${CMAKE_BINARY_DIR}/external/pbrt-v4
)
link_directories(${CMAKE_BINARY_DIR}/external/pbrt-v4)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/external/pbrt-v4/cmake)
set(PBRT_SOURCE_CODE ${CMAKE_SOURCE_DIR}/external/pbrt-v4)

add_definitions(-D PBRT_FLOAT_AS_DOUBLE)
add_subdirectory(external/pbrt-v4)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)

target_link_libraries(${PROJECT_NAME}
  pbrt_lib
)
