#include "../include/MemoryPool.h"

#include <assert.h>
namespace memoryPool {
MemoryPool::MemoryPool(size_t BlockSize)
    : BlockSize_(BlockSize),
      SlotSize_(0),
      firstBlock_(nullptr),
      curSlot_(nullptr),
      freeList_(nullptr),
      lastSlot_(nullptr) {}
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
  freeList_.store(nullptr, std::memory_order_relaxed);
  lastSlot_ = nullptr;
}

bool MemoryPool::pushFreeList(Slot *slot) {
  while (true) {
    Slot *OldHead = freeList_.load(std::memory_order_relaxed);
    slot->next.store(OldHead, std::memory_order_relaxed);
    if (freeList_.compare_exchange_weak(OldHead, slot,
                                        std::memory_order_release,
                                        std::memory_order_relaxed)) {
      return true;
    }
  }
}

Slot *MemoryPool::popFreeList() {
  while (true) {
    Slot *OldHead = freeList_.load(std::memory_order_relaxed);
    if (OldHead == nullptr) {
      return nullptr;
    }
    Slot *newHead = OldHead->next.load(std::memory_order_relaxed);
    if (freeList_.compare_exchange_weak(OldHead, newHead,
                                        std::memory_order_acquire,
                                        std::memory_order_relaxed)) {
      return OldHead;
    }
  }
}

void *MemoryPool::allocate() {
  // 优先使用空闲链表中的内存槽
  Slot *slot = popFreeList();
  if (slot != nullptr) return slot;

  // 若空闲链表为空 分配新的内存
  std::lock_guard<std::mutex> lock(mutexForBlock_);
  if (curSlot_ >= lastSlot_) {
    allocateNewBlock();
  }
  Slot *result = curSlot_;
  curSlot_ =
      reinterpret_cast<Slot *>(reinterpret_cast<char *>(curSlot_) + SlotSize_);
  return result;
}

void MemoryPool::deallocate(void *ptr) {
  if (!ptr) return;
  Slot *slot = static_cast<Slot *>(ptr);
  pushFreeList(slot);
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