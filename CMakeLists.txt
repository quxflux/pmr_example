cmake_minimum_required(VERSION 3.22)

project(pmr_example LANGUAGES CXX)

include(cmake/CompilerWarnings.cmake)

add_library(base_project INTERFACE)
set_project_warnings(base_project)


add_subdirectory(examples/mem_resource_chaining)
add_subdirectory(examples/tri_mesh_smoothing)
add_subdirectory(examples/allocator_aware_object)
