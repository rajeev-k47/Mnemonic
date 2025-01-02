#include "../../include/cache.h"

CacheHierarchy::CacheHierarchy() = default;

void CacheHierarchy::set_levels(const vector<CacheConfig> &level_configs) {
  levels.clear();
  levels.reserve(level_configs.size());
  for (const auto &cfg : level_configs) {
    levels.emplace_back(CacheLevel(cfg));
  }
}

void CacheHierarchy::access(size_t address) {
  if (levels.empty()) {
    return;
  }

  bool hit = false;
  for (auto &level : levels) {
    if (level.access(address)) {
      hit = true;
      break;
    }
  }

  if (!hit) {
  }
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
