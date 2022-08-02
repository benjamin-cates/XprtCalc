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
ColoredString command_help(std::vector<string>& input) {
    string inp = Command::combineArgs(input);
    if(inp.length() == 0) inp = "welcome";
    std::vector<Help::Page*> res = Help::search(inp, 1);
    if(res.size() == 0) throw "Help page not found";
    Help::Page& p = *res[0];
    return res[0]->toColoredString();
}
ColoredString command_query(std::vector<string>& input) {
    ColoredString out;
    string inp = input[0];
    //Print out results
    std::vector<Help::Page*> res = Help::search(inp, 10);
    for(int i = 0;i < res.size();i++) {
        out.append({ {std::to_string(i),'n'},{": ","o "},{res[i]->name,'v'},"\n" });
    }
    //Message to rerun with index
    if(input.size() == 1) {
        out += "Rerun query with search and index to print help page\n";
    }
    //If index is provided, print the page
    else {
        int index = Expression::evaluate(input[1])->getR();
        if(index < 0 || index >= res.size()) out.append({ {"Error: ",'e'},"Unable to print page, index out of bounds\n" });
        else return res[index]->toColoredString();
    }
    return out;
}

void startup() {
    Program::commandList.insert(std::pair<string, Command>{"quit", { &quitCommand }});
    Program::commandList.insert(std::pair<string, Command>{"help", { &command_help }});
    Program::commandList.insert(std::pair<string, Command>{"query", { &command_query }});
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