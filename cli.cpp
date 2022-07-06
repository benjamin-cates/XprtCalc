#include "src/_header.hpp"
#include <iostream>
std::map<char, string> AnsiColors = {
    {Expression::hl_argument,"36"},
    {Expression::hl_delimiter,"37"},
    {Expression::hl_operator,"37"},
    {Expression::hl_error,"31"},
    {Expression::hl_bracket,"0"},
    {Expression::hl_comment,"32"},
    {Expression::hl_function,"93"},
    {Expression::hl_null,"0"},
    {Expression::hl_numeral,"0"},
    {Expression::hl_space,"0"},
    {Expression::hl_string,"91"},
    {Expression::hl_text,"0"},
    {Expression::hl_unit,"36"},
    {Expression::hl_variable,"95"}
};
void printColoredString(const ColoredString& str) {
    const string& text = str.getStr();
    const string& colors = str.getColor();
    char prevColor = Expression::hl_space;
    for(int i = 0;i < text.length();i++) {
        if(prevColor != colors[i])
            std::cout << "\033[" << AnsiColors[colors[i]] << "m";
        std::cout << text[i];
    }
    std::cout << "\033[0m";
}

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
                printColoredString(Program::runCommand(input.substr(commandPrefix.size())));
                std::cout << std::endl;
                continue;
            }
            //Parse and compute tree
            ValPtr tr = Tree::parseTree(input, Program::parseCtx);
            ValPtr a = tr->compute(Program::computeCtx);
            ColoredString toPrint("$" + std::to_string(i), Expression::hl_variable);
            toPrint += ColoredString(" = ", Expression::hl_operator);
            toPrint += ColoredString::fromXpr(a->toString());
            printColoredString(toPrint);
            std::cout << std::endl;
            i++;
        }
        catch(string e) {
            printColoredString(ColoredString("Error: ", Expression::hl_error));
            std::cout << e << std::endl;
        }
        catch(const char* e) {
            printColoredString(ColoredString("Error: ", Expression::hl_error));
            std::cout << e << std::endl;
        }
        catch(...) {
            std::cout << "Unknown error type" << std::endl;
            throw;
        }
    }
    return 0;
}