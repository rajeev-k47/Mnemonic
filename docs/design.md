# Design of Mnemonic

## 1. Overview

This document describes the design and implementation of a memory management simulator that models OS-level memory management behavior. The simulator implements dynamic memory allocation strategies, tracks fragmentation, and provides visualization and statistical analysis capabilities.

## 2. System Architecture

### 2.1 High-Level Architecture

```
┌─────────────────────────────────────────────────────────┐
│                    CLI Interface                        │
│              (Command Parser & Handlers)                │
└─────────────────────────────────────────────────────────┘
                           │
        ┌──────────────────┼──────────────────┐
        │                  │                  │
        ▼                  ▼                  ▼
┌──────────────┐  ┌──────────────┐  ┌──────────────┐
│   Memory     │  │    Cache     │  │   Virtual    │
│  Allocator   │  │  Hierarchy   │  │   Memory     │
│              │  │              │  │   Manager    │
│ - First Fit  │  │ - L1 Cache   │  │ - Page Table │
│ - Best Fit   │  │ - L2 Cache   │  │ - TLB (opt)  │
│ - Worst Fit  │  │ - FIFO/LRU   │  │ - FIFO/LRU   │
└──────────────┘  └──────────────┘  └──────────────┘
        │                  │                  │
        └──────────────────┼──────────────────┘
                           ▼
                ┌──────────────────┐
                │ Physical Memory  │
                │   Simulation     │
                └──────────────────┘
```

### 2.2 Component Breakdown

#### 2.2.1 CLI Interface
- **Responsibility**: User interaction and command processing
- **Components**:
  - `CommandParser`: Tokenizes and parses user input
  - `CLI`: Main command dispatcher and state manager
- **Commands Supported**:
  - `init memory <size>`: Initialize physical memory
  - `set allocator <type>`: Select allocation strategy
  - `malloc <size>`: Allocate memory block
  - `free <block_id>`: Deallocate memory block
  - `dump`: Display memory layout
  - `stats`: Show allocation statistics
  - `help`: Display command help
  - `exit/quit`: Exit simulator

#### 2.2.2 Memory Allocator Base Class
- **Responsibility**: Core memory management operations
- **Key Features**:
  - Physical memory simulation (byte array)
  - Free list management (doubly-linked list)
  - Block splitting and coalescing
  - Statistics tracking
  - Memory visualization

#### 2.2.3 Allocation Strategies
Three concrete implementations of allocation algorithms:

1. **First Fit**: Allocates first available block that satisfies request
2. **Best Fit**: Allocates smallest sufficient block
3. **Worst Fit**: Allocates largest available block

## 3. Data Structures

### 3.1 Memory Block Structure

```cpp
struct MemoryBlock {
    size_t size;              // Usable size (excluding header)
    bool allocated;           // Allocation status
    size_t address;           // Starting address in memory
    MemoryBlock* prev;        // Previous block in free list
    MemoryBlock* next;        // Next block in free list
};
```

**Design Rationale**:
- `size`: Tracks usable memory, excluding metadata overhead
- `allocated`: Distinguishes allocated blocks from free blocks
- `address`: Absolute address within simulated physical memory
- `prev/next`: Enables doubly-linked free list for O(1) insertion/removal

### 3.2 Free List Organization

The free list is maintained as a **doubly-linked list sorted by address**:

```
free_list_head → [Block@0x100] ⟷ [Block@0x300] ⟷ [Block@0x500] → NULL
```

**Benefits**:
- Sorted by address enables efficient coalescing
- Doubly-linked allows O(1) removal
- Address-ordered facilitates finding adjacent blocks

### 3.3 Allocated Block Tracking

Allocated blocks are tracked in a vector indexed by block ID:

```cpp
std::vector<MemoryBlock*> allocated_blocks;
```

**Block ID Assignment**:
- IDs start at 1 (user-friendly)
- Monotonically increasing
- Maps to vector index (block_id - 1)

## 4. Memory Allocation Algorithm

### 4.1 Allocation Process

```
1. Find suitable free block using strategy-specific search
2. If no block found → allocation failure
3. If exact fit:
   a. Remove block from free list
   b. Mark as allocated
4. If block larger than request:
   a. Create new allocated block at current address
   b. Shrink free block and adjust address
5. Assign block ID and add to allocated_blocks
6. Update statistics
```

### 4.2 Strategy-Specific Search

