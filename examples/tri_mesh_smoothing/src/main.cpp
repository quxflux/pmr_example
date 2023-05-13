#include "tri_mesh.h"
#include "laplacian_smoothing.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <numeric>
#include <ranges>

namespace
{
  namespace qf = quxflux;

  template<typename StorageType, typename Duration = std::chrono::milliseconds>
  auto smooth(const qf::tri_mesh& mesh, const std::filesystem::path& output_path)
  {
    const auto copy = mesh.clone();

    const auto start = std::chrono::high_resolution_clock::now();
    qf::laplacian_smoothing<StorageType>(*copy.get(), 10);
    const auto dur = std::chrono::duration_cast<Duration>(std::chrono::high_resolution_clock::now() - start);
    // exclude IO from measurement
    qf::write_to_file(*copy.get(), output_path);
    return dur;
  }

  double calculate_average_vertex_valence(const qf::tri_mesh& mesh)
  {
    const auto vertex_valences = std::views::iota(size_t{0}, mesh.get_num_vertices())  //
                                 | std::views::transform(std::bind_front(&qf::tri_mesh::get_vertex_valence, &mesh));

    return static_cast<double>(
             std::accumulate(std::ranges::begin(vertex_valences), std::ranges::end(vertex_valences), size_t{0})) /
           static_cast<double>(mesh.get_num_vertices());
  }
}  // namespace

int main()
{
  const auto sphere = qf::generate_noisy_unit_sphere(9, 0.01f);

  std::cout.precision(1);
  std::cout << "mesh is built up of " << sphere->get_num_vertices() << " vertices and " << sphere->get_num_faces()
            << " faces\n";
  std::cout << "average vertex valence: " << calculate_average_vertex_valence(*sphere.get()) << "\n";

  qf::write_to_file(*sphere.get(), "noisy_sphere.obj");

  std::cout << "impl with std::vector took "
            << smooth<qf::allocation_strategy::use_vector>(*sphere.get(), "smoothed_sphere_0.obj") << '\n';
  std::cout << "impl with std::pmr::vector took "
            << smooth<qf::allocation_strategy::use_pmr_vector>(*sphere.get(), "smoothed_sphere_2.obj") << '\n';

  return EXIT_SUCCESS;
}
