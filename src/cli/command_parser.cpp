#include "../../include/cli.h"
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>

using namespace std;

CLI::CLI()
    : allocator(nullptr), buddy_allocator(nullptr), use_buddy(false),
      initialized(false) {}

CLI::~CLI() {
  if (allocator) {
    delete allocator;
  }
  if (buddy_allocator) {
    delete buddy_allocator;
  }
}

vector<string> CommandParser::tokenize(const string &input) {
  vector<string> tokens;
  istringstream iss(input);
  string token;

  while (iss >> token) {
    tokens.push_back(token);
  }
  return tokens;
}

CommandType CommandParser::get_command_type(const string &cmd) {
  string lower_cmd = cmd;
  transform(lower_cmd.begin(), lower_cmd.end(), lower_cmd.begin(), ::tolower);

  if (lower_cmd == "init")
    return CommandType::INIT;
  if (lower_cmd == "exit")
    return CommandType::EXIT;
  if (lower_cmd == "set")
    return CommandType::SET_ALLOCATOR;
  if (lower_cmd == "malloc")
    return CommandType::MALLOC;
  if (lower_cmd == "free")
    return CommandType::FREE;
  if (lower_cmd == "dump")
    return CommandType::DUMP;
  if (lower_cmd == "stats")
    return CommandType::STATS;
  if (lower_cmd == "cache_init")
    return CommandType::CACHE_INIT;
  if (lower_cmd == "cache_access")
    return CommandType::CACHE_ACCESS;
  if (lower_cmd == "cache_stats")
    return CommandType::CACHE_STATS;

  return CommandType::UNKNOWN;
}

Command CommandParser::parse(const string &input) {
  vector<string> tokens = tokenize(input);

  if (tokens.empty()) {
    return Command(CommandType::UNKNOWN);
  }

  CommandType type = get_command_type(tokens[0]);
  Command cmd(type);

  for (size_t i = 1; i < tokens.size(); i++) {
    cmd.args.push_back(tokens[i]);
  }

  return cmd;
}

void CLI::run() {
  cout << "~~~MNEMONIC~~~" << endl;

  string input;

  while (true) {
    cout << "> ";
    getline(cin, input);
    if (cin.eof()) {
      break;
    }
    if (input.empty()) {
      continue;
    }
    Command cmd = CommandParser::parse(input);

    if (cmd.type == CommandType::EXIT) {
      cout << "Exiting.." << endl;
      break;
    }

    execute_command(cmd);
  }
}
void CLI::execute_command(const Command &cmd) {
  switch (cmd.type) {
  case CommandType::INIT:
    handle_init(cmd.args);
    break;
  case CommandType::SET_ALLOCATOR:
    handle_set_allocator(cmd.args);
    break;
  case CommandType::MALLOC:
    handle_malloc(cmd.args);
    break;
  case CommandType::FREE:
    handle_free(cmd.args);
    break;
  case CommandType::DUMP:
    handle_dump();
    break;
  case CommandType::STATS:
    handle_stats();
    break;
  case CommandType::CACHE_INIT:
    handle_cache_init(cmd.args);
    break;
  case CommandType::CACHE_ACCESS:
    handle_cache_access(cmd.args);
    break;
  case CommandType::CACHE_STATS:
    handle_cache_stats();
    break;

  case CommandType::UNKNOWN:
    cerr << "[W] Unknown command" << endl;
    break;
  default:
    break;
  }
}

void CLI::handle_cache_init(const vector<string> &args) {
  if (args.size() != 3 && args.size() != 6) {
    cerr << "W[Cache] Use cache_init <L1_size> <L1_block> <L1_assoc> [L2_size "
            "L2_block L2_assoc]"
         << endl;
    return;
  }

  try {
    size_t l1_size = stoull(args[0]);
    size_t l1_block = stoull(args[1]);
    size_t l1_assoc = stoull(args[2]);

    CacheConfig l1_cfg("L1", l1_size, l1_block, l1_assoc,
                       ReplacementPolicy::FIFO);

    vector<CacheConfig> cfgs;
    cfgs.push_back(l1_cfg);

    if (args.size() == 6) {
      size_t l2_size = stoull(args[3]);
      size_t l2_block = stoull(args[4]);
      size_t l2_assoc = stoull(args[5]);
      CacheConfig l2_cfg("L2", l2_size, l2_block, l2_assoc,
                         ReplacementPolicy::FIFO);
      cfgs.push_back(l2_cfg);
    }

    cache_hierarchy.set_levels(cfgs);
    cache_initialized = true;

    cout << "E[Cache] Cache init with " << cfgs.size() << " level(s)." << endl;
  } catch (const exception &) {
    cerr << "E[Cache] Invalid cache" << endl;
  }
}

