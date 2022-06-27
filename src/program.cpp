#include "_header.hpp"

void Program::startup() {
    buildFunctionNameMap();
    Value::zero = std::make_shared<Number>(0);
    if(implementationStartup) implementationStartup();
}
void Program::cleanup() {
    if(implementationCleanup) implementationCleanup();

}
void (*Program::implementationStartup)() = 0;
void (*Program::implementationCleanup)() = 0;
ValList Program::history;
ValPtr Value::zero;
std::unordered_map<string, int> Program::globalFunctionMap;
int Program::getGlobal(const string& name) {
    if(globalFunctionMap.find(name) == globalFunctionMap.end()) return -1;
    else return globalFunctionMap.at(name);
}

#pragma region Preferences
std::map<string, std::pair<ValPtr, void (*)(ValPtr)>> Preferences::pref = {
    {"command_prefix",{std::make_shared<String>("-"),nullptr}}
};
ValPtr Preferences::get(string name) {
    auto it = pref.find(name);
    if(it == pref.end()) throw "Preference " + name + " not found";
    else return it->second.first;
}
template<>
double Preferences::getAs<double>(string name) {
    ValPtr val = get(name);
    return val->getR();
}
template<>
string Preferences::getAs<string>(string name) {
    ValPtr val = get(name);
    if(val->typeID() == Value::str_t) {
        return std::static_pointer_cast<String>(val)->str;
    }
    else return val->toString();
}
void Preferences::set(string name, ValPtr val) {
    //Run set command if it exists
    if(pref[name].second != nullptr) {
        (pref[name].second)(val);
    }
    pref[name].first = val;
}
#pragma endregion
#pragma region Library
using namespace Library;
bool LibFunc::include() {
    if(Program::globalFunctionMap[name] != 0) return false;
    ParseCtx ctx;
    ctx.push(inputs);
    ValPtr tree = Tree::parseTree(xpr, ctx);
    //Program::globalFunctions.push_back(Function(name, inputs, tree));
    Program::globalFunctionMap[name] = Program::globalFunctions.size() - 1;
    return true;
}
void Library::includeAll(string type) {
    for(auto it = functions.begin();it != functions.end();it++)
        if(it->second.type == type) it->second.include();
}
LibFunc::LibFunc(string n, std::vector<string> in, string fullN, string t, string expression) {
    name = n;
    inputs = in;
    fullName = fullN;
    type = t;
    xpr = expression;
}
std::map<string, LibFunc> Library::functions = {
    {"solvequad",LibFunc("solvequad",{"a","b","c"},
        "Solve Quadratic","algebra",
        "<-b-sqrt(b^2-4ac),-b+srqt(b^2-4ac)>/2a")},

};
using namespace std;
#pragma endregion
#pragma region Commands
string combineArgs(std::vector<string>& args) {
    string out;
    if(args.size() == 0) return "";
    int i = 0;
    for(int i;i < args.size() - 1;i++) {
        out += args[i] + " ";
    }
    out += args[i];
    return out;
}
string Program::runCommand(string call) {
    size_t space = call.find(' ');
    if(space == string::npos) space = call.length();
    string name = call.substr(0, space);
    if(Program::commandList.find(name) == Program::commandList.end()) {
        throw "command " + name + " not found";
    }
    std::vector<string> argsList;
    if(space == call.length()) return Program::commandList[name].run(argsList);
    string args = call.substr(space + 1);
    int pos = 0;
    while(args[pos] != 0) {
        int next = Expression::findNext(args, pos, ' ');
        if(next == -1) {
            argsList.push_back(args.substr(pos));
            break;
        }
        argsList.push_back(args.substr(pos, next - pos));
        pos = next + 1;
    }
    return Program::commandList[name].run(argsList);
}
string command_include(std::vector<string>& input) {
    return "";
}
string command_sections(std::vector<string>& input) {
    using sec = Expression::Section;
    string out;
    if(input.size() == 1) input.push_back("");
    ParseCtx ctx;
    std::vector<std::pair<string, sec>> sections = Expression::getSections(input[0], ctx);
    for(int i = 0;i < sections.size();i++) {
        sec type = sections[i].second;
        string& str = sections[i].first;
        char bracket = 0;
        if(type == sec::function) bracket = '(';
        else if(type == sec::parenthesis) bracket = '(';
        else if(type == sec::lambda) {
            out += input[1] + str.substr(0, Expression::findNext(str, 0, '>') + 1) + '\n';
            std::vector<string> newInp{ str.substr(Expression::findNext(str,0,'>') + 1),input[0] + "  " };
            out += command_sections(newInp);
        }
        else if(type == sec::square) bracket = '[';
        else if(type == sec::squareUnit) bracket = '[';
        else if(type == sec::vect) bracket = '<';
        else if(type == sec::curly) bracket = '{';
        if(bracket) {
            int start = Expression::findNext(str, 0, bracket);
            int end = Expression::matchBracket(str, start);
            if(end == -1) end = str.length();
            out += input[1] + str.substr(0, start + 1) + '\n';
            if(type == sec::vect || type == sec::curly || type == sec::function) {
                std::vector<string> secs = Expression::splitBy(str, start, end, ',');
                string newTabbing = input[1] + "  ";
                for(int i = 0;i < secs.size();i++) {
                    std::vector<string> inp{ secs[i],newTabbing };
                    out += command_sections(inp);
                    if(i != secs.size() - 1) out += newTabbing + ",\n";
                }
            }
            else {
                std::vector<string> newInp{ str.substr(start + 1, end - start - 1), input[1] + "  " };
                out += command_sections(newInp);
            }
            out += input[1] + str.substr(end) + '\n';
        }
        else out += input[1] + str + '\n';
    }
    return out;
}

string command_parse(vector<string>& input) {
    string inp = combineArgs(input);
    ParseCtx ctx;
    ValPtr tr = Tree::parseTree(inp, ctx);
    return tr->toString();
}

string command_meta(vector<string>& input) {
    string out;
    for(auto it = Metadata::info.begin();it != Metadata::info.end();it++)
        out += it->first + ": " + it->second + '\n';
    return out;
}
map<string, Command> Program::commandList = {
    {"include",{{"literal"},{"solvequad"},false,&command_include}},
    {"sections",{{"expression"},{"xpr"},false,&command_sections}},
    {"parse",{{"expression"},{"xpr"},true,&command_parse}},
    {"meta",{{},{},false,&command_meta}},
};