#include "util.hh"

std::pmr::memory_resource *pool;

std::pmr::memory_resource *getMemoryResource()
{
  // TODO: this is just for the initial testing. A singelting like construct makes it easy
  //  to replace existing code with allocator aware code.
  //  The downside is: we have no meaningful way to clear the buffer for now. On the other hand
  //  i don't think we have to at all. The buffer has the lifetime of the program execution
  static std::pmr::memory_resource *pool = nullptr;
  if (pool == nullptr) {
    auto bufferPool = new std::pmr::monotonic_buffer_resource(std::pmr::new_delete_resource());
    pool = new std::pmr::unsynchronized_pool_resource(bufferPool);
  }

  return pool;
}