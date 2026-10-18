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

# ---- experiments: add one line per experiment as you write them -------------
add_executable(exp_energy_drift experiments/exp_energy_drift.cpp)
target_link_libraries(exp_energy_drift PRIVATE core)
# Day 4: exp_chaos
# Day 5: exp_mercury
# Day 7: exp_rotation_curve

# ---- OpenGL app (Day 6) -----------------------------------------------------
option(GRAVSIM_BUILD_APP "Build the OpenGL application" ON)
if(GRAVSIM_BUILD_APP)
  find_package(glfw3 CONFIG QUIET)
  find_package(glm   CONFIG QUIET)
  find_package(imgui CONFIG QUIET)
  find_package(OpenGL QUIET)
  if(glfw3_FOUND AND glm_FOUND AND OpenGL_FOUND)
    add_executable(gravsim_app app/main.cpp gfx/glad.c)
    target_link_libraries(gravsim_app PRIVATE core glfw glm::glm OpenGL::GL)
    if(imgui_FOUND)
      target_link_libraries(gravsim_app PRIVATE imgui::imgui)
    endif()
    target_include_directories(gravsim_app PRIVATE ${CMAKE_SOURCE_DIR}/gfx)
  else()
    message(STATUS "graphics deps not found -- skipping gravsim_app")
  endif()
endif()