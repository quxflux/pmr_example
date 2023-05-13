#include "tri_mesh.h"

#include <algorithm>
#include <array>
#include <fstream>
#include <functional>
#include <numeric>
#include <random>
#include <ranges>
#include <unordered_set>
#include <unordered_map>
#include <vector>

namespace quxflux
{
  namespace
  {
    struct tri_mesh_impl : tri_mesh
    {
      size_t get_num_vertices() const final { return vertices_.size(); }

      void get_vertex(const vertex_index i, float* const data) const final { std::ranges::copy(vertices_[i], data); }

      void set_vertex(const vertex_index i, const float* const data) final
      {
        std::ranges::copy_n(data, 3, vertices_[i].begin());
      }

      size_t get_vertex_valence(const vertex_index i) const final { return vertices_neighbors_[i].size(); }

      size_t get_vertex_neighbors(const vertex_index i, vertex_index* const buf, const size_t buf_size) const final
      {
        const auto& neighbors = vertices_neighbors_[i];
        const auto n = std::min(buf_size, neighbors.size());
        std::ranges::copy_n(neighbors.begin(), n, buf);

        return n;
      }

      size_t get_num_faces() const final { return faces_.size(); }

      void get_face(const face_index i, vertex_index* const data) const final { std::ranges::copy(faces_[i], data); }

      void set_face(const face_index i, const vertex_index* const data) final
      {
        std::ranges::copy_n(data, 3, faces_[i].begin());
      }

      std::unique_ptr<tri_mesh> clone() const final { return std::make_unique<tri_mesh_impl>(*this); }

      void add_vertex(const vec3f& v)
      {
        vertices_.push_back(v);
        vertices_neighbors_.push_back({});
      }

      void add_face(const std::array<vertex_index, 3>& face)
      {
        faces_.push_back(face);

        for (size_t i = 0; i < 3; ++i)
        {
          const std::array neighbors{face[(i + 1) % 3], face[(i + 2) % 3]};

          auto& this_vertex_neighbors = vertices_neighbors_[face[i]];

          const auto already_present = [&](const auto neighbor) {
            return std::ranges::find(this_vertex_neighbors, neighbor) != this_vertex_neighbors.end();
          };

          std::ranges::copy_if(neighbors, std::back_inserter(this_vertex_neighbors), std::not_fn(already_present));
        }
      }

    private:
      std::vector<vec3f> vertices_;
      std::vector<std::vector<vertex_index>> vertices_neighbors_;
      std::vector<std::array<vertex_index, 3>> faces_;
    };

    template<typename T>
    constexpr T pairing_func(const T x, const T y)
    {
      return ((x + y) * (x + y + 1)) / 2 + y;
    }

    vec3f normalized(const vec3f& v)
    {
      vec3f result = v;
      std::ranges::transform(result, result, result.begin(), std::multiplies<>{});

      const auto len_recip = 1.f / std::sqrt(std::accumulate(result.begin(), result.end(), 0.f));
      std::ranges::transform(v, result.begin(), std::bind_front(std::multiplies<>{}, len_recip));

      return result;
    }

    vec3f midpoint(const vec3f& a, const vec3f& b)
    {
      vec3f r;
      std::ranges::transform(a, b, r.begin(), [](const auto t0, const auto t1) { return std::midpoint(t0, t1); });
      return r;
    }
  }  // namespace

  std::unique_ptr<tri_mesh> read_from_file(const std::filesystem::path& path)
  {
    auto mesh = std::make_unique<tri_mesh_impl>();

    std::ifstream ifs;
    ifs.open(path);

    while (ifs.good())
    {
      char firstChar;

      ifs >> firstChar;

      firstChar = std::tolower(firstChar, std::locale());

      switch (firstChar)
      {
        case 'v': {
          vec3f t;

          ifs >> t[0];
          ifs >> t[1];
          ifs >> t[2];

          mesh->add_vertex(t);
        }
        break;
        case 'f': {
          std::array<vertex_index, 3> face_indices;

          ifs >> face_indices[0];
          ifs >> face_indices[1];
          ifs >> face_indices[2];

          // vertex indices in obj are 1-based
          std::ranges::transform(face_indices, face_indices.begin(), [](const auto i) { return i - 1; });

          mesh->add_face(face_indices);
        }
        break;
        default:
          break;
      }

      ifs.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }

    return mesh;
  }

