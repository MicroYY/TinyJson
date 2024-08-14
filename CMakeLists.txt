#cmake_minimum_required (VERSION 2.6)
project (TinyJson)

if (CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ansi -pedantic -Wall")
endif()

add_library(tinyjson tinyjson.cpp)
add_executable(tinyjson_test test.cpp)
target_link_libraries(tinyjson_test tinyjson)