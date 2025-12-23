#include "../../include/allocator.h"
#include <algorithm>
#include <cstring>
#include <iomanip>
#include <iostream>

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

bool MemoryAllocator::init(size_t size) {
  if (memory) {
    cerr << "E[Allocator]: Already initialized" << endl;
    return false;
  }

  memory_size = size;
  memory = new char[size];
  memset(memory, 0, size);

  MemoryBlock *initial_block = new MemoryBlock(size, 0);
  free_list_head = initial_block;

  stats.total_memory = size;
  stats.free_memory = size;
  stats.num_free_blocks = 1;

  cout << "I[Allocator] Memory initialized: " << size << " bytes" << endl;
  return true;
}

MemoryBlock *MemoryAllocator::find_block(size_t block_id) {
  if (block_id == 0 || block_id > allocated_blocks.size()) {
    return nullptr;
  }
  return allocated_blocks[block_id - 1];
}

void MemoryAllocator::add_to_free_list(MemoryBlock *block) {
  block->allocated = false;

  if (!free_list_head || block->address < free_list_head->address) {
    block->next = free_list_head;
    block->prev = nullptr;
    if (free_list_head) {
      free_list_head->prev = block;
    }
    free_list_head = block;
  } else {
    MemoryBlock *current = free_list_head;
    while (current->next && current->next->address < block->address) {
      current = current->next;
    }

    block->next = current->next;
    block->prev = current;
    if (current->next) {
      current->next->prev = block;
    }
    current->next = block;
  }
}

void MemoryAllocator::remove_from_free_list(MemoryBlock *block) {
  if (block->prev) {
    block->prev->next = block->next;
  } else {
    free_list_head = block->next;
  }

  if (block->next) {
    block->next->prev = block->prev;
  }

  block->prev = nullptr;
  block->next = nullptr;
}

MemoryBlock *MemoryAllocator::coalesce(MemoryBlock *block) {
  MemoryBlock *result = block;

  if (block->next && block->address + block->size == block->next->address) {
    MemoryBlock *next = block->next;
    block->size += next->size;
    block->next = next->next;
    if (next->next) {
      next->next->prev = block;
    }
    delete next;
    stats.num_free_blocks--;
  }

  if (block->prev &&
      block->prev->address + block->prev->size == block->address) {
    MemoryBlock *prev = block->prev;
    prev->size += block->size;
    prev->next = block->next;
    if (block->next) {
      block->next->prev = prev;
    }
    result = prev;
    delete block;
    stats.num_free_blocks--;
  }

  return result;
}

bool MemoryAllocator::deallocate(size_t block_id) {
  MemoryBlock *block = find_block(block_id);

  if (!block) {
    cerr << "E[Deallocator] Invalid block ID " << block_id << endl;
    return false;
  }

  if (!block->allocated) {
    cerr << "E[Deallocator] Block " << block_id << " isfree" << endl;
    return false;
  }

  stats.used_memory -= block->size;
  stats.free_memory += block->size;
  stats.num_deallocations++;
  stats.num_allocated_blocks--;
  stats.num_free_blocks++;

  add_to_free_list(block);

  coalesce(block);

  cout << "I[Deallocator] Block " << block_id << " freed" << endl;

  update_stats();
  return true;
}
