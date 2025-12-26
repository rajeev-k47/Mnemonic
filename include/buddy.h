#ifndef BUDDY_H
#define BUDDY_H

#include <cstddef>
#include <map>
#include <string>
#include <vector>

using namespace std;

struct AllocationStats;

struct BuddyBlock {
  size_t address;
  size_t size;
  bool allocated;
  BuddyBlock *next;

  BuddyBlock(size_t addr, size_t sz)
      : address(addr), size(sz), allocated(false), next(nullptr) {}
};

struct BuddyAllocationResult {
  bool success;
  size_t block_id;
  size_t address;
  size_t actual_size;
  string message;

  BuddyAllocationResult(bool s = false, size_t bid = 0, size_t addr = 0,
                        size_t actual = 0, const string &msg = "")
      : success(s), block_id(bid), address(addr), actual_size(actual),
        message(msg) {}
};

class BuddyAllocator {
private:
  char *memory;
  size_t memory_size;
  size_t min_block_size;
  size_t max_block_size;

  map<size_t, BuddyBlock *> free_lists;

  map<size_t, BuddyBlock *> allocated_blocks;
  size_t next_block_id;

  size_t total_allocations;
  size_t total_deallocations;
  size_t allocation_failures;
  size_t internal_fragmentation_bytes;

  size_t round_up_to_power_of_two(size_t size);
  size_t log2_size(size_t size);
  size_t get_buddy_address(size_t address, size_t size);
  BuddyBlock *find_buddy(size_t address, size_t size);
  void split_block(BuddyBlock *block, size_t target_size);
  BuddyBlock *coalesce(BuddyBlock *block);
  void add_to_free_list(BuddyBlock *block);
  void remove_from_free_list(BuddyBlock *block);
  BuddyBlock *allocate_from_free_list(size_t size);
  bool is_power_of_two(size_t n);

public:
  BuddyAllocator();
  ~BuddyAllocator();

  bool init(size_t size, size_t min_size = 32);

  BuddyAllocationResult allocate(size_t size);
  bool deallocate(size_t block_id);

  void dump_memory();
  void dump_free_lists();
  AllocationStats get_stats();
  string get_allocator_name() const { return "Buddy System"; }
};

#endif
