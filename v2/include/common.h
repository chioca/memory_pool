#pragma once
#include <array>
#include <atomic>
#include <cstddef>

namespace memory_pool {
constexpr size_t ALIGNMENT = 8;           // 对齐数
constexpr size_t MAX_BYTES = 256 * 1024;  // 256KB
constexpr size_t FREE_LIST_SIZE = MAX_BYTES / ALIGNMENT;

struct BlockHeader {
  size_t size;        // 内存块大小
  bool isUse;         // 使用标志
  BlockHeader *next;  // 指向下一个内存块
};

class SizeClass {
 private:
  /* data */
 public:
  static size_t roundUp(size_t bytes) {
    // 内存对齐: 先向上取整,再将最低3位置0
    return (bytes + ALIGNMENT - 1) & ~(ALIGNMENT - 1);
  }
  static size_t getIndex(size_t bytes) {
    bytes = roundUp(bytes);
    return (bytes + ALIGNMENT - 1) / ALIGNMENT - 1;
  }
};

}  // namespace memory_pool
