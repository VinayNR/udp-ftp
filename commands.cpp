#include "commands.h"
#include <string>
#include <vector>
#include <iostream>

const char * help_command = "help";
const char * ls_command = "ls";
const char * exit_command = "exit";

const char * get_command = "get";
const char * put_command = "put";
const char * delete_command = "delete";

const std::vector<std::string>& getListOfCommands() {
    static const std::vector<std::string> allowedCommands;
    return allowedCommands;
}

bool isCommandAllowed(char * command) {
    vector<string> allowedCommands = getListOfCommands();
    for (const string & x : allowedCommands) {
        if (x == command) {
            return true;
        }
    }
    return false;
}