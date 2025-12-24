#include "../../include/cli.h"
#include <algorithm>
#include <iomanip>
#include <iostream>

using namespace std;

void CLI::handle_init(const vector<string> &args) {
  if (args.size() < 2 || args[0] != "memory") {
    cerr << "W[CLI] Usage: init memory <size>" << endl;
    return;
  }

  if (initialized) {
    cerr << "E[Memory] Memory already initialized" << endl;
    return;
  }

  try {
    size_t size = stoull(args[1]); // TODO implement malloc methods

  } catch (const exception &e) {
    cerr << "E[Memory] Invalid size: " << args[1] << endl;
  }
}

void CLI::handle_set_allocator(const vector<string> &args) {
  if (args.size() < 2 || args[0] != "allocator") {
    cerr << "W[CLI] Usage: set allocator <type>" << endl;
    return;
  }

  if (initialized) {
    cerr << "DL91" << endl;
    return;
  }

  string type = args[1];
  transform(type.begin(), type.end(), type.begin(), ::tolower);
  // TODO implement malloc methods
  if (allocator) {
    delete allocator;
    allocator = nullptr;
  }
}

void CLI::handle_malloc(const vector<string> &args) {
  if (!initialized) {
    cerr << "W[CLI] Use 'init memory <size>' first." << endl;
    return;
  }

  if (args.empty()) {
    cerr << "W[CLI] Usage: malloc <size>" << endl;
    return;
  }

  try {
    size_t size = stoull(args[0]); // TODO implement malloc methods
  } catch (const exception &e) {
    cerr << "E[Memory] Invalid size: " << args[0] << endl;
  }
}

void CLI::handle_free(const vector<string> &args) {
  if (!initialized) {
    cerr << "W[CLI] Use 'init memory <size>' first." << endl;
    return;
  }

  if (args.empty()) {
    cerr << "W[I] Usage: free <block_id>" << endl;
    return;
  }

  try {
    size_t block_id = stoull(args[0]); // TODO implement malloc methods
  } catch (const exception &e) {
    cerr << "E[Memory] Invalid block ID: " << args[0] << endl;
  }
}

void CLI::handle_dump() {
  if (!initialized) {
    cerr << "W[CLI] Use 'init memory <size>' first." << endl;
    return;
  }
  // TODO implement malloc methods
  allocator->dump_memory();
}

void CLI::handle_stats() {
  if (!initialized) {
    cerr << "W[CLI] Use 'init memory <size>' first." << endl;
    return;
  }

  AllocationStats stats = allocator->get_stats();

  cout << "\n~~~~~~~Memory Statistics~~~~~~" << endl;
  cout << "Allocator: " << allocator->get_allocator_name() << endl;
  cout << "Total memory: " << stats.total_memory << " bytes" << endl;
  cout << "Used memory: " << stats.used_memory << " bytes" << endl;
  cout << "Free memory: " << stats.free_memory << " bytes" << endl;
  double utilization = stats.total_memory > 0 ? (double)stats.used_memory /
                                                    stats.total_memory * 100.0
                                              : 0.0;
  cout << "Memory utilization: " << utilization << "%" << endl;
  cout << "Number of allocations: " << stats.num_allocations << endl;
  cout << "Number of deallocations: " << stats.num_deallocations << endl;
  cout << "Allocation failures: " << stats.allocation_failures << endl;
  size_t total_requests = stats.num_allocations + stats.allocation_failures;
  double success_rate = total_requests > 0 ? (double)stats.num_allocations /
                                                 total_requests * 100.0
                                           : 0.0;
  double failure_rate = total_requests > 0 ? (double)stats.allocation_failures /
                                                 total_requests * 100.0
                                           : 0.0;
  cout << "Allocation success rate: " << success_rate << "%" << endl;
  cout << "Allocation failure rate: " << failure_rate << "%" << endl;
  cout << "Allocated blocks: " << stats.num_allocated_blocks << endl;
  cout << "Free blocks: " << stats.num_free_blocks << endl;
  cout << "External fragmentation: " << fixed << setprecision(2)
       << stats.external_fragmentation << "%" << endl;
  cout << "Internal fragmentation: " << fixed << setprecision(2)
       << stats.internal_fragmentation << "%" << endl;
  cout << endl;
}
