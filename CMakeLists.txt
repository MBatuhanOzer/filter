cmake_minimum_required(VERSION 3.14)

project(FilterApp VERSION 1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (NOT WIN32)
    message(FATAL_ERROR "This project is only supported on Windows.")
endif()

# -----------------------------------------------------------------------------
# 1. Local Dependencies (Built from Source)
# -----------------------------------------------------------------------------

# --- GLFW ---
# We disable GLFW options we don't need to speed up the build
set(GLFW_BUILD_EXAMPLES OFF CACHE INTERNAL "Build the GLFW example programs")
set(GLFW_BUILD_TESTS OFF CACHE INTERNAL "Build the GLFW test programs")
set(GLFW_BUILD_DOCS OFF CACHE INTERNAL "Build the GLFW documentation")
set(GLFW_INSTALL OFF CACHE INTERNAL "Generate installation target")

# Add the subdirectory. This runs the CMakeLists.txt inside dependencies/glfw-3.4
add_subdirectory(dependencies/glfw-3.4)

# --- Dear ImGui ---
# ImGui doesn't have a standard CMake build file, so we define a library target here.
set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/dependencies/imgui-1.92.5")

# Select the specific backend files we need for GLFW + OpenGL3
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

# Create a static library for ImGui so we don't have to recompile it for every app
add_library(imgui_lib STATIC ${IMGUI_SOURCES})

# Tell ImGui where its headers are
target_include_directories(imgui_lib PUBLIC 
    ${IMGUI_DIR} 
    ${IMGUI_DIR}/backends
)

# ImGui needs to link against GLFW to access window functions
target_link_libraries(imgui_lib PUBLIC glfw)


# -----------------------------------------------------------------------------
# 2. Filter Engine Core (Library)
# -----------------------------------------------------------------------------
add_library(filter_core STATIC
    src/filter-engine/filter.cpp
    src/filter-engine/filter.h
)

target_include_directories(filter_core PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/filter-engine
    ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/stb
)

# -----------------------------------------------------------------------------
# 3. UI Application
# -----------------------------------------------------------------------------
add_executable(FilterUI
    WIN32
    src/UI/main.cpp
)

# Tell the linker to use mainCRTStartup as the entry point to avoid console window
set_target_properties(FilterUI PROPERTIES LINK_FLAGS "/ENTRY:mainCRTStartup")

# Link our local libraries
target_link_libraries(FilterUI PRIVATE 
    filter_core
    imgui_lib
    glfw
    opengl32
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/out)

# -----------------------------------------------------------------------------
# 4. Speed Test Application
# -----------------------------------------------------------------------------
add_executable(SpeedTest
    tests/filter_engine_speedtest.cpp
)

target_link_libraries(SpeedTest PRIVATE 
    filter_core
)

# -----------------------------------------------------------------------------
# 5. Windows Config
# -----------------------------------------------------------------------------
if(WIN32)
    add_compile_definitions(UNICODE _UNICODE)
endif()