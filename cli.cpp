#include "src/_header.hpp"
#include <iostream>

int main(int argc, char** argv) {
    Program::startup();
    std::cout << "Hello world" << std::endl;
    int i = 0;
    string commandPrefix = Preferences::getAs<string>("command_prefix");
    while(true) {
        try {
            string input;
            std::getline(std::cin, input);
            //Commands
            if(input.substr(0, commandPrefix.size()) == commandPrefix) {
                std::cout << Program::runCommand(input.substr(commandPrefix.size())) << std::endl;
                continue;
            }
            //Parse and compute tree
            ValPtr tr = Tree::parseTree(input, Program::parseCtx);
            ValPtr a = tr->compute(Program::computeCtx);
            std::cout << "$" << i << " = " << a->toString() << std::endl;
            i++;
        }
        catch(string e) {
            std::cout << "Error: " << e << std::endl;
        }
    }
    return 0;
}