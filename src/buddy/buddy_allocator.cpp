#include "../../include/allocator.h"
#include "../../include/buddy.h"

#include <algorithm>
#include <cstring>
#include <iomanip>
#include <iostream>
using namespace std;

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

bool BuddyAllocator::is_power_of_two(size_t n) {
  return n > 0 && (n & (n - 1)) == 0;
}

size_t BuddyAllocator::round_up_to_power_of_two(size_t size) {
  if (size == 0)
    return 0;

  size_t power = 1;
  while (power < size) {
    power *= 2;
  }
  return power;
}

size_t BuddyAllocator::log2_size(size_t size) {
  size_t log = 0;
  while (size >>= 1) {
    log++;
  }
  return log;
}

size_t BuddyAllocator::get_buddy_address(size_t address, size_t size) {
  return address ^ size; // XOR
}

bool BuddyAllocator::init(size_t size, size_t min_size) {
  if (memory) {
    cerr << "E[Buddy] Already initialized" << endl;
    return false;
  }

  if (!is_power_of_two(size)) {
    size = round_up_to_power_of_two(size);
    cout << "E[Buddy] Rounded to power of 2: " << size << endl;
  }

  if (!is_power_of_two(min_size)) {
    min_size = round_up_to_power_of_two(min_size);
  }

  memory_size = size;
  min_block_size = min_size;
  max_block_size = size;

  memory = new char[size];
  memset(memory, 0, size);

  BuddyBlock *initial_block = new BuddyBlock(0, size);
  free_lists[log2_size(size)] = initial_block;

  cout << "I[Buddy] Initialized: " << size << " bytes" << endl;
  cout << "I[Buddy] Min block size: " << min_block_size << " bytes" << endl;
  return true;
}

void BuddyAllocator::add_to_free_list(BuddyBlock *block) {
  size_t log_size = log2_size(block->size);
  block->next = free_lists[log_size];
  free_lists[log_size] = block;
}

void BuddyAllocator::remove_from_free_list(BuddyBlock *block) {
  size_t log_size = log2_size(block->size);
  BuddyBlock **current = &free_lists[log_size];

  while (*current) {
    if (*current == block) {
      *current = block->next;
      block->next = nullptr;
      return;
    }
    current = &((*current)->next);
  }
}

BuddyBlock *BuddyAllocator::find_buddy(size_t address, size_t size) {
  size_t buddy_addr = get_buddy_address(address, size);
  size_t log_size = log2_size(size);

  BuddyBlock *current = free_lists[log_size];
  while (current) {
    if (current->address == buddy_addr) {
      return current;
    }
    current = current->next;
  }

  return nullptr;
}

void BuddyAllocator::split_block(BuddyBlock *block, size_t target_size) {
  while (block->size > target_size) {
    size_t new_size = block->size / 2;
    remove_from_free_list(block);
    BuddyBlock *buddy = new BuddyBlock(block->address + new_size, new_size);
    block->size = new_size;
    add_to_free_list(block);
    add_to_free_list(buddy);
  }
}

BuddyBlock *BuddyAllocator::coalesce(BuddyBlock *block) {
  while (block->size < max_block_size) {
    BuddyBlock *buddy = find_buddy(block->address, block->size);

    if (!buddy || buddy->allocated) {
      break; // merge_error
    }
    remove_from_free_list(block);
    remove_from_free_list(buddy);

    BuddyBlock *merged;
    if (block->address < buddy->address) {
      merged = block;
      delete buddy;
    } else {
      merged = buddy;
      delete block;
      block = merged;
    }

    merged->size *= 2;
    add_to_free_list(merged);
  }

  return block;
}

BuddyBlock *BuddyAllocator::allocate_from_free_list(size_t size) {
  size_t log_size = log2_size(size);

  if (free_lists[log_size]) {
    BuddyBlock *block = free_lists[log_size];
    remove_from_free_list(block);
    return block;
  }

  for (size_t log = log_size + 1; log <= log2_size(max_block_size); log++) {
    if (free_lists[log]) {
      BuddyBlock *block = free_lists[log];
      split_block(block, size);
      block = free_lists[log_size];
      remove_from_free_list(block);
      return block;
    }
  }

  return nullptr;
}

