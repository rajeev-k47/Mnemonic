#ifndef CACHE_H
#define CACHE_H

#include <cstddef>
#include <string>
#include <vector>

using namespace std;
enum class ReplacementPolicy { FIFO, LRU, LFU };

struct CacheStats {
  string level_name;
  size_t accesses;
  size_t hits;
  size_t misses;

  CacheStats(const string &name = "")
      : level_name(name), accesses(0), hits(0), misses(0) {}

  double hit_ratio() const {
    return accesses > 0 ? static_cast<double>(hits) / accesses * 100.0 : 0.0;
  }
};

struct CacheConfig {
  string name;
  size_t size_bytes;
  size_t block_size;
  size_t associativity;
  ReplacementPolicy policy;

  CacheConfig(const string &n = "", size_t size = 0, size_t block = 0,
              size_t assoc = 1, ReplacementPolicy p = ReplacementPolicy::FIFO)
      : name(n), size_bytes(size), block_size(block), associativity(assoc),
        policy(p) {}
};

class CacheLevel {
public:
  explicit CacheLevel(const CacheConfig &config = CacheConfig());

  bool access(size_t address);

  const CacheStats &get_stats() const { return stats; }
  const CacheConfig &get_config() const { return config; }

  void reset();

private:
  struct CacheLine {
    bool valid;
    size_t tag;
    size_t last_access_time;
    size_t insert_time;
    size_t frequency;

    CacheLine()
        : valid(false), tag(0), last_access_time(0), insert_time(0),
          frequency(0) {}
  };

  CacheConfig config;
  CacheStats stats;

  size_t num_sets;
  size_t global_time;

  vector<vector<CacheLine>> sets;

  size_t get_set_index(size_t address) const;
  size_t get_tag(size_t address) const;

  int find_line_with_tag(size_t set_index, size_t tag) const;
  int select_victim_line(size_t set_index);
};

class CacheHierarchy {
public:
  CacheHierarchy();

  void set_levels(const vector<CacheConfig> &level_configs);
  int access(size_t address);
  vector<CacheStats> get_stats() const;
  void reset();
  bool empty() const { return levels.empty(); }

private:
  vector<CacheLevel> levels;
};

#endif
