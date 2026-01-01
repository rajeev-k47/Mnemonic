#ifndef VIRTUAL_MEMORY_H
#define VIRTUAL_MEMORY_H

#include <cstddef>
#include <string>
#include <vector>

using namespace std;

enum class PageReplacementPolicy { FIFO, LRU };

struct PageTableEntry {
  bool valid;
  size_t frame_index;
  size_t load_time;
  size_t last_access_time;

  PageTableEntry()
      : valid(false), frame_index(0), load_time(0), last_access_time(0) {}
};

struct VMStats {
  size_t virtual_size_bytes;
  size_t physical_size_bytes;
  size_t page_size;
  size_t num_virtual_pages;
  size_t num_frames;

  size_t accesses;
  size_t page_hits;
  size_t page_faults;

  VMStats()
      : virtual_size_bytes(0), physical_size_bytes(0), page_size(0),
        num_virtual_pages(0), num_frames(0), accesses(0), page_hits(0),
        page_faults(0) {}

  double fault_rate() const {
    return accesses > 0 ? static_cast<double>(page_faults) / accesses * 100.0
                        : 0.0;
  }
  double hit_rate() const {
    return accesses > 0 ? static_cast<double>(page_hits) / accesses * 100.0
                        : 0.0;
  }
};

struct TranslationResult {
  bool success;
  bool page_fault;

  size_t virtual_address;
  size_t physical_address;
  size_t virtual_page;
  size_t frame_index;

  string message;

  TranslationResult()
      : success(false), page_fault(false), virtual_address(0),
        physical_address(0), virtual_page(0), frame_index(0), message("") {}
};

class VirtualMemoryManager {
public:
  VirtualMemoryManager();

  bool init(size_t virtual_size_bytes, size_t page_size_bytes,
            size_t physical_size_bytes,
            PageReplacementPolicy policy = PageReplacementPolicy::FIFO);

  TranslationResult access(size_t virtual_address);

  VMStats get_stats() const;

  void reset();
  bool is_initialized() const { return initialized; }

private:
  bool initialized;

  size_t virtual_size_bytes;
  size_t physical_size_bytes;
  size_t page_size;
  size_t num_virtual_pages;
  size_t num_frames;

  PageReplacementPolicy policy;

  vector<PageTableEntry> page_table;
  vector<int> frame_to_vpage;

  size_t global_time;

  VMStats stats;

  size_t choose_victim_frame();
  void load_page_into_frame(size_t vpage, size_t frame_index,
                            bool evict_existing);
};

#endif
