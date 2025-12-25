#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <cstddef>
#include <string>
#include <vector>

using namespace std;

struct MemoryBlock {
  size_t size;
  bool allocated;
  size_t address;
  MemoryBlock *prev;
  MemoryBlock *next;

  MemoryBlock(size_t sz, size_t addr)
      : size(sz), allocated(false), address(addr), prev(nullptr),
        next(nullptr) {}
};

struct AllocationStats {
  size_t total_memory;
  size_t used_memory;
  size_t free_memory;
  size_t num_allocations;
  size_t num_deallocations;
  size_t allocation_failures;
  size_t num_free_blocks;
  size_t num_allocated_blocks;
  double external_fragmentation;
  double internal_fragmentation;

  AllocationStats()
      : total_memory(0), used_memory(0), free_memory(0), num_allocations(0),
        num_deallocations(0), allocation_failures(0), num_free_blocks(0),
        num_allocated_blocks(0), external_fragmentation(0.0),
        internal_fragmentation(0.0) {}
};

struct AllocationResult {
  bool success;
  size_t block_id;
  size_t address;
  string message;

  AllocationResult(bool s = false, size_t bid = 0, size_t addr = 0,
                   const string &msg = "")
      : success(s), block_id(bid), address(addr), message(msg) {}
};

class MemoryAllocator {
protected:
  char *memory;
  size_t memory_size;
  MemoryBlock *free_list_head;
  vector<MemoryBlock *> allocated_blocks;
  size_t next_block_id;
  AllocationStats stats;

  MemoryBlock *find_block(size_t block_id);
  void add_to_free_list(MemoryBlock *block);
  void remove_from_free_list(MemoryBlock *block);
  MemoryBlock *coalesce(MemoryBlock *block);
  void update_stats();
  void calculate_fragmentation();

public:
  MemoryAllocator();
  virtual ~MemoryAllocator();

  bool init(size_t size);
  virtual AllocationResult allocate(size_t size) = 0;
  bool deallocate(size_t block_id);

  AllocationStats get_stats();
  void dump_memory();
  string get_allocator_name() const { return allocator_name; }

protected:
  string allocator_name;
  virtual MemoryBlock *find_free_block(size_t size) = 0;
};

class FirstFitAllocator : public MemoryAllocator {
public:
  FirstFitAllocator();
  AllocationResult allocate(size_t size) override;

protected:
  MemoryBlock *find_free_block(size_t size) override;
};

class BestFitAllocator : public MemoryAllocator {
public:
  BestFitAllocator();
  AllocationResult allocate(size_t size) override;

protected:
  MemoryBlock *find_free_block(size_t size) override;
};

class WorstFitAllocator : public MemoryAllocator {
public:
  WorstFitAllocator();
  AllocationResult allocate(size_t size) override;

protected:
  MemoryBlock *find_free_block(size_t size) override;
};

#endif
