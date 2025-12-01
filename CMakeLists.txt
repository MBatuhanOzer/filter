cmake_minimum_required(VERSION 3.10)
project(Filter_App LANGUAGES CXX)

# Only MSVC allowed
if(NOT MSVC)
  message(FATAL_ERROR "This project only supports MSVC for Windows.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# options to control which executables are created
option(BUILD_APP "Build filter_app (UI/main.cpp)" OFF)
option(BUILD_TEST "Build filter_test (tests/filter_test_engine.cpp)" ON)
option(BUILD_DEBUG_TEST "Build debug_test (tests/debug_test_main.cpp)" OFF)

# -- core library (static)
add_library(filter_core STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/src/filter-engine/filter.cpp
)

target_include_directories(filter_core PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/src/filter_engine
)

if(MSVC)
  target_compile_options(filter_core PRIVATE /WX /ZI)
  target_compile_definitions(filter_core PRIVATE PLATFORM_WINDOWS=1)
endif()

# -- UI executable (final main)
if(BUILD_APP)
  add_executable(filter_app
    ${CMAKE_CURRENT_SOURCE_DIR}/src/UI/main.cpp
  )
  target_include_directories(filter_app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/UI
  )
  target_link_libraries(filter_app PRIVATE filter_core)
endif()

# -- Test executable (test_main / speedtest)
if(BUILD_TEST)
  add_executable(filter_test
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/filter_engine_speedtest.cpp
  )
  # If your test needs headers from tests/, expose that dir
  target_include_directories(filter_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/tests
  )
  target_link_libraries(filter_test PRIVATE filter_core)
endif()

# -- Optional debug-test executable
if(BUILD_DEBUG_TEST)
  add_executable(debug_test
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/debug_test_main.cpp
  )
  target_link_libraries(debug_test PRIVATE filter_core)
endif()
