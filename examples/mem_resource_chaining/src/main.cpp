#include <cstdlib>
#include <iostream>
#include <memory_resource>
#include <random>
#include <unordered_map>

namespace
{
  class tracking_mem_resource : public std::pmr::memory_resource
  {
  public:
    struct statistics
    {
      size_t n_allocations = 0;
      size_t n_deallocations = 0;
      size_t n_bytes_allocated = 0;
      size_t n_bytes_deallocated = 0;
    };

    constexpr explicit tracking_mem_resource(
      std::pmr::memory_resource* upstream_resource = std::pmr::new_delete_resource())
      : upstream_resource_(upstream_resource)
    {}

    constexpr const statistics& get_statistics() const { return statistics_; }

  private:
    void* do_allocate(size_t n_bytes, size_t alignment) final
    {
      ++statistics_.n_allocations;
      statistics_.n_bytes_allocated += n_bytes;
      return upstream_resource_->allocate(n_bytes, alignment);
    }

    void do_deallocate(void* ptr, size_t n_bytes, size_t alignment) final
    {
      ++statistics_.n_deallocations;
      statistics_.n_bytes_deallocated += n_bytes;
      upstream_resource_->deallocate(ptr, n_bytes, alignment);
    }

    bool do_is_equal(const memory_resource& that) const noexcept final { return this == &that; }

    statistics statistics_{};
    std::pmr::memory_resource* upstream_resource_;
  };

  std::ostream& operator<<(std::ostream& os, const tracking_mem_resource::statistics& stats)
  {
    os.setf(std::ios_base::fixed, std::ios_base::floatfield);
    os.precision(1);

    os << "allocated " << static_cast<float>(stats.n_bytes_allocated) / 1024 << " KiB in " << stats.n_allocations
       << " allocation requests.\n";
    os << "deallocated " << static_cast<float>(stats.n_bytes_deallocated) / 1024 << " KiB in " << stats.n_deallocations
       << " deallocation requests.";

    return os;
  }

  // this function performs the same insertions and deletions in an unordered_map on each
  // invocation. A memory resource has to be passed which provided the unordered_map with
  // memory.
  void perform_deterministic_random_map_ops(std::pmr::memory_resource* resource)
  {
    std::mt19937 rd{42};

    std::pmr::unordered_map<size_t, std::pair<size_t, float>> map{resource};

    size_t n_erased = 0;
    size_t n_inserted = 0;

    for (size_t i = 0, n_operations = std::uniform_int_distribution<size_t>{1000, 1000000}(rd); i < n_operations; ++i)
    {
      // 50% chance to insert a new element
      const auto delete_item = std::uniform_int_distribution<int>{0, 1}(rd) == 0;

      if (delete_item && !map.empty())
      {
        auto it = map.begin();
        std::advance(it, std::uniform_int_distribution<size_t>{0, map.size() - 1}(rd));
        map.erase(it);
        ++n_erased;
      } else
      {
        const auto key = std::uniform_int_distribution<size_t>{}(rd);
        const auto value = std::pair{std::uniform_int_distribution<size_t>{}(rd),
                                     std::uniform_real_distribution<float>{}(rd)};

        const bool did_insert = map.insert(std::pair{key, value}).second;
        n_inserted += did_insert;
      }
    }

    std::cout << "inserted " << n_inserted << " items, erased " << n_erased << " items\n";
  }
}  // namespace

int main()
{
  {
    // the default upstream resource is std::pmr::new_delete_resource(), so this will behave somewhat
    // similar to as when using a std::unordered_map instead of the std::pmr::unordered_map with
    // custom memory_resource.
    std::cout << "performing allocations with std::pmr::new_delete_resource() upstream resource\n";
    tracking_mem_resource tracking_mem_resource;
    perform_deterministic_random_map_ops(&tracking_mem_resource);
    std::cout << tracking_mem_resource.get_statistics() << "\n\n";
  }

  {
    // an unsynchronized_pool_resource will allocate chunks of memory which are uniformly divided into
    // chunks of equal size.
    // using this memory resource will drastically reduce the number of allocations requested by the
    // upstream resource because the chunks allocated by the unsynchronized_pool_resource will be reused.
    std::cout << "performing allocations with downstream std::pmr::unsynchronized_pool_resource\n";
    tracking_mem_resource tracking_mem_resource;
    {
      std::pmr::unsynchronized_pool_resource pool_res{&tracking_mem_resource};
      perform_deterministic_random_map_ops(&pool_res);
      std::cout << tracking_mem_resource.get_statistics() << '\n';
    }
    // unsynchronized_pool_resource will only free its remaining acquired memory once it goes out of scope.
    std::cout << "deallocating unsynchronized_pool_resource\n";
    std::cout << tracking_mem_resource.get_statistics() << "\n\n";
  }

  {
    // monotonic_buffer_resource will only ever allocate memory but only free allocated memory once it goes
    // out of scope. The size of the requested buffers hereby follows a geometric progression. Therefore
    // the number of allocations should be less than with the std::pmr::new_delete_resource().
    std::cout << "performing allocations with downstream std::pmr::monotonic_buffer_resource\n";
    tracking_mem_resource tracking_mem_resource{};
    {
      std::pmr::monotonic_buffer_resource monotonic_res{&tracking_mem_resource};
      perform_deterministic_random_map_ops(&monotonic_res);
      std::cout << tracking_mem_resource.get_statistics() << '\n';
    }
    // unsynchronized_pool_resource will only free its memory once it goes out of scope.
    std::cout << "deallocating monotonic_buffer_resource\n";
    std::cout << tracking_mem_resource.get_statistics() << '\n';
  }

  return EXIT_SUCCESS;
}
