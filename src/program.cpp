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
ComputeCtx Program::computeCtx;
ParseCtx Program::parseCtx;
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
string LibFunc::include() {
    if(Program::parseCtx.getVariable(name) != nullptr) return "";
    string out;
    //Resolve dependencies
    for(int i = 0;i < dependencies.size();i++) {
        if(Library::functions.find(dependencies[i]) == Library::functions.end())
            out += "Dependency " + dependencies[i] + " not found\n";
        else out += Library::functions[dependencies[i]].include();
    }
    ValPtr lambda;
    try {
        lambda = Tree::parseTree(inputs + "=>" + xpr, Program::parseCtx);
    }
    catch(string mess) { return out + "Error in " + name + ": " + mess + '\n'; }
    catch(const char* mess) { return out + "Error in " + name + ": " + mess + '\n'; }
    catch(...) { return out + "Error in " + name + '\n'; }
    Program::parseCtx.pushVariable(name);
    Program::computeCtx.setVariable(name, lambda);
    return out + name + " included\n";
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
        "<-b-sqrt(b^2-4ac),-b+sqrt(b^2-4ac)>/2a",{})},
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
        "sum(x=>get(data,x),0,length(data))/length(data)",{})},
    {"mean_geom",LibFunc("mean_geom","(data)","Geometric mean",
        "product(x=>get(data,x),0,length(data))^(1/length(data)",{})},
    {"quartile1",LibFunc("quartile1","(data)","First quartile",
        "get(sort(data),floor(length(data)/4))",{})},
    {"quartile3",LibFunc("quartile3","(data)","Third quartile",
        "get(sort(data),floor(3*length(data)/4))",{})},
    {"median",LibFunc("median","(data)","Median",
        "get(sort(data),floor(length(data)/2))",{})},
    {"iqr",LibFunc("iqr","(data)","Interquartile range",
        "quartile3(data)-quartile1(data)",{"quartile1","quartile3"})},
    {"stddev",LibFunc("stddev","(data)","Standard deviation",
        "run(mean=>sqrt(sum(x=>get(data,x)-mean,0,length(data))/length(data)),mean(data))",{"mean"})},
    {"sample_stddev",LibFunc("sample_stddev","(data)","Sample standard deviation",
        "stddev(data)*sqrt(length(data))/sqrt(length(data)-1)",{"stddev"})},
    {"prob_complement",LibFunc("prob_complement","(p)","Probability complement",
        "1-p",{})},
    {"correlation",LibFunc("correlation","(data1,data2)","Correlation coefficient",
        "run((mean1,mean2,count)=>sum(x=>(get(data1,x)-mean1)*(get(data2,x)-mean2),0,count)/sqrt(sum(x=>(get(data1,x)-mean1)^2,0,count)*sum(x=>(get(data2,x)-mean2)^2,0,count)),mean(data1),mean(data2),max(length(data1),length(data2)))",{"mean"})},
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
    string out;
    for(int i = 0;i < input.size();i++) {
        if(Library::categories.find(input[i]) != Library::categories.end()) {
            vector<string> libs = Library::categories[input[i]];
            out += command_include(libs);
        }
        else if(Library::functions.find(input[i]) != Library::functions.end()) {
            out += Library::functions[input[i]].include();
        }
    }
    return out;
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
string command_def(vector<string>& input) {
    //If is in the form a=2
    char eq;
    if((eq = input[0].find('=')) != string::npos) {
        input.resize(2);
        input[1] = input[0].substr(eq + 1);
        input[0] = input[0].substr(0, eq);
    }
    //Push variable to compute context
    ValPtr value = Expression::evaluate(input[1]);
    //Push variable to respective contexts
    Program::parseCtx.pushVariable(input[0]);
    Program::computeCtx.setVariable(input[0], value);
    return input[0] + "=" + value->toString();
}
string command_ls(vector<string>& input) {
    auto& vars = Program::computeCtx.variables;
    string out;
    for(auto it = vars.begin();it != vars.end();it++) {
        out += it->first + " = " + it->second.back()->toString() + '\n';
    }
    return out;

}
map<string, Command> Program::commandList = {
    {"include",{{"literal"},{"solvequad"},false,&command_include}},
    {"sections",{{"expression"},{"xpr"},false,&command_sections}},
    {"parse",{{"expression"},{"xpr"},true,&command_parse}},
    {"meta",{{},{},false,&command_meta}},
    {"def",{{"literal","expression"},{"name","xpr"},true,&command_def}},
    {"ls",{{},{},true,&command_ls}},
};