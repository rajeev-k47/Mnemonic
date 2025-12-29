#include "../../include/cache.h"
#include <stdexcept>

using namespace std;

CacheLevel::CacheLevel(const CacheConfig &cfg)
    : config(cfg), stats(cfg.name), num_sets(0), global_time(0) {
  if (config.size_bytes == 0 || config.block_size == 0 ||
      config.associativity == 0) {
    num_sets = 0;
    return;
  }

  if (config.size_bytes % (config.block_size * config.associativity) != 0) {
    size_t lines = config.size_bytes / config.block_size;
    size_t sets = lines / config.associativity;
    config.size_bytes = sets * config.block_size * config.associativity;
  }

  size_t total_lines = config.size_bytes / config.block_size;
  num_sets = total_lines / config.associativity;
  if (num_sets == 0) {
    throw runtime_error("E[Cache] Invalid cache configuration");
  }

  sets.assign(num_sets, vector<CacheLine>(config.associativity));
}

size_t CacheLevel::get_set_index(size_t address) const {
  size_t block_addr = address / config.block_size;
  return block_addr % num_sets;
}

size_t CacheLevel::get_tag(size_t address) const {
  size_t block_addr = address / config.block_size;
  return block_addr / num_sets;
}

int CacheLevel::find_line_with_tag(size_t set_index, size_t tag) const {
  const auto &set = sets[set_index];
  for (size_t way = 0; way < set.size(); ++way) {
    if (set[way].valid && set[way].tag == tag) {
      return static_cast<int>(way);
    }
  }
  return -1;
}

int CacheLevel::select_victim_line(size_t set_index) {
  auto &set = sets[set_index];

  for (size_t way = 0; way < set.size(); ++way) {
    if (!set[way].valid) {
      return static_cast<int>(way);
    }
  }

  size_t victim = 0;
  switch (config.policy) {
  case ReplacementPolicy::FIFO: {
    size_t oldest_insert = set[0].insert_time;
    for (size_t way = 1; way < set.size(); ++way) {
      if (set[way].insert_time < oldest_insert) {
        oldest_insert = set[way].insert_time;
        victim = way;
      }
    }
    break;
  }
  case ReplacementPolicy::LRU: {
    size_t oldest_access = set[0].last_access_time;
    for (size_t way = 1; way < set.size(); ++way) {
      if (set[way].last_access_time < oldest_access) {
        oldest_access = set[way].last_access_time;
        victim = way;
      }
    }
    break;
  }
  case ReplacementPolicy::LFU: {
    size_t lowest_freq = set[0].frequency;
    for (size_t way = 1; way < set.size(); ++way) {
      if (set[way].frequency < lowest_freq) {
        lowest_freq = set[way].frequency;
        victim = way;
      }
    }
    break;
  }
  }

  return static_cast<int>(victim);
}

bool CacheLevel::access(size_t address) {
  if (num_sets == 0) {
    return false;
  }

  ++stats.accesses;
  ++global_time;

  size_t set_index = get_set_index(address);
  size_t tag = get_tag(address);

  int line_index = find_line_with_tag(set_index, tag);
  auto &set = sets[set_index];

  if (line_index >= 0) {
    CacheLine &line = set[static_cast<size_t>(line_index)];
    ++stats.hits;
    line.last_access_time = global_time;
    ++line.frequency;
    return true;
  }

  ++stats.misses;
  int victim_index = select_victim_line(set_index);
  CacheLine &victim = set[static_cast<size_t>(victim_index)];

  victim.valid = true;
  victim.tag = tag;
  victim.insert_time = global_time;
  victim.last_access_time = global_time;
  victim.frequency = 1;

  return false;
}

void CacheLevel::reset() {
  stats.accesses = stats.hits = stats.misses = 0;
  global_time = 0;
  for (auto &set : sets) {
    for (auto &line : set) {
      line.valid = false;
      line.tag = 0;
      line.last_access_time = 0;
      line.insert_time = 0;
      line.frequency = 0;
    }
  }
}
