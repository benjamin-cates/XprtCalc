#include "_header.hpp"
#pragma region Parsing Context
ParseCtx::ParseCtx() {
    bases.push(10);
    useUnitsStack.push(false);
    argStack = std::deque<string>(0);
};
void ParseCtx::push(int base, bool forceUnits) {
    //Push base
    if(base == 0) bases.push(getBase());
    else bases.push(base);
    //Push units
    if(forceUnits) useUnitsStack.push(true);
    else useUnitsStack.push(useUnits());
    argStackSizes.push_back(0);
    localStackSizes.push_back(0);
}
void ParseCtx::push(const std::vector<string>& arguments) {
    bases.push(getBase());
    useUnitsStack.push(useUnits());
    //Push front backwards
    for(auto it = arguments.rbegin();it != arguments.rend();it++)
        argStack.push_front(*it);
    argStackSizes.push_back(arguments.size());
    localStackSizes.push_back(0);
}
void ParseCtx::pop() {
    bases.pop();
    useUnitsStack.pop();

    //Remove arguments
    int argRemoveCount = argStackSizes.back();
    for(int i = 0;i < argRemoveCount;i++) argStack.pop_front();
    argStackSizes.pop_back();
    //Remove local variables
    int localRemoveCount = localStackSizes.back();
    for(int i = 0;i < localRemoveCount;i++) localStack.pop_front();
    localStackSizes.pop_back();
}
bool ParseCtx::useUnits()const {
    return useUnitsStack.top();
}
int ParseCtx::getBase()const {
    return bases.top();
}
string ParseCtx::getArgName(int index)const {
    return *(argStack.begin() + index);
}
void ParseCtx::pushLocal(const string& name) {
    localStackSizes.front() += 1;
    localStack.push_front(name);
}
ValPtr ParseCtx::getVariable(const string& name)const {
    if(Program::globalFunctionMap.find(name) != Program::globalFunctionMap.end())
        return std::make_shared<Tree>(Program::globalFunctionMap[name]);
    //for(int i = 0;i < localStack.size();i++)
    //    if(name == localStack[i]) return Tree::Operator(Tree::tt_local, i);
    for(auto it = argStack.begin();it != argStack.end();it++) {
        if(name == *it) return std::make_shared<Argument>(std::distance(argStack.begin(), it));
    }
    if(useUnits()) {
        double coef = 1;
        Unit u = Unit::parseName(name, coef);
        if(!(u == Unit(0)))
            return std::make_shared<Number>(coef, 0, u);
    }
    return nullptr;
}
#pragma endregion
std::map<string, std::pair<string, int>> Expression::operatorList = {
    {"+",{"add",3}},
    {"-",{"sub",3}},
    {"*",{"mult",2}},
    {"/",{"div",2}},
    {"%",{"mod",2}},
    {"**",{"pow",1}},
    {"^",{"pow",1}},
    {"==",{"equal",4}},
    {"=",{"equal",4}},
    {"!=",{"not_equal",4}},
    {">",{"gt",4}},
    {">=",{"gt_equal",4}},
    {"<",{"lt",4}},
    {"<=",{"lt_equal",4}},
};
ValPtr Expression::parseNumeral(const string& str, int defaultBase) {
    const static std::unordered_map<char, int> bases = {
        {'b',2},{'t',3},{'o',8},{'d',10},{'x',16}
        #ifdef USE_ARB
        ,{'a',-1}
        #endif
    };
    int base = defaultBase;
    if(str[0] == '0') {
        auto it = bases.find(str[1]);
        if(it != bases.end()) base = it->second;
    #ifdef USE_ARB
        if(base == -1) {
            return std::make_shared<Arb>(mppp::real(str.substr(2), defaultBase));
        }
    #endif
        if(base == 0) base = defaultBase;
    }
    char maxAlphaChar = 'A' + base - 11;
    double out = 0;
    bool hasPeriod = false;
    int periodDivide = 0;
    for(int i = 0;i < str.size();i++) {
        int digit = -1;
        if(str[i] >= '0' && str[i] <= '9')
            digit = str[i] - '0';
        else if(str[i] >= 'A' && str[i] <= maxAlphaChar)
            digit = str[i] - 'A' + 10;
        if(digit != -1) {
            out *= base;
            out += digit;
            if(hasPeriod) periodDivide += 1;
        }
        else if(str[i] == 'e') {
            double exponent = parseNumeral(str.substr(i + 1), base)->getR();
            return std::make_shared<Number>(out * pow(double(base), exponent - periodDivide));
        }
        else if(str[i] == '.') hasPeriod = true;
    }
    return std::make_shared<Number>(out / pow(double(base), periodDivide));
}
int Expression::nextSection(const string& str, int start, Expression::Section* type, ParseCtx& ctx) {
    using namespace Expression;
    char ch = str[start];
    //Brackets and quotes
    if(ch == '(' || ch == '[' || ch == '<' || ch == '{' || ch == '"') {
        int match = matchBracket(str, start);
        if(type) *type = Section(bracs.at(ch));
        //<= operator
        if(bracs.at(ch) == 4 && str[start + 1] == '=') {
            if(type) *type = Section::operat;
            return start + 2;
        }
        if(match == -1) return str.length();
        else {
            // [ABC]_16 base notation
            if(ch == '[' && str[match + 1] == '_') {
                if(type) *type = Section::squareUnit;
                return nextSection(str, match + 2, nullptr, ctx);
            }
            //(a,b)=> lambda notation
            else if(ch == '(' && str[match + 1] == '=' && str[match + 2] == '>') {
                int i = match + 3;
                while(str[i] == ' ') i++;
                if(type) *type = Section::lambda;
                if(str[i] == '{') return nextSection(str, i, nullptr, ctx);
                return str.length();
            }
            return match + 1;
        }
    }
    //Numerals
    else if(ch >= '0' && ch <= '9') {
        //Deal with 0b and 0x... prefixes
        const static std::unordered_map<char, int> bases = {

            {'b',2},{'t',3},{'o',8},{'d',10},{'x',16}
            #ifdef USE_ARB
            ,{'a',-1}
            #endif
        };
        int base;
        if(ch == '0' && (bases.find(str[1]) != bases.end())) {
            start += 2;
            base = bases.at(str[1]);
        }
        else base = ctx.getBase();
        if(base == -1) base = ctx.getBase();
        char maxAlpha = 'A' + base - 10;
        if(type) *type = Section::numeral;
        for(int i = start;i < str.length();i++) {
            char n = str[i];
            if((n < '0' || n>'9') && n != '_' && n != '.' && n != ' ' && n != 'e' && (n<'A' || n>maxAlpha)) return i;
        }
        return str.length();
    }
    //Variables
    else if((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '$' || ch == '_') {
        int i;
        for(i = start + 1;i < str.length();i++) {
            char c = str[i];
            if(c >= 'a' && c <= 'z') continue;
            else if(c >= 'A' && c <= 'Z') continue;
            else if(c >= '0' && c <= '9') continue;
            else if(c == '_' || c == '$') continue;
            else if(c == '(') {
                if(type) *type = Section::function;
                return nextSection(str, i, nullptr, ctx);
            }
            else if(c == '=' && str[i + 1] == '>') {
                i += 2;
                while(str[i] == ' ') i++;
                if(type) *type = Section::lambda;
                if(str[i] == '{') return nextSection(str, i, nullptr, ctx);
                return str.length();
            }
            else break;
        }
        if(type) *type = Section::variable;
        return i;
    }
    //Operators
    else if(ch == '=' || ch == '>' || ch == '+' || ch == '-' || ch == '*' || ch == '/' || ch == '%') {
        if(type) *type = Section::operat;
        int i = start + 1;
        char ch = str[i];
        while(ch == '=' || ch == '>' || ch == '+' || ch == '-' || ch == '*' || ch == '/' || ch == '%') i++, ch = str[i];
        return i;
    }
    if(type) *type = Section::undefined;
    return start + 1;
}
std::vector<std::pair<string, Expression::Section>> Expression::getSections(const string& str, ParseCtx& ctx) {
    std::vector<std::pair<string, Expression::Section>> out;
    for(int i = 0;i < str.length();i++) {
        if(str[i] == ' ') continue;
        Expression::Section type;
        int end = Expression::nextSection(str, i, &type, ctx);
        out.push_back(std::pair<string, Expression::Section>(str.substr(i, end - i), type));
        i = end - 1;
    }
    return out;
}
const std::unordered_map<char, int> Expression::bracs = {
    {'(',1}, {')',1}, {'[',2}, {']',2}, {'{',3}, {'}',3}, {'<',4}, {'>',4}, {'\"',5}
};
const std::unordered_map<char, bool> Expression::isStart = {
    {'(',true}, {')',false}, {'[',true}, {']',false}, {'{',true}, {'}',false}, {'<',true}, {'>',false}, {'\"',true}
};
//Returns the index of the matching bracket or -1 if it is not found
int Expression::matchBracket(const string& str, int start) {
    //Maps brackets to their unique match id
    //Brack stack stores the list of brackets types in order
    std::deque<int> bracStack{ bracs.at(str[start]) };
    for(int i = start + 1;i < str.length();i++) {
        char ch = str[i];
        auto typeIt = bracs.find(ch);
        if(typeIt != bracs.end()) {
            int type = typeIt->second;
            //Prevent >=, <=, and => from tripping the angle brackets up
            if(type == 4) {
                if(str[i + 1] == '=') continue;
                if(str[i] == '>' && i != 0 && str[i - 1] == '=') continue;
            }
            //Quote marks treated differently
            if(type == 5) {
                //Return if trying to match quotes
                if(bracStack.front() == 5) return i;
                //Iterate to end of string and continue program, this is to prevent  ("   ) " breaking the string because the first ending one has priority
                bool backSlash = false;
                for(i = i + 1;i < str.length();i++) {
                    if(str[i] == '"' && !backSlash) break;
                    if(str[i] == '\\') backSlash = !backSlash;
                    else backSlash = false;
                }
                continue;
            }
            if(isStart.at(ch)) bracStack.push_back(type);
            else {
                //Iterate backwards until matching type is found, for example if stack is [<{<( and type is }, it will iterate backwards to { and destroy the <( sequence from the list
                int destroyCount = 1;
                for(auto it = bracStack.rbegin();it != bracStack.rend();it++, destroyCount++)
                    if((*it) == type) break;
                //Return if they match
                if(destroyCount == bracStack.size()) return i;
                //Return not found if a closing bracket cuts it off
                if(destroyCount > bracStack.size()) return -1;
                //remove destroyed brackets
                for(int i = 0;i < destroyCount;i++) bracStack.pop_back();
            }
        }
    }
    //Return -1 when not found
    return -1;
}
int Expression::findNext(const string& str, int index, char find) {
    for(int i = index;i < str.length();i++) {
        if(str[i] == find) return i;
        if(str[i] == '(' || str[i] == '[' || str[i] == '{' || (str[i] == '<' && str[i + 1] != '=') || str[i] == '"') {
            i = matchBracket(str, i);
            if(i == -1) return -1;
        }
    }
    return -1;
}
string Expression::removeSpaces(const string& str) {
    int offset = 0;
    string out(str.length(), 0);
    for(int i = 0;i < str.length();i++) {
        if(str[i] == ' ') {
            offset++;
            continue;
        }
        out[i - offset] = str[i];
    }
    return out;
}
std::vector<string> Expression::splitBy(const string& str, int start, int end, char delimiter) {
    //Get comma positions
    std::vector<int> commas = { start };
    while(commas.back() != -1)
        commas.push_back(findNext(str, commas.back() + 1, delimiter));
    commas.back() = end;
    //Turn to string vector
    std::vector<string> out;
    for(int i = 0;i < commas.size() - 1;i++) {
        out.push_back(str.substr(commas[i] + 1, commas[i + 1] - commas[i] - 1));
    }
    //Prevent () from returning one argument
    if(out.size() == 1 && Expression::removeSpaces(out[0]) == "")
        return std::vector<string>(0);
    return out;
}
ValPtr Expression::evaluate(const string& str) {
    ParseCtx pctx;
    ValPtr tr = Tree::parseTree(str, pctx);
    ComputeCtx cctx;
    return tr->compute(cctx);
}
//THE TREE PARSER
//
//
//
//
//
ValPtr Tree::parseTree(const string& str, ParseCtx& ctx) {
    std::vector<std::pair<string, Expression::Section>> sections = Expression::getSections(str, ctx);
    ValList treeList;
    struct operatorData {
        string name;
        int priority;
        bool negative;
        operatorData(std::pair<string, int> d, int i, bool neg) { name = d.first;priority = d.second; negative = neg; }
        operatorData() { priority = 400; }
    };
    std::vector<operatorData> operators;
    std::vector<string> unaryOpFront;
    std::vector<string> unaryOpBack;
    //Parse each section individually
    for(int j = 0;j < sections.size();j++) {
        Expression::Section type = sections[j].second;
        string& str = sections[j].first;
        //Parenthesis ()
        if(type == Expression::parenthesis)
            treeList.push_back(Tree::parseTree(str.substr(1, str.length() - 2), ctx));
        //Square brackets for units []
        else if(type == Expression::square) {
            ctx.push(0, true);
            treeList.push_back(Tree::parseTree(str.substr(1, str.length() - 2), ctx));
            ctx.pop();
        }
        //Vectors <>
        else if(type == Expression::vect) {
            //Split elements by commas
            std::vector<string> comp = Expression::splitBy(str, 0, str.length() - 1, ',');
            ValList vectorComponents;
            for(int i = 0;i < comp.size();i++) {
                vectorComponents.push_back(Tree::parseTree(comp[i], ctx));
            }
            treeList.push_back(std::make_shared<Vector>(0));
            std::static_pointer_cast<Vector>(treeList.back())->vec = std::move(vectorComponents);
        }
        //Strings ""
        else if(type == Expression::quote) {
            string out;
            int offset = 1;
            bool backslash = false;
            for(int i = 1;i < str.size() - 1;i++) {
                if(!backslash) out += str[i];
                if(str[i] == '\\') backslash = !backslash;
                else if(backslash) {
                    if(str[i] == 'n') out += '\n';
                    else if(str[i] == 't') out += '\t';
                    else out += str[i];
                    backslash = false;
                }
            }
            treeList.push_back(std::make_shared<String>(out));
        }
        //Square bracket with unit [0A]_12
        else if(type == Expression::squareUnit) {
            int endBracket = Expression::matchBracket(str, 0);
            int underscore = Expression::findNext(str, endBracket, '_');
            int base = Expression::evaluate(str.substr(underscore + 1))->getR();
            if(base >= 36) throw "base cannot be over 36";
            if(base <= 1) throw "base cannot be under 2";
            ctx.push(base, true);
            treeList.push_back(Tree::parseTree(str.substr(1, endBracket - 2), ctx));
            ctx.pop();
        }
        //Numeral 0.442e1
        else if(type == Expression::numeral) {
            treeList.push_back(Expression::parseNumeral(str, ctx.getBase()));
        }
        //Variables
        else if(type == Expression::variable) {
            str = Expression::removeSpaces(str);
            ValPtr op = ctx.getVariable(str);
            if(op.get() == nullptr) throw "Variable " + str + " not found";
            if(op->typeID() == Value::tre_t) {
                int id = std::static_pointer_cast<Tree>(op)->op;
                if(Program::assertArgCount(id, 0) != true)
                    throw "Variable " + str + " cannot run without arguments";
                treeList.push_back(op);
            }
            else if(op->typeID() == Value::arg_t) {
                treeList.push_back(op);
            }
            else { throw "Variable " + str + " not found"; }
        }
        //Functions func(a)
        else if(type == Expression::function) {
            int startBrace = Expression::findNext(str, 0, '(');
            int endBrace = Expression::matchBracket(str, startBrace);
            string name = str.substr(0, startBrace);
            name = Expression::removeSpaces(name);
            ValPtr op = ctx.getVariable(name);
            if(op.get() == nullptr) throw "Function " + name + " not found";
            std::vector<string> argsStr = Expression::splitBy(str, startBrace, endBrace, ',');
            ValList arguments;
            for(int i = 0;i < argsStr.size();i++)
                arguments.push_back(Tree::parseTree(argsStr[i], ctx));
            if(op->typeID() == Value::tre_t) {
                std::shared_ptr<Tree> tr = std::static_pointer_cast<Tree>(op);
                if(Program::assertArgCount(tr->op, argsStr.size()) != true)
                    throw "Function '" + name + "' cannot run with " + std::to_string(argsStr.size()) + " arguments";
                tr->branches = std::move(arguments);
                treeList.push_back(op);
            }
            if(op->typeID() == Value::arg_t) {
                if(arguments.size() != 0) {
                    arguments.emplace(arguments.begin(), op);
                    treeList.push_back(std::make_shared<Tree>("run", std::move(arguments)));
                }
                else treeList.push_back(op);
            }
        }
        //Lambda a=>a^2
        else if(type == Expression::lambda) {
            std::vector<string> arguments;
            int arrow;
            if(str[0] == '(') {
                int endBrace = Expression::matchBracket(str, 0);
                arguments = Expression::splitBy(str, 0, endBrace, ',');
                arrow = str.find('>', endBrace + 1);
            }
            else {
                arrow = str.find('>');
                arguments.push_back(str.substr(0, arrow - 1));
            }
            ctx.push(arguments);
            ValPtr tr = Tree::parseTree(str.substr(arrow + 1), ctx);
            ctx.pop();
            treeList.push_back(std::make_shared<Lambda>(arguments, tr));
        }
        //Operators +-
        else if(type == Expression::operat) {
            str = Expression::removeSpaces(str);
            bool neg = (str.back() == '-' && str.length() != 1);
            if(neg) str = str.substr(0, str.length() - 1);
            auto op = Expression::operatorList.find(str);
            if(op == Expression::operatorList.end()) {
                throw "Operator " + str + " not found";
            }
            if(j == 0 && str == "-") { unaryOpFront.push_back("neg");continue; }
            operators.resize(treeList.size());
            operators[treeList.size() - 1] = operatorData(op->second, treeList.size() - 1, neg);
        }
    }
    for(int i = 0;i < unaryOpFront.size();i++) if(unaryOpFront[i] != "")
        treeList[i] = std::make_shared<Tree>(unaryOpFront[i], ValList{ treeList[i] });
    for(int i = 0;i < unaryOpBack.size();i++) if(unaryOpBack[i] != "")
        treeList[i] = std::make_shared<Tree>(unaryOpBack[i], ValList{ treeList[i] });
    operators.resize(treeList.size());
    //Multiply adjacents when there is no operator
    for(int i = 0;i < operators.size();i++)
        if(operators[i].name == "") operators[i] = operatorData(std::pair<string, int>("mult", 2), i, false);
    //Combine operators and values into singular list
    typedef std::pair<ValPtr, operatorData*> valOp;
    //Push all but final value to data list
    std::list<valOp> data;
    for(int i = 0;i < treeList.size();i++)
        data.push_back(valOp(treeList[i], &operators[i]));
    //Sort pointers to valops by operator precedence
    std::list<std::list<valOp>::iterator> sortedByPrecedence;
    for(auto it = data.begin();it != std::prev(data.end());it++) sortedByPrecedence.push_back(it);
    sortedByPrecedence.sort([](std::list<valOp>::iterator a, std::list<valOp>::iterator b) {
        return a->second->priority < b->second->priority;
        });
    //Combine elements
    for(auto& it : sortedByPrecedence) {
        auto cur = it;
        auto next = ++it;
        next->first = std::make_shared<Tree>(cur->second->name, ValList{ cur->first,next->first });
        data.erase(cur);
    }
    if(data.size() != 1)
        throw "operator combination error";
    return data.front().first;
}