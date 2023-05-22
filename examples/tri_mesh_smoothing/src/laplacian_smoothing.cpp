#include "laplacian_smoothing.h"

#include "tri_mesh.h"

#include <algorithm>
#include <functional>
#include <memory_resource>
#include <span>

namespace quxflux
{
  namespace
  {
    vec3f get_vertex(const tri_mesh& mesh, const vertex_index i)
    {
      vec3f vertex{};
      mesh.get_vertex(i, vertex.data());
      return vertex;
    }

    template<typename AllocationStrategy>
    vec3f smoothed_vertex(const tri_mesh& mesh, const vertex_index i, const std::span<const vec3f> org_vertices)
    {
      const auto n = mesh.get_vertex_valence(i);

      if (n == 0) [[unlikely]]
        return get_vertex(mesh, i);

      const auto calculate_smoothed = [&](const std::span<const vertex_index> neighbor_indices) {
        vec3f smoothed{};

        for (const auto vi : neighbor_indices)
          std::ranges::transform(smoothed, org_vertices[vi], smoothed.begin(), std::plus<>{});

        const auto n_recip = 1.f / static_cast<float>(n);
        std::ranges::transform(smoothed, smoothed.begin(), std::bind_front(std::multiplies<>{}, n_recip));

        return smoothed;
      };

      if constexpr (std::same_as<AllocationStrategy, allocation_strategy::detail::use_vector_t>)
      {
        // naive implementation:
        // create a regular vector each time the function is called, resulting in a heap allocation
        // each time
        std::vector<vertex_index> neighbor_indices(n);
        mesh.get_vertex_neighbors(i, neighbor_indices.data(), n);
        return calculate_smoothed(neighbor_indices);
      } else if constexpr (std::same_as<AllocationStrategy, allocation_strategy::detail::use_pmr_vector_t>)
      {
        // optimized implementation using std::pmr::vector:
        // use a monotonic_buffer_resource with a preallocated stack-based buffer which holds space for
        // 6 vertices (which should be sufficient for most vertices).
        std::array<std::byte, sizeof(vertex_index) * 6> static_storage;
        std::pmr::monotonic_buffer_resource buf_resource{static_storage.data(), static_storage.size()};
        std::pmr::vector<vertex_index> neighbor_indices(n, &buf_resource);
        mesh.get_vertex_neighbors(i, neighbor_indices.data(), n);
        return calculate_smoothed(neighbor_indices);
      }
    }
  }  // namespace

  template<typename AllocationStrategy>
  void laplacian_smoothing(tri_mesh& mesh, const size_t num_iterations, const AllocationStrategy&)
  {
    const size_t n = mesh.get_num_vertices();
    std::vector<vec3f> org_vertices(n);

    for (size_t i = 0; i < num_iterations; ++i)
    {
      std::ranges::copy(mesh_vertices(mesh), org_vertices.begin());

      for (size_t vi = 0; vi < n; ++vi)
        mesh.set_vertex(vi, smoothed_vertex<AllocationStrategy>(mesh, vi, org_vertices).data());
    }
  }

  template void laplacian_smoothing<allocation_strategy::detail::use_vector_t>  //
    (tri_mesh&, size_t, const allocation_strategy::detail::use_vector_t&);

  template void laplacian_smoothing<allocation_strategy::detail::use_pmr_vector_t>  //
    (tri_mesh&, size_t, const allocation_strategy::detail::use_pmr_vector_t&);
}  // namespace quxflux
