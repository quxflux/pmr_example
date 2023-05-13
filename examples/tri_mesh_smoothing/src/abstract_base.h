#pragma once

namespace quxflux
{
  struct abstract_base
  {
    abstract_base() = default;
    abstract_base(const abstract_base&) = default;
    abstract_base(abstract_base&&) = default;
    abstract_base& operator=(const abstract_base&) = default;
    abstract_base& operator=(abstract_base&&) = default;
    virtual ~abstract_base() = default;
  };
}  // namespace quxflux
