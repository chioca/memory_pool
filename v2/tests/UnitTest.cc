#include <cassert>
#include <iostream>
#include <thread>
#include <vector>

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

// 多线程测试
void testMultiThreading() {
  std::cout << "Running multi-threading test..." << std::endl;

  const int NUM_THREADS = 4;
  const int ALLOCS_PER_THREAD = 1000;
  std::atomic<bool> has_error{false};

  auto threadFunc = [&has_error]() {
    try {
      std::vector<std::pair<void*, size_t>> allocations;
      allocations.reserve(ALLOCS_PER_THREAD);

      for (size_t i = 0; i < ALLOCS_PER_THREAD && !has_error; i++) {
        size_t size = (rand() % 256 + 1) * 8;
        void* ptr = MemoryPool::allocate(size);
        if (!ptr) {
          std::cerr << "Allocation failed for size: " << size << std::endl;
          has_error = true;
          break;
        }
        allocations.push_back({ptr, size});
        if (rand() % 2 && !allocations.empty()) {
          size_t index = rand() % allocations.size();
          MemoryPool::deallocate(allocations[index].first,
                                 allocations[index].second);
          allocations.erase(allocations.begin() + index);
        }
      }
      for (const auto& alloc : allocations) {
        MemoryPool::deallocate(alloc.first, alloc.second);
      }

    } catch (const std::exception& e) {
      std::cerr << " Thread exception:" << e.what() << '\n';
      has_error = true;
    }
  };
  std::vector<std::thread> threads;
  for (size_t i = 0; i < NUM_THREADS; i++) {
    threads.emplace_back(threadFunc);
  }
  for (auto& thread : threads) {
    thread.join();
  }

  std::cout << "Multi-threading test passed!" << std::endl;
}

int main() { testBasicAllocation(); }