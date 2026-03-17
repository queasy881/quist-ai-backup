cmake_minimum_required(VERSION 3.20)
project(VoxelReams VERSION 1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

# ── GLFW ──────────────────────────────────────────────────
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG        3.3.9
)
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

# ── GLM ───────────────────────────────────────────────────
FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG        0.9.9.8
)
FetchContent_MakeAvailable(glm)

# ── GLAD 2 (requires Python 3 at configure time) ─────────
FetchContent_Declare(
  glad
  GIT_REPOSITORY https://github.com/Dav1dde/glad.git
  GIT_TAG        v2.0.6
)
FetchContent_MakeAvailable(glad)
glad_add_library(glad_gl_core_46 REPRODUCIBLE API gl:core=4.6)

# ── stb (header-only) ────────────────────────────────────
FetchContent_Declare(
  stb
  GIT_REPOSITORY https://github.com/nothings/stb.git
  GIT_TAG        master
)
FetchContent_GetProperties(stb)
if(NOT stb_POPULATED)
  FetchContent_Populate(stb)
endif()

# ── Collect sources ───────────────────────────────────────
file(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS
  src/*.cpp
  src/*.c
)

add_executable(VoxelReams ${ENGINE_SOURCES})

target_include_directories(VoxelReams PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${stb_SOURCE_DIR}
)

target_link_libraries(VoxelReams PRIVATE
  glfw
  glm::glm
  glad_gl_core_46
)

# Copy shaders + assets next to binary
add_custom_command(TARGET VoxelReams POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders
    $<TARGET_FILE_DIR:VoxelReams>/shaders
)
add_custom_command(TARGET VoxelReams POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/assets
    $<TARGET_FILE_DIR:VoxelReams>/assets
)

if(MSVC)
  target_compile_options(VoxelReams PRIVATE /W3 /MP)
else()
  target_compile_options(VoxelReams PRIVATE -Wall -Wextra -O2)
  find_package(Threads REQUIRED)
  target_link_libraries(VoxelReams PRIVATE Threads::Threads)
endif()
