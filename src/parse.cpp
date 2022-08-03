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
}
void ParseCtx::push(const std::vector<string>& arguments) {
    bases.push(getBase());
    useUnitsStack.push(useUnits());
    //Push front backwards
    for(auto it = arguments.rbegin();it != arguments.rend();it++)
        argStack.push_front(*it);
    argStackSizes.push_back(arguments.size());
}
void ParseCtx::pop() {
    bases.pop();
    useUnitsStack.pop();

    //Remove arguments
    int argRemoveCount = argStackSizes.back();
    for(int i = 0;i < argRemoveCount;i++) argStack.pop_front();
    argStackSizes.pop_back();
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
void ParseCtx::pushVariable(const string& name) {
    auto it = variables.find(name);
    if(it == variables.end()) variables[name] = 1;
    else it->second++;
}
//Undefines variables
void ParseCtx::popVariables(const std::vector<string>& names) {
    for(int i = 0;i < names.size();i++) {
        auto it = variables.find(names[i]);
        if(it == variables.end()) continue;
        it->second--;
        if(it->second == 0) variables.erase(it);
    }
}
//Returns bool whether variable exists
bool ParseCtx::variableExists(const string& name)const {
    auto it = variables.find(name);
    if(it == variables.end()) return false;
    return it->second > 0;
}
Value ParseCtx::getVariable(const string& name)const {
    string san = Expression::sanitizeVariable(name);
    for(auto it = argStack.begin();it != argStack.end();it++) {
        if(san == *it) return std::make_shared<Argument>(std::distance(argStack.begin(), it));
    }
    if(variableExists(san)) {
        return std::make_shared<Variable>(san);
    }
    if(Program::globalFunctionMap.find(san) != Program::globalFunctionMap.end())
        return std::make_shared<Tree>(Program::globalFunctionMap[san]);
    if(useUnits()) {
        string unitName = Expression::removeSpaces(name);
        double coef = 1;
        Unit u = Unit::parseName(unitName, coef);
        if(u.getBits() != 0)
            return std::make_shared<Number>(coef, 0, u);
    }
    if(san[0] == '$') {
        try {
            int index = std::stoi(san.substr(1));
            return std::make_shared<Tree>("ans", ValList{ std::make_shared<Number>(index) });
        }
        catch(...) { return nullptr; }
    }
    return nullptr;
}
#pragma endregion
#pragma region Expression constants
const std::map<string, std::pair<string, int>> Expression::operatorList = {
    {"+",{"add",4}},
    {"-",{"sub",4}},
    {"*",{"mul",3}},
    {"/",{"div",3}},
    {"%",{"mod",3}},
    {"**",{"pow",1}},
    {"^",{"pow",1}},
    {"==",{"equal",5}},
    {"=",{"equal",5}},
    {"!=",{"not_equal",5}},
    {">",{"gt",5}},
    {">=",{"gt_equal",5}},
    {"<",{"lt",5}},
    {"<=",{"lt_equal",5}},
};
const std::map<char, string> Expression::prefixOperators = {
    {'-',"neg"},
};
const std::map<char, string> Expression::suffixOperators = {
    {'!',"factorial"},
};
std::unordered_set<char> Expression::operatorChars = {
    '+','-','*','/','%','^','=','!','>','<'
};
const std::unordered_map<char, int> Expression::basesPrefix = {
    {'b',2},{'t',3},{'o',8},{'d',10},{'x',16}
};
const std::unordered_map<char, int> Expression::bracs = {
    {'(',1}, {')',1}, {'[',2}, {']',2}, {'{',3}, {'}',3}, {'<',4}, {'>',4}, {'\"',5}
};
const std::unordered_map<char, bool> Expression::isStart = {
    {'(',true}, {')',false}, {'[',true}, {']',false}, {'{',true}, {'}',false}, {'<',true}, {'>',false}, {'\"',true}
};
#pragma endregion
#pragma region Basic parsing
//Returns the index of the matching bracket or -1 if it is not found
int Expression::matchBracket(const string& str, int start) {
    //Maps brackets to their unique match id
    //Brack stack stores the list of brackets types in order
    std::deque<int> bracStack{ bracs.at(str[start]) };
    //If trying to match quote marks
    if(bracStack.front() == 5) {
        for(int i = start + 1;i < str.length();i++) {
            if(str[i] == '"') return i;
            if(str[i] == '\\') i++;
        }
    }
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
                //Iterate to end of string and continue program, this is to prevent  ("   ) " breaking the string because the first ending one has priority
                int j = i + 1;
                for(;j < str.length();j++) {
                    if(str[j] == '"') break;
                    if(str[j] == '\\') j++;
                }
                i = j;
                continue;
            }
            if(isStart.at(ch)) bracStack.push_back(type);
            else {
                //Ignore less than and greater than signs
                if(type == 4 && bracStack.back() != 4) continue;
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
//Find index of "find" withing "str" starting at index. Ignores within brackets and quotes
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
//Removes spaces from a string
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
    out.resize(str.size() - offset);
    return out;
}
string Expression::sanitizeVariable(const string& str) {
    string out(' ', str.length());
    int offset = 0;
    for(int i = 0;i < str.size();i++) {
        if(str[i] == ' ') offset++;
        else if(str[i] >= 'A' && str[i] <= 'Z') out[i - offset] = str[i] - 'A' + 'a';
        else out[i - offset] = str[i];
    }
    out.resize(str.size() - offset);
    return out;
}
//Get a list of positions of delimiter, including start and end
std::vector<int> getDelimiterList(const string& str, int start, int end, char delimiter) {
    //Get positions
    std::vector<int> out = { start };
    while(out.back() != -1)
        out.push_back(Expression::findNext(str, out.back() + 1, delimiter));
    //Add end and return
    out.back() = end;
    return out;
}
std::vector<string> Expression::splitBy(const string& str, int start, int end, char delimiter) {
    std::vector<int> indicies = getDelimiterList(str, start, end, delimiter);
    //Turn to string vector
    std::vector<string> out;
    for(int i = 0;i < indicies.size() - 1;i++) {
        out.push_back(str.substr(indicies[i] + 1, indicies[i + 1] - indicies[i] - 1));
    }
    //Prevent () from returning one argument
    if(out.size() == 1 && Expression::removeSpaces(out[0]) == "")
        return std::vector<string>(0);
    return out;
}
#pragma endregion
#pragma region Complicated parsing
Value Expression::parseNumeral(const string& str, int base) {
    //Parse base
    int start = 0;
    if(str[0] == '0') {
        auto it = basesPrefix.find(str[1]);
        if(it != basesPrefix.end()) {
            base = it->second;
            start = 2;
        }
    }
    //Parse precision
    int pPos;
    long long precision = 15;
    if((pPos = str.find('p')) != string::npos) {
        string prec = str.substr(pPos + 1);
        try { precision = std::stoll(prec, nullptr, base); }
        catch(const std::invalid_argument& ia) { throw "Invalid precision " + prec; }
        if(Program::smallCompute) precision = std::min(precision, 20LL);
    }
    else pPos = str.length();
    //Parse exponent
    int ePos;
    double exponent = 0;
    if((ePos = str.find('e')) != string::npos) {
        if(ePos > pPos) throw "Exponent must be specified before precision";
        string exp = str.substr(ePos + 1, pPos - ePos - 1);
        exponent = Expression::parseNumeral(exp, base)->getR();
        if(exp[0] == '-') exponent = -exponent;
    }
    else ePos = pPos;
    //Get digits
    string digits = str.substr(start, ePos - start);
    #if defined( USE_ARB) || defined(GMP_WASM)
        //If digits are longer than 15, default to arb
    if(digits.length() > 15 && precision == 15 && pPos == str.length())
        precision = digits.length() - (digits.find('.') != string::npos);
    //Parse as arb if precision is specified
    if(precision != 15) {
        #ifdef USE_ARB
        mppp::real out(digits, base, Arb::digitsToPrecision(precision));
        if(exponent != 0) out = out * mppp::pow(mppp::real{ base }, mppp::real{ exponent,Arb::digitsToPrecision(precision + 10) });
        #endif
        #ifdef GMP_WASM
        mpfr_t out = mpfr_t::stringToArb(digits, precision, base);
        if(exponent != 0) out = out * std::pow(mpfr_t(double(base), precision + 10), mpfr_t(double(exponent), precision + 10));
        #endif
        return std::make_shared<Arb>(out);
    }
    #else
    if(precision != 15) throw "Precision could not be specified, arb library not present";
    #endif
    char maxAlphaChar = 'A' + base - 10;
    //Parse decimal part of digits
    int period = digits.length();
    double decimal = 0;
    if(digits.find('.') != string::npos) {
        period = digits.find('.');
        for(int i = digits.length() - 1;i != period;i--) {
            int digit = -1;
            if(str[i] >= '0' && str[i] <= '9')
                digit = digits[i] - '0';
            else if(str[i] >= 'A' && str[i] < maxAlphaChar)
                digit = digits[i] - 'A' + 10;
            if(digit != -1) {
                decimal += digit;
                decimal /= base;
            }
        }
    }
    //Parse integer part of digits
    double integer = 0;
    for(int i = 0;i < period;i++) {
        int digit = -1;
        if(digits[i] >= '0' && digits[i] <= '9')
            digit = digits[i] - '0';
        else if(digits[i] >= 'A' && digits[i] < maxAlphaChar)
            digit = digits[i] - 'A' + 10;
        if(digit != -1) {
            integer *= base;
            integer += digit;
        }
    }
    if(exponent != 0)
        return std::make_shared<Number>(std::pow(base * 1.0, exponent * 1.0) * (integer + decimal));
    return std::make_shared<Number>(integer + decimal);
}
int Expression::nextSection(const string& str, int start, Expression::Section* type, ParseCtx& ctx) {
    using namespace Expression;
    while(str[start] == ' ') start++;
    char ch = str[start];
    //Brackets and quotes
    if(ch == '(' || ch == '[' || ch == '<' || ch == '{' || ch == '"') {
        int match = matchBracket(str, start);
        if(type) *type = Section(bracs.at(ch));
        //Deal with less than or greater than mix up with brackets
        if(bracs.at(ch) == 4) {
            if(match == -1) match = -2;
            if(str[start + 1] == '=') {
                if(type) *type = Section::operat;
                return start + 2;
            }
        }
        if(match == -1) return str.length() + 1;
        else if(match != -2) {
            // [ABC]_16 base notation
            if(ch == '[' && str[match + 1] == '_') {
                if(type) *type = Section::squareWithBase;
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
    else if(ch >= '0' && ch <= '9' || ch == '.') {
        //Deal with 0b and 0x... prefixes
        int base;
        if(ch == '0' && ((basesPrefix.find(str[start + 1]) != basesPrefix.end()))) {
            base = basesPrefix.at(str[start + 1]);
            start += 2;
        }
        else base = ctx.getBase();
        if(base == -1) base = ctx.getBase();
        char maxAlpha = 'A' + base - 10;
        if(type) *type = Section::numeral;
        for(int i = start;i < str.length();i++) {
            char n = str[i];
            if((n == '-' || n == '+') && str[i - 1] != 'e') return i;
            if(n >= '0' && n <= '9');
            else if(n >= 'A' && n < maxAlpha);
            else if(n == '.' || n == ' ');
            else if(n == '-' || n == '+' || n == 'e' || n == 'p');
            else return i;
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
            else if(c == ' ' || c == '_' || c == '$') continue;
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
    if(operatorChars.find(ch) != operatorChars.end()) {
        if(type) *type = Section::operat;
        //i is the character past the end of the operator
        int i = start;
        //Suffix operators
        if(suffixOperators.find(str[i]) != suffixOperators.end()) i++;
        //Find upper and lower bounds on first character of the operator
        auto lower = operatorList.lower_bound(string(1, str[i]));
        auto upper = operatorList.upper_bound(string(1, str[i]));
        //Fix faulty comparison in upper bound
        if(upper != operatorList.end()) while(upper->first[0] == str[i]) upper++;
        //Find longest operator in list
        int maxOpLen = 1;
        for(;lower != upper;lower++) {
            if(lower->first.length() > maxOpLen)
                if(str.compare(i, lower->first.length(), lower->first) == 0)
                    maxOpLen = lower->first.length();
        }
        i += maxOpLen;
        //Prefix operators
        if(prefixOperators.find(str[i]) != prefixOperators.end()) i++;
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
        //Deal with unmatched brackets
        if(end > str.length()) {
            out.back().first += " ";
        }
        //Iterate to next index
        i = end - 1;
    }
    return out;
}
Value Expression::evaluate(const string& str) {
    Value tr = Tree::parseTree(str, Program::parseCtx);
    return tr->compute(Program::computeCtx);
}
std::tuple<string, ValList, string> Expression::parseAssignment(string str) {
    if(str[0] >= '0' && str[0] <= '9') return std::tuple<string, ValList, string>();
    for(int i = 0;i < str.length();i++) {
        if(str[i] >= 'A' && str[i] <= 'Z') continue;
        else if(str[i] >= 'a' && str[i] <= 'z') continue;
        else if(str[i] >= '0' && str[i] <= '9') continue;
        else if(str[i] == '_' || str[i] == ' ') continue;
        else if(str[i] == '=') {
            //Ignore lambdas
            if(str[i + 1] == '>') break;
            //Get name and assignment
            string name = str.substr(0, i);
            name = sanitizeVariable(name);
            return std::tuple<string, ValList, string>(name, {}, str.substr(i + 1));
        }
        //Index assignment
        else if(str[i] == '[' && i != 0) {
            string name = str.substr(0, i);
            int bracketDepth = 0;
            std::vector<int> startingBrackets;
            std::vector<int> endingBrackets;
            while(str[i] == '[') {
                startingBrackets.push_back(i);
                i = matchBracket(str, i) + 1;
                endingBrackets.push_back(i);
                while(str[i] == ' ') i++;
                bracketDepth++;
            }
            if(str[i] == '=') {
                name = sanitizeVariable(name);
                ValList indicies(bracketDepth);
                for(int x = 0;x < bracketDepth;x++) {
                    indicies[x] = evaluate(str.substr(startingBrackets[x] + 1, endingBrackets[x] - startingBrackets[x] - 2));
                }
                return std::tuple<string, ValList, string>(name, indicies, str.substr(i + 1));
            }
            else break;
        }
        else if(str[i] == '(' && i != 0) {
            string name = str.substr(0, i);
            name = sanitizeVariable(name);
            int endB = matchBracket(str, i);
            if(endB == str.length()) break;
            int eq = endB + 1;
            while(str[eq] == ' ') eq++;
            if(str[eq] != '=') break;
            string function = str.substr(i, endB - i + 1) + "=>" + str.substr(eq + 1);
            return std::tuple<string, ValList, string>(name, {}, function);
        }
        else break;
    }
    return std::tuple<string, ValList, string>();
}
#pragma endregion
#pragma region ColoredString
void ColoredString::setStr(string&& s) {
    str = std::forward<string>(s);
    color.resize(s.size(), Expression::hl_text);
}
void ColoredString::setColor(string&& c) {
    color = std::forward<string>(c);
    if(color.size() < str.size()) color.resize(str.size(), Expression::hl_text);
}
string ColoredString::toHTML()const {
    string out = "";
    char prevColor = 0;
    for(int i = 0;i < color.length();i++) {
        if(color[i] != prevColor || i == 0) {
            if(i != 0) out += "</span>";
            out += "<span class='COL";
            out += color[i];
            out += "'>";
            prevColor = color[i];
        }
        if(str[i] == '&') out += "&amp;";
        else if(str[i] == '<') out += "&lt;";
        else if(str[i] == '>') out += "&gt;";
        else if(str[i] == '"') out += "&quot;";
        else if(str[i] == ' ') out += "&#32;";
        else out += str[i];
    }
    out += "</span>";
    return out;
}
ColoredString ColoredString::operator+(ColoredString& rhs) {
    return ColoredString(str + rhs.getStr(), color + rhs.getColor());
}
ColoredString& ColoredString::operator+=(const ColoredString& rhs) {
    str += rhs.getStr();
    color += rhs.getColor();
    return *this;
}
ColoredString& ColoredString::operator+=(char c) {
    str += c;
    color += Expression::hl_text;
    return *this;
}
ColoredString& ColoredString::operator+=(const char* c) {
    str += c;
    color.resize(str.size(), Expression::hl_text);
    return *this;
}
size_t ColoredString::length() {
    return str.size();
}
void ColoredString::splice(int pos, int len) {
    str.erase(pos + len);
    str.erase(0, pos);
    color.erase(pos + len);
    color.erase(0, pos);
}
void ColoredString::colorAsExpression() {
    color = string(str.size(), Expression::hl_error);
    Expression::color(str, color.begin(), Program::parseCtx);
}
ColoredString ColoredString::fromXpr(const string& str) {
    ColoredString out(str);
    out.colorAsExpression();
    return out;
}
ColoredString& ColoredString::append(const std::vector<ColoredString>& args) {
    for(int i = 0;i < args.size();i++) (*this) += args[i];
    return *this;
}
void Expression::color(string str, string::iterator output, ParseCtx& ctx) {
    if(str.length() == 0) return;
    std::vector<std::pair<string, Section>> secs = Expression::getSections(str, ctx);
    int pos = 0;
    //Deal with front-trailing spaces
    while(str[pos] == ' ') {
        *(output + pos) = ColorType::hl_space;
        pos++;
    }
    Section previousSec = undefined;
    //Loop through sections
    for(int i = 0;i < secs.size();i++) {
        string& sec = secs[i].first;
        Section& type = secs[i].second;
        string::iterator out = output + pos;
        if(type == Section::numeral)
            std::fill(out, out + sec.length(), ColorType::hl_numeral);
        else if(type == Section::quote) {
            std::fill(out, out + sec.length(), ColorType::hl_string);
        }
        else if(type == Section::parenthesis) {
            //Color parenthesis
            if(sec.back() != ')') *out = ColorType::hl_error;
            else {
                *out = ColorType::hl_bracket;
                *(out + sec.length() - 1) = ColorType::hl_bracket;
            }
            //Color innards
            Expression::color(sec.substr(1, sec.length() - 2), out + 1, ctx);
        }
        else if(type == Section::square) {
            //Color brackets
            if(sec.back() != ']') *out = ColorType::hl_error;
            else {
                *out = ColorType::hl_bracket;
                *(out + sec.length() - 1) = ColorType::hl_bracket;
            }
            //Color as an accessor
            if(previousSec == variable || previousSec == parenthesis || previousSec == vect || previousSec == curly || previousSec == quote || previousSec == function || previousSec == square)
                Expression::color(sec.substr(1, sec.length() - 2), out + 1, ctx);
            //Color as a unit
            else {
                ctx.push(0, true);
                Expression::color(sec.substr(1, sec.length() - 2), out + 1, ctx);
                ctx.pop();
            }
        }
        else if(type == Section::squareWithBase) {
            int endB = matchBracket(sec, 0);
            int underscore = endB + 1;
            double base = -1;
            try {
                ParseCtx pctx;
                Value tr = Tree::parseTree(sec.substr(underscore + 1), ctx);
                ComputeCtx cctx;
                Value v = tr->compute(cctx);
                if(v != nullptr) base = v->getR();
            }
            catch(...) {}
            if(base < 2 || base>36) {
                std::fill(out + underscore + 1, out + sec.length(), ColorType::hl_error);
                base = ctx.getBase();
            }
            else Expression::color(sec.substr(underscore + 1), out + underscore + 1, ctx);
            ctx.push(base);
            Expression::color(sec.substr(1, endB - 1), out + 1, ctx);
            ctx.pop();
            *out = ColorType::hl_bracket;
            *(out + underscore) = ColorType::hl_operator;
            *(out + endB) = ColorType::hl_bracket;
        }
        else if(type == Section::vect) {
            std::vector<int> commas = getDelimiterList(sec, 0, sec.length() - 1, ',');
            for(int i = 0;i < commas.size() - 1;i++) {
                *(out + commas[i]) = ColorType::hl_delimiter;
                Expression::color(sec.substr(commas[i] + 1, commas[i + 1] - commas[i] - 1), out + commas[i] + 1, ctx);
            }
            //Color brackets
            *out = ColorType::hl_bracket;
            *(out + sec.length() - 1) = ColorType::hl_bracket;
        }
        else if(type == Section::variable) {
            int len = sec.length();
            Value a = ctx.getVariable(sec);
            ColorType type;
            if(a == nullptr) type = ColorType::hl_error;
            else if(a->typeID() == Value::tre_t) type = ColorType::hl_function;
            else if(a->typeID() == Value::arg_t) type = ColorType::hl_argument;
            else if(a->typeID() == Value::num_t) type = ColorType::hl_unit;
            else if(a->typeID() == Value::var_t) type = ColorType::hl_variable;
            std::fill(out, out + len, type);
        }
        else if(type == Section::function) {
            int bracket = findNext(sec, 0, '(');
            //Color name
            string name = sec.substr(0, bracket);
            int nameLen = name.length();
            Value func = ctx.getVariable(name);
            ColorType nameType;
            if(func == nullptr) nameType = ColorType::hl_error;
            else if(func->typeID() == Value::tre_t) nameType = ColorType::hl_function;
            else if(func->typeID() == Value::arg_t) nameType = ColorType::hl_argument;
            else if(func->typeID() == Value::num_t) nameType = ColorType::hl_error;
            else if(func->typeID() == Value::var_t) nameType = ColorType::hl_variable;
            std::fill(out, out + nameLen, nameType);
            //Color commas and arguments
            std::vector<int> commas = getDelimiterList(sec, bracket, sec.length() - 1, ',');
            for(int i = 0;i < commas.size() - 1;i++) {
                *(out + commas[i]) = ColorType::hl_delimiter;
                Expression::color(sec.substr(commas[i] + 1, commas[i + 1] - commas[i] - 1), out + commas[i] + 1, ctx);
            }
            //Color brackets
            if(sec.back() != ')')
                *(out + bracket) = ColorType::hl_error;
            else {
                *(out + bracket) = ColorType::hl_bracket;
                *(out + sec.length() - 1) = ColorType::hl_bracket;
            }
        }
        else if(type == Section::lambda) {
            int equal;
            std::vector<string> arguments;
            equal = Expression::findNext(sec, 0, '=');
            //Color arg list (a,b)
            if(sec[0] == '(') {
                arguments = colorArgList(sec, out, ctx);
            }
            //Else if single variable
            else {
                std::fill(out, out + equal, ColorType::hl_argument);
                string name = Expression::sanitizeVariable(sec.substr(0, equal));
                if(name != "_") arguments.push_back(name);
            }
            //Color => symbol
            *(out + equal) = ColorType::hl_operator;
            *(out + equal + 1) = ColorType::hl_operator;
            //Color rest of statement
            ctx.push(arguments);
            Expression::color(sec.substr(equal + 2), out + equal + 2, ctx);
            ctx.pop();
        }
        else if(type == Section::curly) {
            int endB = matchBracket(sec, 0);
            std::vector<int> commas = getDelimiterList(sec, 0, endB, ',');
            for(int i = 0;i < commas.size() - 1;i++) {
                *(out + commas[i]) = ColorType::hl_delimiter;
                int colon = findNext(sec, commas[i] + 1, ':');
                if(colon > commas[i + 1] || colon == -1)
                    Expression::color(sec.substr(commas[i] + 1, commas[i + 1] - commas[i] - 1), out + commas[i] + 1, ctx);
                else {
                    *(out + colon) = ColorType::hl_operator;
                    Expression::color(sec.substr(commas[i] + 1, colon - commas[i] - 1), out + commas[i] + 1, ctx);
                    Expression::color(sec.substr(colon + 1, commas[i + 1] - colon - 1), out + colon + 1, ctx);
                }
            }
            //Color brackets
            if(sec[endB] != '}')
                *out = ColorType::hl_error;
            else {
                *out = ColorType::hl_bracket;
                *(out + endB) = ColorType::hl_bracket;
            }
        }
        else if(type == Section::operat) {
            std::fill(out, out + sec.length(), ColorType::hl_operator);
        }
        else std::fill(out, out + sec.length(), ColorType::hl_error);
        //Iterate out to next position
        pos += sec.length();
        while(str[pos] == ' ') {
            *(output + pos) = ColorType::hl_space;
            pos++;
        }
        previousSec = type;
    }
}
std::vector<string> Expression::colorArgList(string sec, string::iterator out, ParseCtx& ctx) {
    int end = Expression::matchBracket(sec, 0);
    int equal = Expression::findNext(sec, end + 1, '=');
    //Split by commas
    std::vector<int> commas = getDelimiterList(sec, 0, end, ',');
    std::vector<string> arguments;
    for(int i = 0;i < commas.size() - 1;i++) {
        //Color comma
        *(out + commas[i]) = ColorType::hl_delimiter;
        //Color if is valid argument
        string arg = sec.substr(commas[i] + 1, commas[i + 1] - commas[i] - 1);
        bool isValid = true;
        if(arg[0] >= '0' && arg[0] <= '9') isValid = false;
        else for(int x = 0;x < arg.length();x++) {
            if(arg[x] >= 'a' && arg[x] <= 'z') {}
            else if(arg[x] >= 'A' && arg[x] <= 'Z') {}
            else if(arg[x] >= '0' && arg[x] <= '9') {}
            else if(arg[x] == '_' || arg[x] == ' ') {}
            else { isValid = false;break; }
        }
        ColorType type = isValid ? hl_argument : hl_error;
        std::fill(out + commas[i] + 1, out + commas[i + 1], type);
        //Add to arg list if valid
        if(isValid) arguments.push_back(Expression::sanitizeVariable(arg));
    }
    //Color brackets
    *out = ColorType::hl_bracket;
    *(out + end) = ColorType::hl_bracket;
    return arguments;
}
string Expression::colorLine(string str, ParseCtx& ctx) {
    string out(str.length(), hl_null);
    if(str == "") return "";
    string commandPrefix = Preferences::getAs<string>("command_prefix");
    if(str.substr(0, commandPrefix.length()) == commandPrefix) {
        int sp = findNext(str, 1, ' ');
        if(sp == -1) return string(str.length(), ColorType::hl_command);
        else {
            std::fill(out.begin(), out.begin() + sp, hl_command);
            std::fill(out.begin() + sp, out.end(), hl_text);
        }
        return out;
    }
    std::tuple<string, ValList, string> assign;
    try { assign = Expression::parseAssignment(str); }
    catch(...) {}
    if(std::get<0>(assign) != "") {
        string& name = std::get<0>(assign);
        int eq = findNext(str, 0, '=');
        int b = findNext(str, 0, '[');
        int p = findNext(str, 0, '(');
        //As assignment with index
        if(b != -1 && b < eq) {
            std::fill(out.begin(), out.begin() + b, hl_variable);
            while(true) {
                int end = matchBracket(str, b);
                //Color within brackets
                out[b] = hl_bracket;
                color(str.substr(b + 1, end - b - 1), out.begin() + b + 1, ctx);
                out[end] = hl_bracket;
                //Find next bracket or equal
                while(str[end] == ' ') out[end++] = hl_space;
                if(str[end] == '=') {
                    out[end] = hl_command;
                    color(str.substr(end + 1), out.begin() + end + 1, ctx);
                    return out;
                }
                else if(str[end] == '[') {
                    b = end;
                    continue;
                }
                else return out;
            }
        }
        //As functional assignment
        else if(p != -1 && p < eq) {
            std::fill(out.begin(), out.begin() + p, hl_variable);
            int end = matchBracket(str, p);
            int eq = findNext(str, end + 1, '=');
            std::vector<string> args = colorArgList(str.substr(p, end - p + 1), out.begin() + p, ctx);
            ctx.push(args);
            //Color expression
            out[eq] = hl_command;
            color(str.substr(eq + 1), out.begin() + eq + 1, ctx);
            ctx.pop();
        }
        //As regular assignment
        else {
            std::fill(out.begin(), out.begin() + eq, hl_variable);
            out[eq] = hl_command;
            color(str.substr(eq + 1), out.begin() + eq + 1, ctx);
            return out;
        }
    }
    else {
        color(str, out.begin(), ctx);
    }
    return out;
}
#pragma endregion
//THE TREE PARSER
//
//
//
//
//
Value Tree::parseTree(const string& str, ParseCtx& ctx) {
    using namespace Expression;
    if(str.length() == 0) return Value::zero;
    std::vector<std::pair<string, Section>> sections = getSections(str, ctx);
    ValList treeList;
    std::vector<std::pair<string, int>> operators;
    std::vector<string> unaryOpFront;
    std::vector<string> unaryOpBack;
    Section prevSec = undefined;
    //Parse each section individually
    for(int j = 0;j < sections.size();j++) {
        Section type = sections[j].second;
        string& sect = sections[j].first;
        //Parenthesis ()
        if(type == parenthesis)
            treeList.push_back(Tree::parseTree(sect.substr(1, sect.length() - 2), ctx));
        //Square brackets for accessor or units
        else if(type == square) {
            //Accessor notation
            if(prevSec == variable || prevSec == parenthesis || prevSec == vect || prevSec == curly || prevSec == quote || prevSec == function || prevSec == square) {
                Value index = Tree::parseTree(sect.substr(1, sect.length() - 2), ctx);
                treeList.back() = std::make_shared<Tree>("get", ValList{ treeList.back(),index });
            }
            //Unit notation
            else {
                ctx.push(0, true);
                try { treeList.push_back(Tree::parseTree(sect.substr(1, sect.length() - 2), ctx)); }
                catch(...) { ctx.pop();throw; }
                ctx.pop();
            }
        }
        //Vectors <>
        else if(type == vect) {
            //Split elements by commas
            std::vector<string> comp = splitBy(sect, 0, sect.length() - 1, ',');
            ValList vectorComponents;
            for(int i = 0;i < comp.size();i++) {
                vectorComponents.push_back(Tree::parseTree(comp[i], ctx));
            }
            treeList.push_back(std::make_shared<Vector>(0));
            treeList.back().cast<Vector>()->vec = std::move(vectorComponents);
        }
        //Strings ""
        else if(type == quote) {
            string out;
            int offset = 1;
            for(int i = 1;i < sect.size() - 1;i++) {
                if(str[i] == '\\') {
                    char next = sect[i + 1];
                    if(next == '\"') out += '\"';
                    else if(next == 'n') out += '\n';
                    else if(next == 't') out += '\t';
                    else if(next == '\\') out += '\\';
                    else { out += "\\";out += next; }
                    i++;
                }
                else out += sect[i];
            }
            treeList.push_back(std::make_shared<String>(out));
        }
        //Square bracket with unit [0A]_12
        else if(type == squareWithBase) {
            int endBracket = Expression::matchBracket(sect, 0);
            int underscore = Expression::findNext(sect, endBracket, '_');
            int base = Expression::evaluate(sect.substr(underscore + 1))->getR();
            if(base >= 36) throw "base cannot be over 36";
            if(base <= 1) throw "base cannot be under 2";
            ctx.push(base, true);
            try { treeList.push_back(Tree::parseTree(sect.substr(1, endBracket - 1), ctx)); }
            catch(...) { ctx.pop(); throw; }
            ctx.pop();
        }
        //Numeral 0.442e1
        else if(type == numeral) {
            treeList.push_back(parseNumeral(sect, ctx.getBase()));
        }
        //Variables
        else if(type == variable) {
            Value op = ctx.getVariable(sect);
            if(op.get() == nullptr) throw "Variable " + sect + " not found";
            if(op->typeID() == Value::tre_t) {
                int id = op.cast<Tree>()->op;
                if(Program::assertArgCount(id, 0) != true)
                    throw "Variable " + str + " cannot run without arguments";
                treeList.push_back(op);
            }
            else if(op->typeID() == Value::arg_t || op->typeID() == Value::num_t || op->typeID() == Value::var_t) {
                treeList.push_back(op);
            }
            else { throw "Variable " + sect + " not found"; }
        }
        //Functions func(a)
        else if(type == function) {
            int startBrace = findNext(sect, 0, '(');
            int endBrace = matchBracket(sect, startBrace);
            string name = sect.substr(0, startBrace);
            Value op = ctx.getVariable(name);
            if(op.get() == nullptr) throw "Function " + name + " not found";
            std::vector<string> argsStr = splitBy(sect, startBrace, endBrace, ',');
            ValList arguments;
            for(int i = 0;i < argsStr.size();i++)
                arguments.push_back(Tree::parseTree(argsStr[i], ctx));
            if(op->typeID() == Value::tre_t) {
                std::shared_ptr<Tree> tr = op.cast<Tree>();
                if(Program::assertArgCount(tr->op, argsStr.size()) != true)
                    throw "Function '" + name + "' cannot run with " + std::to_string(argsStr.size()) + " arguments";
                tr->branches = std::move(arguments);
                treeList.push_back(op);
            }
            if(op->typeID() == Value::arg_t || op->typeID() == Value::var_t) {
                if(arguments.size() != 0) {
                    arguments.emplace(arguments.begin(), op);
                    treeList.push_back(std::make_shared<Tree>("run", std::move(arguments)));
                }
                else treeList.push_back(op);
            }
        }
        //Lambda a=>a^2
        else if(type == lambda) {
            std::vector<string> arguments;
            int arrow;
            if(str[0] == '(') {
                int endBrace = matchBracket(sect, 0);
                arguments = splitBy(sect, 0, endBrace, ',');
                for(int i = 0;i < arguments.size();i++) arguments[i] = Expression::sanitizeVariable(arguments[i]);
                arrow = str.find('>', endBrace + 1);
            }
            else {
                arrow = str.find('>');
                string var = Expression::sanitizeVariable(sect.substr(0, arrow - 1));
                if(var != "_") arguments.push_back(var);
            }
            ctx.push(arguments);
            Value tr;
            try { tr = Tree::parseTree(str.substr(arrow + 1), ctx); }
            catch(...) { ctx.pop(); throw; }
            ctx.pop();
            treeList.push_back(std::make_shared<Lambda>(arguments, tr));
        }
        //Maps {"a":1, "b":2 }
        else if(type == curly) {
            int endB = matchBracket(sect, 0);
            std::vector<string> pairs = splitBy(sect, 0, endB, ',');
            std::shared_ptr<Map> map = std::make_shared<Map>();
            for(int i = 0;i < pairs.size();i++) {
                //Find colon
                int colon = findNext(pairs[i], 0, ':');
                if(colon == -1) throw "':' not found in map pair";
                //Parse key and value
                Value first = Tree::parseTree(pairs[i].substr(0, colon), ctx);
                Value second = Tree::parseTree(pairs[i].substr(colon + 1), ctx);
                map->append(first, second);
            }
            treeList.push_back(map);
        }
        //Operators +-
        else if(type == operat) {
            sect = removeSpaces(sect);
            //Prefix operators
            if(prefixOperators.find(sect.back()) != prefixOperators.end()) {
                unaryOpFront.resize(treeList.size() + 1);
                if(sect.back() == '-' && j != 0 && sect.length() == 1); //Ignore if a subtraction
                else {
                    unaryOpFront[treeList.size()] = prefixOperators.find(sect.back())->second;
                    if(sect.length() == 1) continue;
                    sect = sect.substr(0, sect.length() - 1);
                }
            }
            //Suffix operators
            if(suffixOperators.find(sect.front()) != suffixOperators.end() && sect != "!=") {
                unaryOpBack.resize(treeList.size() + 1);
                unaryOpBack[treeList.size() - 1] = suffixOperators.find(sect.front())->second;
                if(sect.length() == 1) continue;
                sect = sect.substr(1);
            }
            //Find operator name
            auto op = operatorList.find(sect);
            if(op == operatorList.end()) {
                throw "Operator " + sect + " not found";
            }
            //Add to list
            operators.resize(treeList.size());
            if(treeList.size() == 0) throw "Missing expression before " + sect;
            operators[treeList.size() - 1] = op->second;
        }
        else {
            throw "Unable to parse " + sect;
        }
        prevSec = type;
    }
    if(treeList.size() == 0) throw "Unable to parse " + str;
    for(int i = 0;i < unaryOpFront.size();i++) if(unaryOpFront[i] != "")
        treeList[i] = std::make_shared<Tree>(unaryOpFront[i], ValList{ treeList[i] });
    for(int i = 0;i < unaryOpBack.size();i++) if(unaryOpBack[i] != "")
        treeList[i] = std::make_shared<Tree>(unaryOpBack[i], ValList{ treeList[i] });
    if(operators.size() == treeList.size()) throw "Missing expression after " + operators.back().first;
    operators.resize(treeList.size());
    //Multiply adjacents when there is no operator
    for(int i = 0;i < operators.size();i++)
        if(operators[i].first == "") operators[i] = std::pair<string, int>("mul", 2);
    //Combine operators and values into singular list
    typedef std::pair<Value, std::pair<string, int>*> valOp;
    //Push all but final value to data list
    std::list<valOp> data;
    for(int i = 0;i < treeList.size();i++)
        data.push_back(valOp(treeList[i], &operators[i]));
    //Sort pointers to valops by operator precedence
    std::list<std::list<valOp>::iterator> sortedByPrecedence;
    for(auto it = data.begin();it != std::prev(data.end());it++) sortedByPrecedence.push_back(it);
    sortedByPrecedence.sort([](std::list<valOp>::iterator a, std::list<valOp>::iterator b) {
        return a->second->second < b->second->second;
        });
    //Combine elements
    for(auto& it : sortedByPrecedence) {
        auto cur = it;
        auto next = ++it;
        next->first = std::make_shared<Tree>(cur->second->first, ValList{ cur->first,next->first });
        data.erase(cur);
    }
    if(data.size() != 1)
        throw "operator combination error";
    return data.front().first;
}