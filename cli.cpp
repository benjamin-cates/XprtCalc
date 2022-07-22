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
ColoredString quitCommand(std::vector<string>& args) {
    exit(0);
    return ColoredString("");
}

void startup() {
    Program::commandList.insert(std::pair<string, Command>{"quit", { &quitCommand }});

}

int main(int argc, char** argv) {
    Program::implementationStartup = &startup;
    Program::startup();
    int i = 0;
    string commandPrefix = Preferences::getAs<string>("command_prefix");
    while(true) {
        string input;
        std::getline(std::cin, input);
        ColoredString output = Program::runLine(input);
        printColoredString(output);
        std::cout << std::endl;
        i++;
    }
    return 0;
}