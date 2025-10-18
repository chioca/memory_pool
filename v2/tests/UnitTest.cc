#include <cassert>
#include <iostream>
#include <thread>

#include "MemoryPool.h"
using namespace memory_pool;

// 基础分配测试
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

// 内存写入测试

void testMemoryWriting() {
  std::cout << "Running memory writing test..." << std::endl;
  const size_t size = 128;
  char* ptr = static_cast<char*>(MemoryPool::allocate(size));
  assert(ptr != nullptr);
  // 写入数据
  for (size_t i = 0; i < size; ++i) {
    ptr[i] = static_cast<char>(i % 256);
  }

  // 验证数据
  for (size_t i = 0; i < size; ++i) {
    assert(ptr[i] == static_cast<char>(i % 256));
  }

  MemoryPool::deallocate(ptr, size);
  std::cout << "Memory writing test passed!" << std::endl;
}
int main() { testBasicAllocation(); }