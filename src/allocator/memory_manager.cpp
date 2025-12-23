#include "../../include/allocator.h"

using namespace std;

MemoryAllocator::MemoryAllocator()
    : memory(nullptr), memory_size(0), free_list_head(nullptr),
      next_block_id(1), allocator_name("Base") {}

MemoryAllocator::~MemoryAllocator() {
  if (memory) {
    delete[] memory;
  }

  vector<MemoryBlock *> freed_blocks;
  MemoryBlock *current = free_list_head;
  while (current) {
    freed_blocks.push_back(current);
    current = current->next;
  }

  for (auto block : allocated_blocks) {
    if (block && block->allocated) {
      delete block;
    }
  }

  for (auto block : freed_blocks) {
    delete block;
  }
}
