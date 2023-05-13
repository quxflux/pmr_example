#pragma once

namespace quxflux
{
  struct tri_mesh;

  namespace allocation_strategy
  {
    namespace detail
    {
      // clang-format off
      struct use_vector_t {};
      struct use_pmr_vector_t {};
      // clang-format on
    }  // namespace detail

    static constexpr detail::use_vector_t use_vector;
    static constexpr detail::use_pmr_vector_t use_pmr_vector;
  }  // namespace allocation_strategy

  template<typename AllocationStrategy>
  void laplacian_smoothing(tri_mesh& mesh, const size_t iterations, const AllocationStrategy&);
}  // namespace quxflux