  void write_to_file(const tri_mesh& mesh, const std::filesystem::path& path)
  {
    std::ofstream ofs;
    ofs.exceptions(std::ios_base::failbit);
    ofs.open(path);

    for (auto v : mesh_vertices(mesh))
      ofs << "v " << v[0] << ' ' << v[1] << ' ' << v[2] << '\n';

    for (auto f : mesh_faces(mesh))
    {
      // vertex indices in obj are 1-based
      std::ranges::transform(f, f.begin(), std::bind_front(std::plus<>{}, size_t{1}));
      ofs << "f " << f[0] << ' ' << f[1] << ' ' << f[2] << '\n';
    }
  }

  std::unique_ptr<tri_mesh> generate_noisy_unit_sphere(const size_t num_sudivisions, const float stddev)
  {
    using face = std::array<vertex_index, 3>;

    struct face_hasher
    {
      constexpr auto operator()(const face& f) const noexcept { return pairing_func(pairing_func(f[0], f[1]), f[2]); }
    };

    struct edge_hasher
    {
      constexpr auto operator()(const std::pair<vertex_index, vertex_index>& edge) const noexcept
      {
        return pairing_func(edge.first, edge.second);
      }
    };

    using faces_container = std::unordered_set<face, face_hasher>;

    // create the sphere by incrementally subdividing an octahedron and reprojecting the resulting vertices onto the
    // unit sphere
    static constexpr auto octahedron_vertices = std::to_array<vec3f>(
      {{0, 1.f, 0}, {1.f, 0, 0}, {0, 0, 1.f}, {-1.f, 0, 0}, {0, 0, -1.f}, {0, -1.f, 0}});
    static constexpr auto octahedron_faces = std::to_array<face>(
      {{0, 2, 1}, {0, 3, 2}, {0, 4, 3}, {0, 1, 4}, {5, 1, 2}, {5, 2, 3}, {5, 3, 4}, {5, 4, 1}});

    std::vector<vec3f> vertices{octahedron_vertices.begin(), octahedron_vertices.end()};
    faces_container faces{octahedron_faces.begin(), octahedron_faces.end()};

    std::unordered_map<std::pair<vertex_index, vertex_index>, vertex_index, edge_hasher> edge_vertex_map;

    const auto create_or_reuse_midpoint_vertex = [&](const vertex_index v0, const vertex_index v1) -> vertex_index {
      const std::pair edge{std::min(v0, v1), std::max(v0, v1)};

      auto it = edge_vertex_map.find(edge);

      if (it == edge_vertex_map.end())
      {
        vertices.push_back(normalized(midpoint(vertices[v0], vertices[v1])));
        it = edge_vertex_map.insert(std::pair{edge, vertices.size() - 1}).first;
      }

      return it->second;
    };

    for (size_t current_level = 0; current_level < num_sudivisions; ++current_level)
    {
      faces_container this_level_faces;

      for (const auto& f : faces)
      {
        const auto a = create_or_reuse_midpoint_vertex(f[0], f[1]);
        const auto b = create_or_reuse_midpoint_vertex(f[1], f[2]);
        const auto c = create_or_reuse_midpoint_vertex(f[2], f[0]);

        this_level_faces.insert({a, b, c});
        this_level_faces.insert({f[0], a, c});
        this_level_faces.insert({a, f[1], b});
        this_level_faces.insert({c, b, f[2]});
      }

      std::swap(faces, this_level_faces);
    }

    const auto perturbe_vertex = [rd = std::mt19937{42},
                                  dis = std::normal_distribution<float>{1.f, stddev}](const vec3f& v) mutable {
      vec3f r{};
      std::ranges::transform(v, r.begin(), std::bind_front(std::multiplies<>{}, dis(rd)));
      return r;
    };

    auto mesh = std::make_unique<tri_mesh_impl>();
    std::ranges::for_each(vertices, std::bind_front(&tri_mesh_impl::add_vertex, mesh.get()), perturbe_vertex);
    std::ranges::for_each(faces, std::bind_front(&tri_mesh_impl::add_face, mesh.get()));
    return mesh;
  }
}  // namespace quxflux
