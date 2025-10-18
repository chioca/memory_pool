#include <array>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <vector>

#include "MemoryPool.h"

using namespace memory_pool;

// 计时器类

class Timer {
  using _clock = std::chrono::high_resolution_clock;
  _clock::time_point start;

 public:
  Timer() : start(_clock::now()) {}
  double elapsed() {
    auto end = _clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(end - start)
               .count() /
           1000.0;  // 转为毫秒
  }
};
class PerformanceTest {
 private:
  struct TestStats {
    double memPoolTime{0.0};
    double systemTime{0.0};
    size_t totalAllocs{0};
    size_t totalBytes{0};
  };

 public:
  // 系统预热
  static void warmUp() {
    std::cout << "Warming up memory systems..." << std::endl;
    std::vector<std::pair<void*, size_t>> warmupPtrs;

    for (size_t i = 0; i < 1000; i++) {
      for (size_t size = 8; size < 1025; size *= 2) {
        void* p = MemoryPool::allocate(size);
        warmupPtrs.push_back({p, size});
      }
      for (const auto& [ptr, size] : warmupPtrs) {
        MemoryPool::deallocate(ptr, size);
      }

      std::cout << "Warmup completed" << std::endl;
    }
  }

  // 小对象分配测试

  static void testSmallAllocation() {
    constexpr size_t NUM_ALLOCS = 50000;
    const size_t SIZES[] = {8, 16, 32, 64, 128, 256};
    const size_t NUM_SIZES = sizeof(SIZES) / sizeof(SIZES[0]);
    std::cout << "\nTesting small allocations (" << NUM_ALLOCS
              << " allocations of fixed sizes):" << std::endl;

    // 测试内存池
    {
      Timer T;
      std::array<std::vector<std::pair<void*, size_t>>, NUM_SIZES> sizePtrs;
      for (auto& ptrs : sizePtrs) {
        ptrs.reserve(NUM_ALLOCS / NUM_SIZES);
      }
      for (size_t i = 0; i < NUM_ALLOCS; i++) {
        size_t sizeIndex = i % NUM_SIZES;
        size_t size = SIZES[sizeIndex];
        void* ptr = MemoryPool::allocate(size);
        sizePtrs[sizeIndex].push_back({ptr, size});

        // 部分立即释放
        if (i % 4 == 0) {
          size_t releaseIndex = rand() % NUM_SIZES;
          auto& ptrs = sizePtrs[releaseIndex];
          if (!ptrs.empty()) {
            MemoryPool::deallocate(ptrs.back().first, ptrs.back().second);
            ptrs.pop_back();
          }
        }
      }
      for (auto& ptrs : sizePtrs) {
        for (auto& ptr : ptrs) {
          MemoryPool::deallocate(ptr.first, ptr.second);
        }
      }
      std::cout << "Memory Pool: " << std::fixed << std::setprecision(3)
                << T.elapsed() << " ms" << std::endl;
    }

    // 测试new 和 delete
    {
      Timer T;
      std::array<std::vector<std::pair<void*, size_t>>, NUM_SIZES> sizePtrs;
      for (auto& ptrs : sizePtrs) {
        ptrs.reserve(NUM_ALLOCS / NUM_SIZES);
      }
      for (size_t i = 0; i < NUM_ALLOCS; i++) {
        size_t sizeIndex = i % NUM_SIZES;
        size_t size = SIZES[sizeIndex];
        void* ptr = operator new(size);
        sizePtrs[sizeIndex].push_back({ptr, size});
        if (i % 4 == 0) {
          size_t releaseIndex = rand() % NUM_SIZES;
          auto& ptrs = sizePtrs[releaseIndex];

          if (!ptrs.empty()) {
            operator delete(ptrs.back().first);
            ptrs.pop_back();
          }
        }
      }
      // 清除剩余内存
      for (auto& ptrs : sizePtrs) {
        for (auto& ptr : ptrs) {
          operator delete(ptr.first);
        }
      }
      std::cout << "New/Delete: " << std::fixed << std::setprecision(3)
                << T.elapsed() << " ms" << std::endl;
    }
  }
};