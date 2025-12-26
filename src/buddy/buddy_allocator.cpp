#include "../../include/buddy.h"

BuddyAllocator::BuddyAllocator()
    : memory(nullptr), memory_size(0), min_block_size(32), max_block_size(0),
      next_block_id(1), total_allocations(0), total_deallocations(0),
      allocation_failures(0), internal_fragmentation_bytes(0) {}

BuddyAllocator::~BuddyAllocator() {
  if (memory) {
    delete[] memory;
  }

  for (auto &pair : free_lists) {
    BuddyBlock *current = pair.second;
    while (current) {
      BuddyBlock *next = current->next;
      delete current;
      current = next;
    }
  }

  for (auto &pair : allocated_blocks) {
    delete pair.second;
  }
}
// TODO implement alloc and other methods :(
