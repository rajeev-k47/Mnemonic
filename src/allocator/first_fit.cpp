#include "../../include/allocator.h"
#include <iomanip>
#include <iostream>

using namespace std;
FirstFitAllocator::FirstFitAllocator() { allocator_name = "First Fit"; }

MemoryBlock *FirstFitAllocator::find_free_block(size_t size) {
  MemoryBlock *current = free_list_head;

  while (current) {
    if (current->size >= size) {
      return current;
    }
    current = current->next;
  }

  return nullptr;
}

AllocationResult FirstFitAllocator::allocate(size_t size) {
  if (size == 0) {
    return AllocationResult(false, 0, 0, "Invalid alloc size");
  }

  MemoryBlock *free_block = find_free_block(size);

  if (!free_block) {
    stats.allocation_failures++;
    return AllocationResult(false, 0, 0, "No suitable bl");
  }

  MemoryBlock *allocated_block;

  if (free_block->size == size) {
    allocated_block = free_block;
    remove_from_free_list(free_block);
    stats.num_free_blocks--;
  } else {
    allocated_block = new MemoryBlock(size, free_block->address);
    free_block->size -= size;
    free_block->address += size;
  }

  allocated_block->allocated = true;

  size_t block_id = next_block_id++;
  allocated_blocks.push_back(allocated_block);

  stats.used_memory += size;
  stats.free_memory -= size;
  stats.num_allocations++;
  stats.num_allocated_blocks++;

  cout << "I[Allocator] Allocated block id=" << block_id << " at address=0x"
       << hex << setw(4) << setfill('0') << allocated_block->address << dec
       << " (size=" << size << ")" << endl;

  return AllocationResult(true, block_id, allocated_block->address, "Success");
}
