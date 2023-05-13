# pmr example

This repo contains examples demonstrating the usage of [polymorphic memory resources / allocators](https://en.cppreference.com/w/cpp/header/memory_resource).

## notes
Implemented using Visual Studio 2022 17.5.4. The code in this repo may not neccessarily be compilable using other compilers (e.g. gcc).

## contained projects

### mem_resource_chaining
This project demonstrates how `std::pmr::memory_resources` may be chained and how this may influence the number of heap allocations.

### tri_mesh_smoothing
A project with a real world example to demonstrate the performance impact of using a `std::pmr::vector` with `std::pmr::monotonic_buffer_resource` instead of a `std::vector` in a hot loop.

The task in the example is to smooth a [triangle mesh](https://en.wikipedia.org/wiki/Triangle_mesh) using [Laplacian smoothing](https://en.wikipedia.org/wiki/Laplacian_smoothing). The triangle mesh implementation is private, the smoothing has to be performed using a given pure virtual interface. 

The design of the interface makes it neccessary to copy indices of neighboring vertices (which is an essential operation for this algorithm) en block. Unfortunately the required buffer size can not be determined at compile time but it can be proven that for well-behaved (i.e. closed manifold) triangle meshes the vertex valence is 6 which gives a good estimate to use with `std::pmr::monotonic_buffer_resource`.

## Acknowledgements
* [Jason Turners C++ Starter Project](https://github.com/cpp-best-practices/cpp_starter_project)
* [C++ Stories blog entry](https://www.cppstories.com/2020/08/pmr-dbg.html/) regarding `std::pmr`
