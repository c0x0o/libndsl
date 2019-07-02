cmake_minimum_required(VERSION 3.0.0)

project(libndsl)

# for cpp
set(CMAKE_CXX_STANDARD 11)

# for asm
enable_language(ASM)

include(${libndsl_SOURCE_DIR}/src/sources.cmake)

add_library(libndsl STATIC ${LIBNDSL_SRC})

include_directories(${libndsl_SOURCE_DIR}/src)
add_executable(coroutine test/testcase-coroutine.cc)
add_executable(echo-server test/testcase-echo-server.cc)
target_link_libraries(coroutine libndsl)
target_link_libraries(echo-server libndsl)
