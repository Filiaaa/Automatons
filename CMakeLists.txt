cmake_minimum_required(VERSION 3.16)
project(DeterminationAndMinimalization LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(ENABLE_COVERAGE "Build tests with gcov coverage flags" OFF)

add_executable(automata main.cpp)
target_include_directories(automata PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

include(FetchContent)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(googletest)

enable_testing()

# One translation unit so gcov attributes header-only coverage correctly.
add_executable(automata_tests tests/all_tests.cpp)
target_include_directories(automata_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(automata_tests PRIVATE GTest::gtest_main)

if(ENABLE_COVERAGE)
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(automata_tests PRIVATE --coverage -O0 -g)
    target_link_options(automata_tests PRIVATE --coverage)
  else()
    message(WARNING "ENABLE_COVERAGE is supported with GCC/Clang only")
  endif()
endif()

include(GoogleTest)
gtest_discover_tests(automata_tests)
