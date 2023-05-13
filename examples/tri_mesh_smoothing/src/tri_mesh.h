#pragma once

#include "abstract_base.h"

#include <array>
#include <filesystem>
#include <memory>
#include <ranges>

namespace quxflux
{
  using vec3f = std::array<float, 3>;
  using vertex_index = size_t;
  using face_index = size_t;

  struct tri_mesh : abstract_base
  {
    virtual std::unique_ptr<tri_mesh> clone() const = 0;

    virtual size_t get_num_vertices() const = 0;
    virtual void get_vertex(const vertex_index i, float* const data) const = 0;
    virtual void set_vertex(const vertex_index i, const float* const data) = 0;

    virtual size_t get_vertex_valence(const vertex_index i) const = 0;
    virtual size_t get_vertex_neighbors(const vertex_index i, vertex_index* const buf, const size_t buf_size) const = 0;

    virtual size_t get_num_faces() const = 0;
    virtual void get_face(const face_index i, vertex_index* const data) const = 0;
    virtual void set_face(const face_index i, const vertex_index* const data) = 0;
  };

  inline auto mesh_vertices(const tri_mesh& mesh)
  {
    return std::views::iota(uint32_t{0}, static_cast<uint32_t>(mesh.get_num_vertices()))  //
           | std::views::transform([&](const auto vi) {
               vec3f vertex;
               mesh.get_vertex(vi, vertex.data());
               return vertex;
             });
  }

  inline auto mesh_faces(const tri_mesh& mesh)
  {
    return std::views::iota(uint32_t{0}, static_cast<uint32_t>(mesh.get_num_faces()))  //
           | std::views::transform([&](const auto fi) {
               std::array<vertex_index, 3> f;
               mesh.get_face(fi, f.data());
               return f;
             });
  }

  std::unique_ptr<tri_mesh> read_from_file(const std::filesystem::path& path);
  void write_to_file(const tri_mesh& mesh, const std::filesystem::path& path);

  std::unique_ptr<tri_mesh> generate_noisy_unit_sphere(size_t subdivision_level, const float stddev);
}  // namespace quxflux
