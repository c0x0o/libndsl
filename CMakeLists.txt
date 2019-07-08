cmake_minimum_required(VERSION 3.0.0)

project(libndsl)

# for cpp
set(CMAKE_CXX_STANDARD 11)
# for asm
enable_language(ASM)

# debug options
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -pg")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -pg")
set(CMAKE_SHARED_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -pg")

# release options
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

include_directories(${libndsl_SOURCE_DIR}/src)

include(${libndsl_SOURCE_DIR}/src/sources.cmake)
add_library(libndsl STATIC ${LIBNDSL_SRC})

add_executable(coroutine test/testcase-coroutine.cc)
add_executable(echo-server test/testcase-echo-server.cc)
add_executable(echo-client test/testcase-echo-client.cc)
target_link_libraries(coroutine libndsl)
target_link_libraries(echo-server libndsl)
target_link_libraries(echo-client libndsl)
