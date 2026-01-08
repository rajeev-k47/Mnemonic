# Mnemonic

A memory management simulator that models OS-level memory management including dynamic allocation strategies, multilevel cache simulation, and virtual memory with paging.

## Features

- **Physical Memory Simulation**: Contiguous memory block with byte-level addressing
- **Multiple Allocation Strategies**:
  - First Fit: Allocates first available block that fits
  - Best Fit: Allocates smallest sufficient block
  - Worst Fit: Allocates largest available block
  - Buddy System: Powerof two allocator with fast splitting/coalescing
- **Memory Management**:
  - Dynamic allocation and deallocation
  - Automatic coalescing of adjacent free blocks
  - Block splitting for efficient space utilization
- **Statistics & Visualization**:
  - Memory dump showing allocated and free blocks
  - Fragmentation metrics (internal and external)
  - Allocation success/failure tracking

## Project Structure

```
mnemonic/
├── include/             
│   ├── allocator.h      # MemoryAlloc structure
│   ├── buddy.h          # BuddyAlloc impl
│   ├── cache.h          # Cache config structure
│   ├── cli.h            # CLI
│   └── vm.h             # Virt. memory implementation
├── src/
│   ├── allocator/      
│   │   ├── best_fit.cpp
│   │   ├── first_fit.cpp
│   │   ├── memory_manager.cpp
│   │   └── worst_fit.cpp
│   ├── buddy/           
│   │   └── buddy_allocator.cpp # Buddy system logic
│   ├── cache/           # Cache simulation
│   │   ├── cache.cpp
│   │   └── cache_heirarchy.cpp
│   ├── cli/             # CLI implementation and helper func.
│   │   ├── command_parser.cpp
│   │   └── handlers.cpp
│   ├── vm/              # Virtual memory
│   │   └── vm_manager.cpp
│   └── main.cpp      
├── tests/               # Files for testing
│   └── workloads/
│       ├── basic_test.txt
│       ├── fragmentation_test.txt
│       ├── strategy_comparison.txt
│       ├── vm_basic.txt
│       └── vm_cache_integration.txt
└── Makefile           
```

## Building

### Compilation

```bash
git clone https://github.com/rajeev-k47/Mnemonic
cd Mnemonic
make
./mnemonic
```

### Testing
For testing purpose refer to 
[Testing Doc](docs/tests.md)


## Usage

#### Initialize Memory
```
init memory <size>
```

#### Set Allocation Strategy
```
set allocator <type> # Must be called before init or default strategy = first_fit
```
**Types:** `first_fit`, `best_fit`, `worst_fit`, `buddy`

#### Allocate Memory
```
malloc <size>
```
Allocate a memory block of specified size.

#### Free Memory
```
free <block_id>
```
Free an allocated memory block by its ID.

#### Display Memory Layout
```
dump
```
Show current memory state with all allocated and free blocks.

#### Show Statistics
```
stats
```
Display comprehensive memory statistics.

#### Initialize Cache Hierarchy
```
cache_init <L1_size> <L1_block> <L1_assoc> [L2_size L2_block L2_assoc]
```
Configure a 1 or 2-level cache hierarchy.

**Example:**
```
> cache_init 1024 16 1
Cache hierarchy initialized with 1 level.
```

#### Access Cache
```
cache_access <address>
```
Simulate a cache access to the given physical address.

#### Show Cache Statistics
```
cache_stats
```
Display per-level cache hit/miss counts and hit ratios.

#### Initialize Virtual Memory
```
vm_init <vsize> <page> <psize>
```
Initialize a paged virtual memory system.
- `vsize`: virtual address space size in bytes
- `page`: page size in bytes
- `psize`: physical memory size (for VM) in bytes

#### Virtual Memory Access
```
vm_access <vaddr>
```
Access a virtual address. The simulator will:
- Translate VA to page number + offset
- Check the page table
- On page hit: compute the physical address directly
- On page fault: choose a frame (FIFO/LRU), evict victim if needed, load page, then compute physical address
- If a cache is configured, the resulting physical address is also sent through the cache hierarchy.

#### Virtual Memory Statistics
```
vm_stats
```
Show virtual memory configuration and statistics (page hits, page faults, hit/fault rates).

