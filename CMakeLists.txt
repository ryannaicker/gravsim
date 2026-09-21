cmake_minimum_required(VERSION 3.20)
project(gravsim LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)      # for clangd / VS Code intellisense
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# ---- core: header-only for now, so an INTERFACE library ---------------------
# An INTERFACE library produces no binary. It is a bundle of "here is an include
# path and some flags", which is exactly what a header-only library is.
# When core gains .cpp files, change INTERFACE -> STATIC and add sources.
add_library(core INTERFACE)
target_include_directories(core INTERFACE ${CMAKE_SOURCE_DIR})

# ---- tests ------------------------------------------------------------------
enable_testing()
add_executable(tests tests/test_main.cpp)
target_link_libraries(tests PRIVATE core)
add_test(NAME unit_tests COMMAND tests)

# ---- headless experiments (Days 2-5, 7) -------------------------------------
#foreach(exp exp_energy_drift exp_mercury exp_chaos exp_rotation_curve)
#  add_executable(${exp} experiments/${exp}.cpp)
#  target_link_libraries(${exp} PRIVATE core)
#endforeach()

# ---- the windowed application (Day 6) ---------------------------------------
# Everything above builds with zero external dependencies. Only the app needs
# graphics libraries, so it is gated: you can do Days 1-5 before installing
# anything.
option(GRAVSIM_BUILD_APP "Build the OpenGL application" ON)
if(GRAVSIM_BUILD_APP)
  find_package(glfw3 CONFIG QUIET)
  find_package(glm   CONFIG QUIET)
  find_package(imgui CONFIG QUIET)
  find_package(OpenGL QUIET)
  if(glfw3_FOUND AND glm_FOUND AND OpenGL_FOUND)
    add_executable(gravsim_app
        app/main.cpp
        gfx/glad.c                 # download glad for GL 4.3 core, drop it here
    )
    target_link_libraries(gravsim_app PRIVATE core glfw glm::glm OpenGL::GL)
    if(imgui_FOUND)
      target_link_libraries(gravsim_app PRIVATE imgui::imgui)
      target_compile_definitions(gravsim_app PRIVATE GRAVSIM_HAVE_IMGUI)
    endif()
    target_include_directories(gravsim_app PRIVATE ${CMAKE_SOURCE_DIR}/gfx)
  else()
    message(STATUS "graphics deps not found -- skipping gravsim_app")
  endif()
endif()