void CLI::handle_cache_access(const vector<string> &args) {
  if (!cache_initialized) {
    cerr << "W[Cache] Use 'cache_init' first." << endl;
    return;
  }

  if (args.empty()) {
    cerr << "W[Cache] Use cache_access <address>" << endl;
    return;
  }

  try {
    size_t address = 0;
    if (args[0].rfind("0x", 0) == 0 || args[0].rfind("0X", 0) == 0) {
      address = stoull(args[0], nullptr, 16);
    } else {
      address = stoull(args[0]);
    }

    cache_hierarchy.access(address);
  } catch (const exception &) {
    cerr << "E[Cache] Invalid address: " << args[0] << endl;
  }
}

void CLI::handle_cache_stats() {
  if (!cache_initialized) {
    cerr << "E[Cache]  Use 'cache_init' first." << endl;
    return;
  }

  auto stats_vec = cache_hierarchy.get_stats();

  cout << "\n~~~~~~Cache Statistics~~~~~" << endl;
  for (const auto &st : stats_vec) {
    cout << st.level_name << ":" << endl;
    cout << "Accesses:" << st.accesses << endl;
    cout << "Hits:" << st.hits << endl;
    cout << "Misses:" << st.misses << endl;
    cout << "Hit ratio:" << fixed << setprecision(2) << st.hit_ratio() << "%"
         << endl;
  }
  cout << endl;
}

void CLI::handle_vm_init(const std::vector<std::string> &args) {
  if (args.size() < 3) {
    std::cerr << "W[VM] Use vm_init <vsize> <page> <psize>" << std::endl;
    return;
  }

  try {
    size_t vsize = std::stoull(args[0]);
    size_t page = std::stoull(args[1]);
    size_t psize = std::stoull(args[2]);

    if (vm_manager.init(vsize, page, psize, PageReplacementPolicy::FIFO)) {
      vm_initialized = true;
    }
  } catch (const std::exception &) {
    std::cerr << "E[VM] Invalid parameters" << std::endl;
  }
}

void CLI::handle_vm_access(const std::vector<std::string> &args) {
  if (!vm_initialized) {
    std::cerr << "E[VM] Use 'vm_init' first." << std::endl;
    return;
  }

  if (args.empty()) {
    std::cerr << "W[VM] Use vm_access <vaddr>" << std::endl;
    return;
  }

  try {
    size_t vaddr = 0;
    if (args[0].rfind("0x", 0) == 0 || args[0].rfind("0X", 0) == 0) {
      vaddr = std::stoull(args[0], nullptr, 16);
    } else {
      vaddr = std::stoull(args[0]);
    }

    TranslationResult tr = vm_manager.access(vaddr);
    if (!tr.success) {
      std::cerr << "VM access error: " << tr.message << std::endl;
      return;
    }

    std::cout << "VM access: VA=0x" << std::hex << vaddr
              << " (page=" << std::dec << tr.virtual_page << ") -> PA=0x"
              << std::hex << tr.physical_address << " (frame=" << std::dec
              << tr.frame_index << ")";
    if (tr.page_fault) {
      std::cout << " [PAGE FAULT]";
    } else {
      std::cout << " [HIT]";
    }
    std::cout << std::endl;

    if (cache_initialized) {
      cache_hierarchy.access(tr.physical_address);
    }

  } catch (const std::exception &) {
    std::cerr << "E[VM] Invalid virt. address: " << args[0] << std::endl;
  }
}

void CLI::handle_vm_stats() {
  if (!vm_initialized) {
    std::cerr << "E[VM] Use 'vm_init' first." << std::endl;
    return;
  }

  VMStats s = vm_manager.get_stats();

  std::cout << "\n~~~~~~~~VM Statistics~~~~~~~~" << std::endl;
  std::cout << "Virt. address space:" << s.virtual_size_bytes << " bytes"
            << std::endl;
  std::cout << "Physical memory (VM):" << s.physical_size_bytes << " bytes"
            << std::endl;
  std::cout << "Page size:" << s.page_size << " bytes" << std::endl;
  std::cout << "Virtual pages:  " << s.num_virtual_pages << std::endl;
  std::cout << "Physical frames:" << s.num_frames << std::endl;
  std::cout << "VM accesses:" << s.accesses << std::endl;
  std::cout << "Page hits:" << s.page_hits << std::endl;
  std::cout << "Page faults:" << s.page_faults << std::endl;
  std::cout << "Page hit rate:" << std::fixed << std::setprecision(2)
            << s.hit_rate() << "%" << std::endl;
  std::cout << "Page fault rate:       " << std::fixed << std::setprecision(2)
            << s.fault_rate() << "%" << std::endl;
  std::cout << std::endl;
}
