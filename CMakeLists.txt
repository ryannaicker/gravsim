cmake_minimum_required(VERSION 3.20)
project(gravsim LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# ---- core: header-only library ---------------------------------------------
add_library(core INTERFACE)
target_include_directories(core INTERFACE ${CMAKE_SOURCE_DIR})

# ---- tests ------------------------------------------------------------------
enable_testing()
add_executable(tests tests/test_main.cpp)
target_link_libraries(tests PRIVATE core)
add_test(NAME unit_tests COMMAND tests)

# ---- experiments ------------------------------------------------------------
add_executable(exp_energy_drift experiments/exp_energy_drift.cpp)
target_link_libraries(exp_energy_drift PRIVATE core)

# ---- the window ---------------------------------------------------------------
option(GRAVSIM_BUILD_APP "Build the OpenGL application" ON)
if(GRAVSIM_BUILD_APP)
  find_package(glfw3 REQUIRED)                 # from apt: libglfw3-dev
  find_package(glm REQUIRED)                   # from apt: libglm-dev
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(EPOXY REQUIRED IMPORTED_TARGET epoxy)   # from apt: libepoxy-dev

  # Dear ImGui isn't packaged for apt, so CMake downloads a fixed version.
  # GIT_TAG pins the exact version: the build can't change under you.
  include(FetchContent)
  FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.91.9
    GIT_SHALLOW    TRUE)
  FetchContent_MakeAvailable(imgui)

  # ImGui ships as source files, not a library, so we build it ourselves:
  # the core, plus the two "backends" that connect it to GLFW and OpenGL.
  add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp)
  target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
  target_link_libraries(imgui PUBLIC glfw)

  add_executable(gravsim_app app/main.cpp)
  target_link_libraries(gravsim_app PRIVATE core imgui glfw glm::glm PkgConfig::EPOXY)
endif()