#include "_header.hpp"
#pragma region Startup and cleanup
void Program::startup() {
    buildFunctionNameMap();
    Help::addPageData();
    Value::zero = std::make_shared<Number>(0);
    Value::one = std::make_shared<Number>(1);
    if(implementationStartup) implementationStartup();
}
void Program::cleanup() {
    if(implementationCleanup) implementationCleanup();
}
void (*Program::implementationStartup)() = 0;
void (*Program::implementationCleanup)() = 0;
#pragma endregion
#pragma region Global variables
bool Program::smallCompute = false;
ValList Program::history;
ComputeCtx Program::computeCtx;
ParseCtx Program::parseCtx;
Value Value::zero;
Value Value::one;
std::map<string, int> Program::globalFunctionMap;
int Program::getGlobal(const string& name) {
    if(globalFunctionMap.find(name) == globalFunctionMap.end()) return -1;
    else return globalFunctionMap.at(name);
}
#pragma endregion
#pragma region Preferences
std::map<string, std::pair<Value, void (*)(Value)>> Preferences::pref = {
    {"command_prefix",{std::make_shared<String>("/"),nullptr}}
};
Value Preferences::get(string name) {
    auto it = pref.find(name);
    if(it == pref.end()) throw "Preference " + name + " not found";
    else return it->second.first;
}
template<>
double Preferences::getAs<double>(string name) {
    Value val = get(name);
    return val->getR();
}
template<>
string Preferences::getAs<string>(string name) {
    Value val = get(name);
    if(val->typeID() == Value::str_t) {
        return val.cast<String>()->str;
    }
    else return val->toString();
}
void Preferences::set(string name, Value val) {
    //Run set command if it exists
    if(pref[name].second != nullptr) {
        (pref[name].second)(val);
    }
    pref[name].first = val;
}
#pragma endregion
#pragma region Library
using namespace Library;
ColoredString LibFunc::include() {
    if(Program::parseCtx.getVariable(name) != nullptr) return ColoredString("");
    ColoredString out;
    //Resolve dependencies
    for(int i = 0;i < dependencies.size();i++) {
        if(Library::functions.find(dependencies[i]) == Library::functions.end()) {
            out.append({ "Dependency ",{dependencies[i],'v'}," not found\n" });
        }
        else out += Library::functions[dependencies[i]].include();
    }
    Value lambda;
    try {
        lambda = Tree::parseTree(inputs + "=>" + xpr, Program::parseCtx);
    }
    catch(string mess) {
        return out.append({ {"Error",'e'}," in ",{name,'v'},": ",mess,"\n" });
    }
    catch(const char* mess) {
        return out.append({ {"Error",'e'}," in ",{name,'v'},": ",mess,"\n" });
    }
    catch(...) {
        return out.append({ {"Error",'e'}," in ",{name,'v'},": unknown\n" });
    }
    Program::parseCtx.pushVariable(name);
    Program::computeCtx.setVariable(name, lambda);
    out.append({ {name,'v'}," included\n" });
    return out;
}
Library::LibFunc::LibFunc(string n, string in, string fullN, string expression, std::vector<string> dependants) {
    name = n;
    inputs = in;
    fullName = fullN;
    xpr = expression;
    dependencies = dependants;
}
std::map<string, std::vector<string>> Library::categories = {
    {"algebra",{"solvequad"}},
    {"statistics",{"p","npr","ncr","mean","mean_geom","quartile1","quartile3","median","iqr","stddev","sample_stddev","prob_complement","correlation","binomial"}},
    {"vector",{"cross","dot","angle_between"}},
    {"complex",{"comp_argument","comp_complement","comp_magnitude"}},
    {"random",{"rand_int","rand_range","rand_member"}},
};
std::map<string, LibFunc> Library::functions = {
    {"solvequad",LibFunc("solvequad","(a,b,c)","Solve Quadratic",
        "(-b- <-1,1>*sqrt(b^2-4a*c))/2a",{})},
    {"cross",LibFunc("cross","(u,v)", "Cross product",
        "determinant(<<<1,0,0>,get(u,0),get(v,0)>,<<0,1,0>,get(u,1),get(v,2)>,<<0,0,1>,get(u,2),get(v,2)>>)",{"determinant"})},
    {"dot",LibFunc("dot","(u,v)", "Dot product",
        "sum(x=>get(u,x)*get(v,x),0,max(length(u),length(v)))",{})},
    {"angle_between",LibFunc("angle_between","(u,v)","Angle between vector",
        "acos(dot(u,v)/magnitude(u)/magnitude(v))",{"dot"})},
    {"comp_argument",LibFunc("comp_argument","(z)","Complex argument",
        "atan2(geti(z),getr(z))",{})},
    {"comp_complement",LibFunc("comp_complement","(z)","Complex complement",
        "getr(z)-geti(z)",{})},
    {"comp_magnitude",LibFunc("comp_magnitude","(z)","Complex magnitude",
        "sqrt(getr(z)^2+geti(z)^2)",{})},
    {"comp_cis",LibFunc("comp_cis","(angle)","Cosine i sine",
        "cos(angle)+i*sin(angle)",{})},
    {"p",LibFunc("p","(event,count)","Probability",
        "sum(_=>event()!=0,0,count)/count",{})},
    {"npr",LibFunc("npr","(n,r)","Permutations",
        "factorial(n)/factorial(n-r)",{})},
    {"ncr",LibFunc("ncr","(n,r)","Choose",
        "factorial(n)/factorial(n-r)/factorial(r)",{})},
    {"mean",LibFunc("mean","(data)","Mean",
        "sum(x=>data[x],0,length(data))/length(data)",{})},
    {"mean_geom",LibFunc("mean_geom","(data)","Geometric mean",
        "product(x=>get(data,x),0,length(data)-1)^(1/length(data)",{})},
    //{"quartile1",LibFunc("quartile1","(data)","First quartile",
    //    "get(sort(data),floor((length(data)+1/4))",{})},
    //{"quartile3",LibFunc("quartile3","(data)","Third quartile",
    //    "get(sort(data),floor(3*length(data)/4))",{})},
    //{"median",LibFunc("median","(data)","Median",
    //    "get(sort(data),floor(length(data)/2))",{})},
    //{"iqr",LibFunc("iqr","(data)","Interquartile range",
    //    "quartile3(data)-quartile1(data)",{"quartile1","quartile3"})},
    {"stddev",LibFunc("stddev","(data)","Standard deviation",
        "run(u => sqrt(sum(x => (data[x] - u) ^ 2, 0, length(data)-1) / (length(data)-1)), mean(data))",{"mean"})},
    {"pop_stddev",LibFunc("pop_stddev","(data)","Population standard deviation",
        "stddev(data)*sqrt(length(data)-1)/sqrt(length(data))",{"stddev"})},
    {"prob_complement",LibFunc("prob_complement","(p)","Probability complement",
        "1-p",{})},
    //Not working rinht now
    //{"covariance",LibFunc("covariance","(datax,datay)","Covariance",
    //    "run((ux,uy,count)=>sum(i=>(datax[i]-ux)*(datay[i]-uy),0,count-1)/(count-1),mean(datax),mean(datay),max(length(datax),length(datay)))",{"mean"})},
    {"correlation",LibFunc("correlation","(datax,datay)","Correlation coefficient",
        "covariance(datax,datay)/stddev(datax)/stddev(datay)",{"covariance","stddev"})},
    {"binomial",LibFunc("binomial","(p,n,x)","Binomial distribution",
        "ncr(n,x)*p^x*(1-p)^(n-x)",{"ncr"})},
    {"rand_int",LibFunc("rand_int","(min,max)","Random integer",
        "floor(rand()*(max-min)+min)",{})},
    {"rand_member",LibFunc("rand_member","(data)","Random member",
        "get(data,floor(rand*length(data)))",{})},
    {"rand_range",LibFunc("rand_range","(min,max)","Random within range",
        "rand()*(max-min)+min",{})},


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
ColoredString Program::runCommand(string call) {
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
    ColoredString out = Program::commandList[name].run(argsList);
    if(out.getStr().back() == '\n') out.splice(0, out.length() - 1);
    return out;
}
ColoredString Program::runLine(string str) {
    string commandPrefix = Preferences::getAs<string>("command_prefix");
    try {
        //Commands
        if(str.substr(0, commandPrefix.size()) == commandPrefix) {
            return Program::runCommand(str.substr(commandPrefix.size()));
        }
        //Parsing assignment statements
        std::tuple<string, ValList, string> assign = Expression::parseAssignment(str);
        if(std::get<0>(assign) != "") {
            string& name = std::get<0>(assign);
            if(!Program::parseCtx.variableExists(name)) {
                Program::parseCtx.pushVariable(name);
                Program::computeCtx.defineVariable(name, Value::zero);
            }
            Value& var = Program::computeCtx.variables[name].back();
            Value set = Expression::evaluate(std::get<2>(assign));
            Value::set(var, std::get<1>(assign), set);
            ColoredString out(name, Expression::hl_variable);
            out += ColoredString(" = ", " o ");
            out += ColoredString::fromXpr(var->toString());
            return out;
        }
        //Parse and compute tree
        Value tr = Tree::parseTree(str, Program::parseCtx);
        Value a = tr->compute(Program::computeCtx);
        ColoredString toPrint("$" + std::to_string(history.size()), Expression::hl_variable);
        toPrint += ColoredString(" = ", Expression::hl_operator);
        toPrint += ColoredString::fromXpr(a->toString());
        Program::history.push_back(a);
        return toPrint;
    }
    catch(string e) {
        ColoredString out("Error: ", Expression::hl_error); out += e; return out;
    }
    catch(const char* e) {
        ColoredString out("Error: ", Expression::hl_error); out += e; return out;
    }
    catch(ColoredString e) {
        ColoredString out("Error: ", Expression::hl_error); out += e; return out;
    }
    catch(...) {
        ColoredString out("Error: ", Expression::hl_error); out += "unknown error"; return out;
    }

}
ColoredString command_include(std::vector<string>& input) {
    ColoredString out;
    for(int i = 0;i < input.size();i++) {
        if(Library::categories.find(input[i]) != Library::categories.end()) {
            vector<string> libs = Library::categories[input[i]];
            out += command_include(libs);
        }
        else if(Library::functions.find(input[i]) != Library::functions.end()) {
            out += Library::functions[input[i]].include();
        }
        else out.append({ {"Error: ",'e'},{input[i],'v'}," not found\n" });
    }
    return out;
}
string command_sections_internal(const string& inp, string tabbing) {
    using sec = Expression::Section;
    string out;
    ParseCtx ctx;
    string newTab = tabbing;
    newTab += "    ";
    std::vector<std::pair<string, sec>> sections = Expression::getSections(inp, ctx);
    for(int i = 0;i < sections.size();i++) {
        sec type = sections[i].second;
        string& str = sections[i].first;
        char bracket = 0;
        if(type == sec::function) bracket = '(';
        else if(type == sec::parenthesis) bracket = '(';
        else if(type == sec::lambda) {
            out += tabbing + str.substr(0, Expression::findNext(str, 0, '>') + 1) + '\n';
            out += command_sections_internal(str.substr(Expression::findNext(str, 0, '>') + 1), newTab);
            continue;
        }
        else if(type == sec::square) bracket = '[';
        else if(type == sec::squareWithBase) bracket = '[';
        else if(type == sec::vect) bracket = '<';
        else if(type == sec::curly) bracket = '{';
        if(bracket) {
            int start = Expression::findNext(str, 0, bracket);
            int end = Expression::matchBracket(str, start);
            if(end == -1) end = str.length();
            out += tabbing + str.substr(0, start + 1) + '\n';
            if(type == sec::vect || type == sec::curly || type == sec::function) {
                std::vector<string> secs = Expression::splitBy(str, start, end, ',');
                for(int i = 0;i < secs.size();i++) {
                    out += command_sections_internal(secs[i], tabbing + "    ");
                    if(i != secs.size() - 1) out += tabbing + "    ,\n";
                }
            }
            else {
                out += command_sections_internal(str.substr(start + 1, end - start - 1), newTab);
            }
            out += tabbing + str.substr(end) + '\n';
        }
        else out += tabbing + str + '\n';
    }
    return out;
}
ColoredString command_sections(std::vector<string>& input) {
    string inp = combineArgs(input);
    return ColoredString(command_sections_internal(inp, ""));
}
ColoredString command_parse(vector<string>& input) {
    string inp = combineArgs(input);
    ParseCtx ctx;
    Value tr = Tree::parseTree(inp, ctx);
    return ColoredString::fromXpr(tr->toString());
}
ColoredString command_meta(vector<string>& input) {
    string out;
    for(auto it = Metadata::info.begin();it != Metadata::info.end();it++)
        out += it->first + ": " + it->second + '\n';
    return { out };
}
ColoredString command_def(vector<string>& input) {
    string inp = combineArgs(input);
    std::tuple<string, ValList, string> assign = Expression::parseAssignment(inp);
    if(std::get<0>(assign) == "") throw "not an assignment";
    return Program::runLine(inp);
}
ColoredString command_pref(vector<string>& input) {
    string inp = combineArgs(input);
    std::tuple<string, ValList, string> assign = Expression::parseAssignment(inp);
    string& name = std::get<0>(assign);
    //Just display
    if(name == "") {
        name = inp;
        Expression::removeSpaces(name);
        if(Preferences::pref.find(name) == Preferences::pref.end()) throw "preference " + name + " not found";
    }
    else {
        if(Preferences::pref.find(name) == Preferences::pref.end()) throw "preference " + name + " not found";
        Value& val = Preferences::pref[name].first;
        Value set=Expression::evaluate(std::get<2>(assign));
        Value::set(val, std::get<1>(assign), set);
        if(Preferences::pref[name].second) Preferences::pref[name].second(Preferences::pref[name].first);
    }
    ColoredString out(name, Expression::hl_function);
    out.append({ {" = "," o "},ColoredString::fromXpr(Preferences::pref[name].first->toString()) });
    return out;
}
ColoredString command_ls(vector<string>& input) {
    auto& vars = Program::computeCtx.variables;
    ColoredString out;
    for(auto it = vars.begin();it != vars.end();it++) {
        out.append({ {it->first,'v'},{" = "," o "},ColoredString::fromXpr(it->second.back()->toString()),"\n" });
    }
    return out;
}
ColoredString command_highlight(vector<string>& input) {
    string inp = combineArgs(input);
    return { Expression::colorLine(inp,Program::parseCtx) };
}
ColoredString command_help(vector<string>& input) {
    string inp = combineArgs(input);
    if(inp.length() == 0) inp = "welcome";
    std::vector<Help::Page*> res = Help::search(inp, 1);
    if(res.size() == 0) throw "Help page not found";
    Help::Page& p = *res[0];
    return res[0]->toColoredString();
}
ColoredString command_query(vector<string>& input) {
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
ColoredString command_debug_help(vector<string>& input) {
    if(Help::queryHash.size() == 0) Help::generateQueryHash();
    string out;
    for(auto it = Help::queryHash.begin();it != Help::queryHash.end();it++) {
        uint64_t hash = it->first;
        string name(8, ' ');
        for(int x = 0;(hash & (uint64_t(0xff) << ((7 - x) * 8))) && x < 8;x++) {
            name[x] = char((hash >> ((7 - x) * 8)) & 0xff);
        }
        out += name + ": ";
        for(int x = 0;x < it->second.size();x++) {
            if(x != 0) out += ", ";
            out += Help::pages[it->second[x].first].name;
            out += "(" + std::to_string(it->second[x].second) + ")";
        }
        out += "\n";
    }
    return out;
}
map<string, Command> Program::commandList = {
    {"include",{&command_include}},
    {"sections",{&command_sections}},
    {"parse",{&command_parse}},
    {"meta",{&command_meta}},
    {"def",{&command_def}},
    {"ls",{&command_ls}},
    {"highlight",{&command_highlight}},
    {"help",{&command_help}},
    {"query",{&command_query}},
    {"debug_help",{&command_debug_help}},
    {"pref",{&command_pref}},
};
#pragma endregion