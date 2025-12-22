#ifndef CLI_H
#define CLI_H

#include <string>
#include <vector>

using namespace std;

enum class CommandType {
    INIT,
    UNKNOWN,
    EXIT
};

struct Command {
    CommandType type;
    vector<string> args;

    explicit Command(CommandType t = CommandType::UNKNOWN)
        : type(t) {}
};

class CommandParser {
public:
    static Command parse(const string& input);

private:
    static vector<string> tokenize(const string& input);
    static CommandType get_command_type(const string& cmd);
};


class CLI {
public:
    CLI();
    ~CLI();
    void run();
    void execute_command(const Command& cmd);

private:
};

#endif
