#include "../../include/allocator.h"
#include <iomanip>
#include <iostream>

WorstFitAllocator::WorstFitAllocator() { allocator_name = "Worst Fit"; }

MemoryBlock *WorstFitAllocator::find_free_block(size_t size) {
  MemoryBlock *current = free_list_head;
  MemoryBlock *worst_fit = nullptr;
  size_t largest_size = 0;

  while (current) {
    if (current->size >= size && current->size > largest_size) {
      largest_size = current->size;
      worst_fit = current;
    }
    current = current->next;
  }

  return worst_fit;
}

AllocationResult WorstFitAllocator::allocate(size_t size) {
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

  std::cout << "I[Allocator] Allocated block id=" << block_id
            << " at address=0x" << std::hex << std::setw(4) << std::setfill('0')
            << allocated_block->address << std::dec << " (size=" << size << ")"
            << std::endl;

  return AllocationResult(true, block_id, allocated_block->address, "Success");
}