**Example:**
```
> stats

~~~~~~~Memory Statistics~~~~~~
Allocator: First Fit
Total memory: 1024 bytes
Used memory: 0 bytes
Free memory: 1024 bytes
Memory utilization: 0.00%
Number of allocations: 1
Number of deallocations: 1
Allocation failures: 0
Allocation success rate: 100.00%
Allocation failure rate: 0.00%
Allocated blocks: 0
Free blocks: 1
External fragmentation: 0.00%
Internal fragmentation: 0.00%

```

#### Help
```
help
```

#### Exit
```
exit
```

### Sample Usecase:

```
$ ./mnemonic
~~~MNEMONIC~~~

> set allocator first_fit
I[Allocator] Alloc: First Fit
> init memory 1024
I[Allocator] Memory initialized: 1024 bytes
> malloc 100
I[Allocator] Allocated block id=1 at address=0x0000 (size=100)
> malloc 200
I[Allocator] Allocated block id=2 at address=0x0064 (size=200)
> malloc 150
I[Allocator] Allocated block id=3 at address=0x012c (size=150)

> dump
~~~~~Memory Dump~~~~~~
[0x0000 - 0x0063] I[Allocator] USED (id=1, size=100)
[0x0064 - 0x012b] I[Allocator] USED (id=2, size=200)
[0x012c - 0x01c1] I[Allocator] USED (id=3, size=150)
[0x01c2 - 0x03ff] I[Allocator] FREE (size=574)

> free 2
I[Deallocator] Block 2 freed

> dump
~~~~~Memory Dump~~~~~~
[0x0000 - 0x0063] I[Allocator] USED (id=1, size=100)
[0x0064 - 0x012b] I[Allocator] FREE (size=200)
[0x012c - 0x01c1] I[Allocator] USED (id=3, size=150)
[0x01c2 - 0x03ff] I[Allocator] FREE (size=574)

> stats

~~~~~~~Memory Statistics~~~~~~
Allocator: First Fit
Total memory: 1024 bytes
Used memory: 250 bytes
Free memory: 774 bytes
Memory utilization: 24.4141%
Number of allocations: 3
Number of deallocations: 1
Allocation failures: 0
Allocation success rate: 100%
Allocation failure rate: 0%
Allocated blocks: 2
Free blocks: 2
External fragmentation: 25.84%
Internal fragmentation: 0.00%

# Cache

> cache_init 256 64 1
I[Cache] Cache init with 1 level.

> cache_access 0x1000
Address 0x1000 - L1 MISS - Loaded from memory

> cache_access 0x1000
Address 0x1000 - L1 HIT

> cache_stats
~~~~~~Cache Statistics~~~~~
L1:
Accesses:2
Hits:1
Misses:1
Hit ratio:50.00%

# 2 Way with L2

> cache_init 512 64 2 2048 128 4
E[Cache] Cache init with 2 level(s).
> cache_access 0x0000
Address 0x0 - L1 MISS, L2 MISS - Loaded from memory
> cache_access 0x0040
Address 0x40 - L1 MISS, L2 HIT
> cache_access 0x1000
Address 0x1000 - L1 MISS, L2 MISS - Loaded from memory
> cache_access 0x0000
Address 0x0 - L1 HIT
> cache_stats

~~~~~~Cache Statistics~~~~~
L1:
Accesses:4
Hits:1
Misses:3
Hit ratio:25.00%
L2:
Accesses:3
Hits:1
Misses:2
Hit ratio:33.33%

# Virtual memory

> vm_init 16 8 4096
I[VM] INIT VAS=16 bytes, PM=4096 bytes, page size=8 bytes
> vm_access 0x0000
VM access: VA=0x0 (page=0) -> PA=0x0 (frame=0) [PAGE FAULT]
> vm_access 0x0000
VM access: VA=0x0 (page=0) -> PA=0x0 (frame=0) [HIT]

> vm_stats
~~~~~~~~VM Statistics~~~~~~~~
Virt. address space:16 bytes
Physical memory (VM):4096 bytes
Page size:8 bytes
Virtual pages:  2
Physical frames:512
VM accesses:2
Page hits:1
Page faults:1
Page hit rate:50.00%
Page fault rate:       50.00%

> exit
Exiting...
```
