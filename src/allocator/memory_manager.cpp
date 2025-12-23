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

void MemoryAllocator::update_stats() { calculate_fragmentation(); }

void MemoryAllocator::calculate_fragmentation() {
  size_t largest_free_block = 0;
  MemoryBlock *current = free_list_head;

  while (current) {
    if (current->size > largest_free_block) {
      largest_free_block = current->size;
    }
    current = current->next;
  }

  if (stats.free_memory > 0) {
    stats.external_fragmentation =
        (1.0 - (double)largest_free_block / stats.free_memory) * 100.0;
  } else {
    stats.external_fragmentation = 0.0;
  }

  stats.internal_fragmentation = 0.0;
}

AllocationStats MemoryAllocator::get_stats() {
  update_stats();
  return stats;
}

void MemoryAllocator::dump_memory() {
  cout << "\n~~~~~Memory Dump~~~~~~" << endl;

  vector<MemoryBlock *> all_blocks;

  MemoryBlock *free_block = free_list_head;
  while (free_block) {
    all_blocks.push_back(free_block);
    free_block = free_block->next;
  }

  for (size_t i = 0; i < allocated_blocks.size(); i++) {
    if (allocated_blocks[i] && allocated_blocks[i]->allocated) {
      all_blocks.push_back(allocated_blocks[i]);
    }
  }

  sort(all_blocks.begin(), all_blocks.end(),
       [](MemoryBlock *a, MemoryBlock *b) { return a->address < b->address; });

  for (auto block : all_blocks) {
    cout << "[0x" << hex << setw(4) << setfill('0') << block->address << " - 0x"
         << setw(4) << setfill('0') << (block->address + block->size - 1)
         << "] ";

    if (block->allocated) {
      size_t block_id = 0;
      for (size_t i = 0; i < allocated_blocks.size(); i++) {
        if (allocated_blocks[i] == block) {
          block_id = i + 1;
          break;
        }
      }
      cout << "I[Allocator] USED (id=" << dec << block_id
           << ", size=" << block->size << ")";
    } else {
      cout << "I[Allocator] FREE (size=" << dec << block->size << ")";
    }
    cout << endl;
  }

  cout << dec << endl;
}
