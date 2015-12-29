cmake_minimum_required(VERSION 3.2)
project(Ebc)

add_definitions(-std=c++11)
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
#set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -fsanitize=address")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")

file(GLOB SOURCE_FILES "../src/*.cpp")
include_directories("../include")

file(GLOB TEST_FILES "*.cpp")

add_executable(EbcTests ${SOURCE_FILES} ${TEST_FILES})

target_link_libraries(EbcTests LLVM-3.4)
