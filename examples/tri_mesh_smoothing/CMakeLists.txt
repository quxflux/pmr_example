project(tri_mesh_smoothing)

add_executable(${PROJECT_NAME} "src/main.cpp" "src/tri_mesh.cpp" "src/laplacian_smoothing.h" "src/laplacian_smoothing.cpp" "src/abstract_base.h")
target_link_libraries(${PROJECT_NAME} base_project)

set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 20
                                                 CXX_STANDARD_REQUIRED ON
                                                 CXX_EXTENSIONS OFF)