#### First Fit
```cpp
while (current_block) {
    if (current_block->size >= requested_size) {
        return current_block;  // First suitable block
    }
    current_block = current_block->next;
}
```

**Time Complexity**: O(n) worst case, O(1) best case

#### Best Fit
```cpp
MemoryBlock* best = nullptr;
size_t smallest_diff = SIZE_MAX;

while (current_block) {
    if (current_block->size >= requested_size) {
        size_t diff = current_block->size - requested_size;
        if (diff < smallest_diff) {
            smallest_diff = diff;
            best = current_block;
        }
    }
    current_block = current_block->next;
}
```

**Time Complexity**: O(n) - must scan entire list

#### Worst Fit
```cpp
MemoryBlock* worst = nullptr;
size_t largest_size = 0;

while (current_block) {
    if (current_block->size >= requested_size && 
        current_block->size > largest_size) {
        largest_size = current_block->size;
        worst = current_block;
    }
    current_block = current_block->next;
}
```

**Time Complexity**: O(n) - must scan entire list

### 4.3 Block Splitting

When a free block is larger than requested:

```
Before:
┌─────────────────────────────────┐
│     Free Block (size=200)        │
└─────────────────────────────────┘
Address: 0x100

After malloc(50):
┌───────────────┐ ┌───────────────┐
│ Allocated(50) │ │  Free(150)    │
└───────────────┘ └───────────────┘
Address: 0x100    Address: 0x132
```

**Implementation**:
```cpp
allocated_block = new MemoryBlock(requested_size, free_block->address);
free_block->size -= requested_size;
free_block->address += requested_size;
```

## 5. Memory Deallocation

### 5.1 Deallocation Process

```
1. Validate block ID
2. Check block is actually allocated
3. Add block to free list (maintains address order)
4. Attempt coalescing with adjacent free blocks
5. Update statistics
```

### 5.2 Coalescing Algorithm

**Goal**: Merge adjacent free blocks to reduce fragmentation

```cpp
// Coalesce with next block
if (block->next && block->address + block->size == block->next->address) {
    merge_with_next();
}

// Coalesce with previous block
if (block->prev && block->prev->address + block->prev->size == block->address) {
    merge_with_prev();
}
```

**Example**:
```
Before free(block_2):
┌────────┐ ┌────────┐ ┌────────┐ ┌────────┐
│ Block1 │ │ Block2 │ │ Block3 │ │ Block4 │
│  USED  │ │  USED  │ │  FREE  │ │  FREE  │
└────────┘ └────────┘ └────────┘ └────────┘

After free(block_2) + coalescing:
┌────────┐ ┌──────────────────────────────┐
│ Block1 │ │      Merged Free Block       │
│  USED  │ │     (Block2+Block3+Block4)   │
└────────┘ └──────────────────────────────┘
```

## 6. Fragmentation Metrics

### 6.1 External Fragmentation

**Definition**: Free memory that cannot be used due to being split into small, non-contiguous blocks.

**Formula**:
```
External Fragmentation = (1 - largest_free_block / total_free_memory) × 100%
```

**Example**:
- Total free memory: 1000 bytes
- Largest free block: 600 bytes

## 7. Cache Simulation Design (Summary)

The cache simulator models one or more cache levels using configurable geometry:
- Total size in bytes
- Block (line) size in bytes
- Associativity (direct-mapped or set-associative)
- Replacement policy (FIFO, LRU, LFU)

Each level is implemented by `CacheLevel`, which maintains:
- A 2D array of cache lines grouped into sets
- Per-line metadata: valid bit, tag, timestamps, access frequency
- Per-level statistics: accesses, hits, misses, hit ratio

Address mapping:
- `set_index = (address / block_size) % num_sets`
- `tag       = (address / block_size) / num_sets`

The `CacheHierarchy` composes multiple `CacheLevel` instances and on each access walks
levels from L1 downward until the first hit (or falls through to memory on global miss).

## 8. Virtual Memory Design (Summary)

Virtual memory is modeled as a single-process paged system:
- Virtual address space size (bytes)
- Page size (bytes)
- Physical memory size for VM (bytes)

From these values the simulator derives:
- Number of virtual pages = `virtual_size / page_size`
- Number of physical frames = `physical_size / page_size`

