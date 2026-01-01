#include "../../include/vm.h"

#include <iostream>

VirtualMemoryManager::VirtualMemoryManager()
    : initialized(false), virtual_size_bytes(0), physical_size_bytes(0),
      page_size(0), num_virtual_pages(0), num_frames(0),
      policy(PageReplacementPolicy::FIFO), global_time(0) {}

bool VirtualMemoryManager::init(size_t vsize, size_t page_sz, size_t psize,
                                PageReplacementPolicy pol) {
  if (page_sz == 0) {
    cerr << "E[VM] VM init error" << endl;
    return false;
  }

  if (vsize < page_sz || psize < page_sz) {
    cerr << "E[VM] VM init error" << endl;
    return false;
  }

  vsize -= vsize % page_sz;
  psize -= psize % page_sz;

  size_t vpages = vsize / page_sz;
  size_t frames = psize / page_sz;

  if (vpages == 0 || frames == 0) {
    cerr << "E[VM] VM init error" << endl;
    return false;
  }

  virtual_size_bytes = vsize;
  physical_size_bytes = psize;
  page_size = page_sz;
  num_virtual_pages = vpages;
  num_frames = frames;
  policy = pol;

  page_table.assign(num_virtual_pages, PageTableEntry());
  frame_to_vpage.assign(num_frames, -1);

  global_time = 0;

  stats = VMStats();
  stats.virtual_size_bytes = virtual_size_bytes;
  stats.physical_size_bytes = physical_size_bytes;
  stats.page_size = page_size;
  stats.num_virtual_pages = num_virtual_pages;
  stats.num_frames = num_frames;

  initialized = true;

  cout << "I[VM] INIT VAS=" << virtual_size_bytes
       << " bytes, PM=" << physical_size_bytes
       << " bytes, page size=" << page_size << " bytes" << endl;

  return true;
}

size_t VirtualMemoryManager::choose_victim_frame() {
  for (size_t frame = 0; frame < num_frames; ++frame) {
    if (frame_to_vpage[frame] == -1) {
      return frame;
    }
  }
  size_t victim_frame = 0;

  switch (policy) {
  case PageReplacementPolicy::FIFO: {
    size_t oldest_load = page_table[frame_to_vpage[0]].load_time;
    for (size_t frame = 1; frame < num_frames; ++frame) {
      size_t vpage = static_cast<size_t>(frame_to_vpage[frame]);
      size_t load = page_table[vpage].load_time;
      if (load < oldest_load) {
        oldest_load = load;
        victim_frame = frame;
      }
    }
    break;
  }
  case PageReplacementPolicy::LRU: {
    size_t oldest_access = page_table[frame_to_vpage[0]].last_access_time;
    for (size_t frame = 1; frame < num_frames; ++frame) {
      size_t vpage = static_cast<size_t>(frame_to_vpage[frame]);
      size_t acc = page_table[vpage].last_access_time;
      if (acc < oldest_access) {
        oldest_access = acc;
        victim_frame = frame;
      }
    }
    break;
  }
  }

  return victim_frame;
}

void VirtualMemoryManager::load_page_into_frame(size_t vpage,
                                                size_t frame_index,
                                                bool evict_existing) {
  if (evict_existing) {
    int existing_vpage = frame_to_vpage[frame_index];
    if (existing_vpage >= 0) {
      page_table[static_cast<size_t>(existing_vpage)].valid = false;
    }
  }

  frame_to_vpage[frame_index] = static_cast<int>(vpage);

  PageTableEntry &pte = page_table[vpage];
  pte.valid = true;
  pte.frame_index = frame_index;
  pte.load_time = global_time;
  pte.last_access_time = global_time;
}

TranslationResult VirtualMemoryManager::access(size_t vaddr) {
  TranslationResult res;
  res.virtual_address = vaddr;

  if (!initialized) {
    res.success = false;
    res.message = "Virtual memory not initialized";
    return res;
  }

  if (vaddr >= virtual_size_bytes) {
    res.success = false;
    res.message = "Virtual address out of range";
    return res;
  }

  ++global_time;
  ++stats.accesses;

  size_t vpage = vaddr / page_size;
  size_t offset = vaddr % page_size;
  res.virtual_page = vpage;

  PageTableEntry &pte = page_table[vpage];

  if (pte.valid) {
    ++stats.page_hits;
    pte.last_access_time = global_time;

    size_t frame_index = pte.frame_index;
    res.frame_index = frame_index;
    res.physical_address = frame_index * page_size + offset;
    res.success = true;
    res.page_fault = false;
    res.message = "Page hit";
    return res;
  }

  ++stats.page_faults;
  res.page_fault = true;
  size_t frame_index = choose_victim_frame();
  bool evict = (frame_to_vpage[frame_index] != -1);

  load_page_into_frame(vpage, frame_index, evict);

  res.frame_index = frame_index;
  res.physical_address = frame_index * page_size + offset;
  res.success = true;
  res.message = evict ? "replaced victim page" : "loaded into free frame";

  return res;
}

VMStats VirtualMemoryManager::get_stats() const { return stats; }

void VirtualMemoryManager::reset() {
  if (!initialized)
    return;

  page_table.assign(num_virtual_pages, PageTableEntry());
  frame_to_vpage.assign(num_frames, -1);

  global_time = 0;

  stats.accesses = 0;
  stats.page_hits = 0;
  stats.page_faults = 0;
}
