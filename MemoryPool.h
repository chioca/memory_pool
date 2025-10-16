#include <cstddef>
#include <mutex>
namespace memoryPool {
#define MEMORY_POOL_NUM 64
#define SLOT_BASE_SIZE 8
#define MAX_SLOT_SIZE 512
struct Slot {
  Slot* next;
};

class MemoryPool {
 private:
  void allocateNewBlock();
  size_t padPointer(char* p, size_t align);

 public:
  MemoryPool(size_t BlockSize = 4096);
  ~MemoryPool();
  void init(size_t);
  void* allocate();
  void deallocate(void*);

 private:
  size_t BlockSize_;
  size_t SlotSize_;
  Slot* firstBlock_;
  Slot* curSlot_;
  Slot* freeList_;
  Slot* lastSlot_;
  std::mutex mutexForFreeList_;  // 确保多线程下freeList操作的原子性
  std::mutex mutexForBlock_;     // 避免多线程下重复开辟内存
};
class HashBucket {
 private:
  /* data */
 public:
  HashBucket() = default;
  ~HashBucket() = default;
  static void initMemoryPool();
  static MemoryPool& getMemortPool(size_t index);
  static void* useMemory(size_t size) {
    if (size <= 0) return nullptr;
    if (size > MAX_SLOT_SIZE) return operator new(size);
    return getMemortPool((size + 7) / SLOT_BASE_SIZE - 1).allocate();
  }
  static void freeMemory(void* ptr, size_t size) {
    if (!ptr) return;
    if (size > MAX_SLOT_SIZE) {
      operator delete(ptr);
      return;
    }
    getMemortPool((size + 7) / SLOT_BASE_SIZE - 1).deallocate(ptr);
  }

  template <typename T, typename... Args>
  friend T* newElement(Args&&... args);

  template <typename T>
  friend void delElement(T* p);
};

template <typename T, typename... Args>
T* newElement(Args&&... args) {
  T* p = nullptr;
  if ((p = reinterpret_cast<T*>(HashBucket::useMemory(sizeof(T)))) != nullptr) {
    new (p) T(std::forward<Args>(args)...);
  }
  return p;
}
template <typename T>
void delElement(T* p) {
  if (p) {
    p->~T();
    HashBucket::freeMemory(reinterpret_cast<void*>(p), sizeof(T));
  }
}
}  // namespace memoryPool