BuddyAllocationResult BuddyAllocator::allocate(size_t size) {
  if (size == 0) {
    return BuddyAllocationResult(false, 0, 0, 0, "Invalid alloc size");
  }
  size_t actual_size = round_up_to_power_of_two(size);
  if (actual_size < min_block_size) {
    actual_size = min_block_size;
  }
  BuddyBlock *block = allocate_from_free_list(actual_size);

  if (!block) {
    allocation_failures++;
    return BuddyAllocationResult(false, 0, 0, 0, "Out of memory");
  }

  block->allocated = true;
  size_t block_id = next_block_id++;
  allocated_blocks[block_id] = block;
  total_allocations++;
  internal_fragmentation_bytes += (actual_size - size);

  cout << "I[Buddy] Allocated block id=" << block_id << " at address=0x" << hex
       << setw(4) << setfill('0') << block->address << dec
       << " (requested=" << size << ", actual=" << actual_size << ")" << endl;

  return BuddyAllocationResult(true, block_id, block->address, actual_size,
                               "Success");
}

bool BuddyAllocator::deallocate(size_t block_id) {
  auto it = allocated_blocks.find(block_id);

  if (it == allocated_blocks.end()) {
    cerr << "E[Buddy] Invalid block ID " << block_id << endl;
    return false;
  }

  BuddyBlock *block = it->second;

  if (!block->allocated) {
    cerr << "E[Buddy] Block " << block_id << " is already free" << endl;
    return false;
  }

  block->allocated = false;
  allocated_blocks.erase(it);
  add_to_free_list(block);
  coalesce(block);

  total_deallocations++;

  cout << "I[Buddy] Block" << block_id << "freed" << endl;

  return true;
}

void BuddyAllocator::dump_memory() {
  cout << "\n~~~~~~~Buddy Memory Dump~~~~~~~~" << endl;

  vector<BuddyBlock *> all_blocks;
  for (auto &pair : free_lists) {
    BuddyBlock *current = pair.second;
    while (current) {
      all_blocks.push_back(current);
      current = current->next;
    }
  }

  for (auto &pair : allocated_blocks) {
    all_blocks.push_back(pair.second);
  }

  sort(all_blocks.begin(), all_blocks.end(),
       [](BuddyBlock *a, BuddyBlock *b) { return a->address < b->address; });

  for (auto block : all_blocks) {
    cout << "[0x" << hex << setw(4) << setfill('0') << block->address << " - 0x"
         << setw(4) << setfill('0') << (block->address + block->size - 1)
         << "] ";

    if (block->allocated) {
      size_t block_id = 0;
      for (auto &pair : allocated_blocks) {
        if (pair.second == block) {
          block_id = pair.first;
          break;
        }
      }
      cout << "I[Buddy] USED (id=" << dec << block_id
           << ", size=" << block->size << ")";
    } else {
      cout << "I[Buddy] FREE (size=" << dec << block->size << ")";
    }
    cout << endl;
  }

  cout << dec << endl;
}

void BuddyAllocator::dump_free_lists() {
  cout << "\n~~~~~~~Buddy Free Lists~~~~~~~" << endl;

  for (auto &pair : free_lists) {
    size_t block_size = (1ULL << pair.first);
    BuddyBlock *current = pair.second;

    if (current) {
      cout << "Size " << block_size << " bytes: ";
      while (current) {
        cout << "0x" << hex << setw(4) << setfill('0') << current->address
             << dec;
        if (current->next)
          cout << " -> ";
        current = current->next;
      }
      cout << endl;
    }
  }

  cout << endl;
}

AllocationStats BuddyAllocator::get_stats() {
  AllocationStats stats;

  stats.total_memory = memory_size;

  size_t used = 0;
  for (auto &pair : allocated_blocks) {
    used += pair.second->size;
  }

  stats.used_memory = used;
  stats.free_memory = memory_size - used;
  stats.num_allocations = total_allocations;
  stats.num_deallocations = total_deallocations;
  stats.allocation_failures = allocation_failures;
  stats.num_allocated_blocks = allocated_blocks.size();

  size_t free_block_count = 0;
  for (auto &pair : free_lists) {
    BuddyBlock *current = pair.second;
    while (current) {
      free_block_count++;
      current = current->next;
    }
  }
  stats.num_free_blocks = free_block_count;

  if (used > 0) {
    stats.internal_fragmentation =
        (double)internal_fragmentation_bytes / used * 100.0;
  } else {
    stats.internal_fragmentation = 0.0;
  }

  size_t largest_free = 0;
  for (auto &pair : free_lists) {
    BuddyBlock *current = pair.second;
    while (current) {
      if (current->size > largest_free) {
        largest_free = current->size;
      }
      current = current->next;
    }
  }

  if (stats.free_memory > 0) {
    stats.external_fragmentation =
        (1.0 - (double)largest_free / stats.free_memory) * 100.0;
  } else {
    stats.external_fragmentation = 0.0;
  }

  return stats;
}
