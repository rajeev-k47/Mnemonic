#include "../../include/cache.h"

CacheHierarchy::CacheHierarchy() = default;

void CacheHierarchy::set_levels(const vector<CacheConfig> &level_configs) {
  levels.clear();
  levels.reserve(level_configs.size());
  for (const auto &cfg : level_configs) {
    levels.emplace_back(CacheLevel(cfg));
  }
}

int CacheHierarchy::access(size_t address) {
  if (levels.empty()) {
    return -1;
  }

  for (size_t i = 0; i < levels.size(); ++i) {
    if (levels[i].access(address)) {
      return static_cast<int>(i);
    }
  }

  return -1;
}

vector<CacheStats> CacheHierarchy::get_stats() const {
  vector<CacheStats> out;
  out.reserve(levels.size());
  for (const auto &level : levels) {
    out.push_back(level.get_stats());
  }
  return out;
}

void CacheHierarchy::reset() {
  for (auto &level : levels) {
    level.reset();
  }
}
