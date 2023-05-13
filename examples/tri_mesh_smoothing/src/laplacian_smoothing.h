#pragma once

namespace quxflux
{
  struct tri_mesh;

  namespace allocation_strategy
  {
    // clang-format off
    struct use_vector {};
    struct use_pmr_vector {};
    // clang-format on
  }  // namespace allocation_strategy

  template<typename AllocationStrategy>
  void laplacian_smoothing(tri_mesh& mesh, const size_t iterations);
}  // namespace quxflux
