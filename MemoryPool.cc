#include "MemoryPool.h"

#include <assert.h>
namespace memoryPool {
MemoryPool::MemoryPool(size_t BlockSize) : BlockSize_(BlockSize) {}
MemoryPool::~MemoryPool() {
  Slot *cur = firstBlock_;
  while (cur) {
    Slot *nex = cur->next;
    operator delete(reinterpret_cast<void *>(cur));
    cur = nex;
  }
}

void MemoryPool::init(size_t size) {
  assert(size > 0);
  SlotSize_ = size;
  firstBlock_ = nullptr;
  curSlot_ = nullptr;
  freeList_ = nullptr;
  lastSlot_ = nullptr;
}

void *MemoryPool::allocate() {
  // 优先使用空闲链表中的内存槽
  if (freeList_ != nullptr) {
    {
      std::lock_guard<std::mutex> lock(mutexForFreeList_);
      if (freeList_ != nullptr) {
        Slot *temp = freeList_;
        freeList_ = freeList_->next;
        return temp;
      }
    }
  }
  Slot *temp;
  {
    std::lock_guard<std::mutex> lock(mutexForBlock_);
    if (curSlot_ >= lastSlot_) {
      allocateNewBlock();
    }
    temp = curSlot_;
    curSlot_ += SlotSize_ / sizeof(Slot);
    return temp;
  }
}

void MemoryPool::deallocate(void *ptr) {
  if (ptr) {
    std::lock_guard<std::mutex> lock(mutexForFreeList_);
    reinterpret_cast<Slot *>(ptr)->next = freeList_;
    freeList_ = reinterpret_cast<Slot *>(ptr);
  }
}

void MemoryPool::allocateNewBlock() {
  void *newBlock = operator new(BlockSize_);
  reinterpret_cast<Slot *>(newBlock)->next = firstBlock_;
  firstBlock_ = reinterpret_cast<Slot *>(newBlock);

  char *body = reinterpret_cast<char *>(newBlock) + sizeof(Slot);
  size_t paddingSize = padPointer(body, SlotSize_);
  curSlot_ = reinterpret_cast<Slot *>(body + paddingSize);

  lastSlot_ = reinterpret_cast<Slot *>(reinterpret_cast<size_t>(newBlock) +
                                       BlockSize_ - SlotSize_ + 1);
  freeList_ = nullptr;
}

size_t MemoryPool::padPointer(char *p, size_t align) {
  return (align - reinterpret_cast<size_t>(p)) % align;
}

void HashBucket::initMemoryPool() {
  for (size_t i = 0; i < MEMORY_POOL_NUM; i++) {
    getMemortPool(i).init((i + 1) * SLOT_BASE_SIZE);
  }
}
MemoryPool &HashBucket::getMemortPool(size_t index) {
  static MemoryPool memoryPool[MEMORY_POOL_NUM];
  return memoryPool[index];
}

}  // namespace memoryPool