Each virtual page has a `PageTableEntry` containing:
- `valid`: whether the page is resident in a frame
- `frame_index`: physical frame number
- `load_time`: time when the page was loaded (for FIFO)
- `last_access_time`: last access time (for LRU)

The `VirtualMemoryManager` supports page replacement policies:
- FIFO: evicts the page with the oldest `load_time`
- LRU:  evicts the page with the oldest `last_access_time`

On each `vm_access` (virtual address):
1. The simulator computes `(vpage, offset)` from the virtual address.
2. If the page table entry for `vpage` is valid, it is a **page hit** and the physical
   address is `frame_index * page_size + offset`.
3. If invalid, it is a **page fault**:
   - If a free frame exists, the page is loaded there.
   - Otherwise, a victim frame is chosen according to FIFO/LRU and its page is evicted.
   - The new page is then loaded into the selected frame and the page table is updated.

Statistics tracked include:
- Total VM accesses
- Page hits and page faults
- Hit/fault rates (percentages)

When a cache hierarchy is configured, a successful VM translation produces a physical
address that is then passed through the cache simulation:

`Virtual Address → Page Table → Physical Address → Cache Hierarchy → Memory`
- External fragmentation: (1 - 600/1000) × 100% = 40%

### 6.2 Internal Fragmentation

**Definition**: Wasted space within allocated blocks.

**In this implementation**: Near zero, as we don't impose minimum block sizes or alignment requirements.

### 6.3 Memory Utilization

```
Memory Utilization = (used_memory / total_memory) × 100%
```

## 7. Memory Layout Visualization

### 7.1 Dump Format

```
[0x0000 - 0x0063] USED (id=1, size=100)
[0x0064 - 0x012b] FREE (size=200)
[0x012c - 0x01c1] USED (id=3, size=150)
```

**Fields**:
- Address range in hexadecimal
- Status: USED or FREE
- For allocated blocks: ID and size
- For free blocks: size only

## 8. Performance Characteristics

### 8.1 Time Complexity

| Operation | First Fit | Best Fit | Worst Fit |
|-----------|-----------|----------|-----------|
| Allocation | O(n) | O(n) | O(n) |
| Deallocation | O(n) | O(n) | O(n) |
| Coalescing | O(1) | O(1) | O(1) |

Where n = number of free blocks

### 8.2 Space Complexity

- Physical memory: O(m) where m = configured memory size
- Block metadata: O(b) where b = total number of blocks (free + allocated)
- Free list: O(f) where f = number of free blocks

### 8.3 Allocation Strategy Trade-offs

| Strategy | Pros | Cons |
|----------|------|------|
| **First Fit** | Fast allocation, simple | May cause fragmentation at beginning of memory |
| **Best Fit** | Minimizes wasted space | Slower (full scan), creates tiny unusable fragments |
| **Worst Fit** | Leaves larger usable fragments | Generally highest fragmentation, slowest |

## 9. Implementation Details

### 9.1 Memory Representation

Physical memory simulated as:
```cpp
char* memory = new char[size];
```

**Addressing**:
- Byte-level addressing (0 to size-1)
- No page boundaries or segments
- Contiguous address space

### 9.2 Block ID Management

```cpp
size_t next_block_id = 1;  // Starts at 1 for user-friendliness

// On allocation:
size_t block_id = next_block_id++;
allocated_blocks.push_back(allocated_block);
```

### 9.3 Statistics Tracking

```cpp
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
};
```

Updated on every allocation/deallocation operation.

## 10. Edge Cases and Error Handling

### 10.1 Invalid Operations

- **Allocating zero bytes**: Rejected with error message
- **Freeing invalid block ID**: Error, no state change
- **Double free**: Detected via `allocated` flag
- **Out of memory**: Tracked in `allocation_failures` statistic

### 10.2 Memory Safety

- Destructor properly cleans up all allocated blocks
- Separate tracking of free list and allocated blocks prevents double-free
- No memory leaks in normal operation

## 11. Testing Strategy

### 11.1 Unit Tests

- Individual allocation strategy correctness
- Coalescing in various scenarios
- Edge cases (empty memory, full memory)

### 11.2 Workload Tests

Test files in `tests/workloads/`:
- Sequential allocations
- Alternating alloc/free patterns
- Fragmentation stress tests
- Strategy comparison tests

### 11.3 Validation

- Memory dump inspection
- Statistics verification
- Fragmentation calculation accuracy

## 12. Limitations

- Single-threaded (no concurrency)

