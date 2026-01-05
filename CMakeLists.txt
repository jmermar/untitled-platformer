cmake_minimum_required(VERSION 3.22) # CMake version check
project(app)                         # Create project "app"
set(C_STANDARD 11)           # Enable C++20 standard

set(DAWN_FETCH_DEPENDENCIES ON)
add_subdirectory("dawn" EXCLUDE_FROM_ALL)

file(GLOB all_sources 
    "src/*.c"
    "src/render/*.c"
)

add_executable(app 
    ${all_sources}
)

target_link_libraries(app PRIVATE webgpu_dawn webgpu_glfw glfw)