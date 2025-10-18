#include <cassert>
#include <iostream>
#include <thread>

#include "MemoryPool.h"
using namespace memory_pool;
void testBasicAllocation() {
  std::cout << "Running basic allocation test..." << std::endl;

  void* ptr1 = MemoryPool::allocate(8);
  assert(ptr1 != nullptr);
  MemoryPool::deallocate(ptr1, 8);

  void* ptr2 = MemoryPool::allocate(1024);
  assert(ptr2 != nullptr);
  MemoryPool::deallocate(ptr2, 1024);

  void* ptr3 = MemoryPool::allocate(1024 * 1024);
  assert(ptr3 != nullptr);
  MemoryPool::deallocate(ptr3, 1024 * 1024);

  std::cout << "Basic allocation test passed!" << std::endl;
}

int main() { testBasicAllocation(); }