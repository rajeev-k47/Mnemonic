#include "../../include/cli.h"
#include <algorithm>
#include <iostream>
#include <sstream>

using namespace std;

CLI::CLI() {}
CLI::~CLI() {}

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

    // execute_command(cmd);
    cout << input << endl;
  }
}
void CLI::execute_command(const Command &cmd) {}
