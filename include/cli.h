#ifndef CLI_H
#define CLI_H

#include "allocator.h"
#include "buddy.h"
#include "cache.h"
#include "vm.h"
#include <string>
#include <vector>

using namespace std;

enum class CommandType {
  INIT,
  UNKNOWN,
  EXIT,
  SET_ALLOCATOR,
  MALLOC,
  FREE,
  DUMP,
  STATS,
  CACHE_INIT,
  CACHE_ACCESS,
  CACHE_STATS
};

struct Command {
  CommandType type;
  vector<string> args;

  explicit Command(CommandType t = CommandType::UNKNOWN) : type(t) {}
};

class CommandParser {
public:
  static Command parse(const string &input);

private:
  static vector<string> tokenize(const string &input);
  static CommandType get_command_type(const string &cmd);
};

class CLI {
public:
  CLI();
  ~CLI();
  void run();
  void execute_command(const Command &cmd);

private:
  MemoryAllocator *allocator;
  BuddyAllocator *buddy_allocator;
  bool initialized;
  bool use_buddy;

  CacheHierarchy cache_hierarchy;
  bool cache_initialized;

  VirtualMemoryManager vm_manager;
  bool vm_initialized;

  void handle_init(const vector<string> &args);
  void handle_set_allocator(const vector<string> &args);
  void handle_malloc(const vector<string> &args);
  void handle_free(const vector<string> &args);
  void handle_dump();
  void handle_stats();

  void handle_cache_init(const vector<string> &args);
  void handle_cache_access(const vector<string> &args);
  void handle_cache_stats();
};

#endif
