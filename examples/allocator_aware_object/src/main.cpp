#include <array>
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <memory_resource>
#include <random>
#include <ranges>
#include <span>
#include <string_view>

namespace
{
  void print(const std::string_view prefix, const std::span<const std::byte> data)
  {
    std::cout << prefix << ":";
    std::ranges::copy(data                                                        //
                        | std::views::transform(&std::to_integer<unsigned char>)  //
                        | std::views::transform([](const auto c) -> char { return std::isprint(c) ? c : '#'; }),
                      std::ostream_iterator<unsigned char>(std::cout));
    std::cout << std::endl;
  }

  struct product
  {
    std::pmr::string name;
  };

  struct product_pmr_alloc_aware
  {
    using allocator_type = std::pmr::polymorphic_allocator<>;

    constexpr product_pmr_alloc_aware(const allocator_type& alloc = {}) : name(alloc) {}
    explicit constexpr product_pmr_alloc_aware(const std::string_view s, const allocator_type& alloc = {})
      : name(s, alloc)
    {}
    explicit constexpr product_pmr_alloc_aware(const product_pmr_alloc_aware& other, const allocator_type& alloc = {})
      : name(other.name, alloc)
    {}
    explicit constexpr product_pmr_alloc_aware(product_pmr_alloc_aware&& other, const allocator_type& alloc = {})
      : name(std::move(other.name), alloc){};

    std::pmr::string name;
  };

  template<typename T>
  void run()
  {
    std::array<std::byte, 128> buffer;
    std::ranges::fill(buffer, std::byte{'_'});
    std::pmr::monotonic_buffer_resource res(buffer.data(), buffer.size(), std::pmr::null_memory_resource());
    print("#0", buffer);

    std::pmr::vector<T> objects(&res);
    objects.emplace_back(T{"foo bar baz qux lorem ipsum dolor"});
    print("#1", buffer);
  }
}  // namespace

int main()
{
  run<product>();
  // prints
  // #0________________________________________________________________________________________________________________________________
  // #1#P8#####0###############!#######/#######________________________________________________________________________________________
  // the storage of the string is not contained in the stack buffer

  run<product_pmr_alloc_aware>();
  // prints
  // #0________________________________________________________________________________________________________________________________
  // #1p###!#######!###########!#######/#######foo bar baz qux lorem ipsum
  // dolor#______________________________________________________
  //                                           ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
  // the storage of the string _is_ contained in the stack buffer

  return EXIT_SUCCESS;
}
