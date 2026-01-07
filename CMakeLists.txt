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

set_target_properties(app PROPERTIES COMPILE_FLAGS "${CMAKE_CXX_FLAGS} -Werror")


target_link_libraries(app PRIVATE webgpu_dawn webgpu_glfw glfw)

add_custom_target(copy_res
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/res/"
        "${CMAKE_CURRENT_BINARY_DIR}/res"
    COMMENT "Copying resources"
)
add_dependencies(app copy_res)