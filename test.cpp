#include "src/_header.hpp"
#include <chrono>

namespace TestResult {
    inline void printIfError(const string& msg) {
        if(msg.length()) {
            std::cout << "Error: " << msg << std::endl;
        }
    }
    inline static string fail(const string& message, const string& name) {
        return "failed " + name + ", " + message;
    }
    inline static string thrown(const string& message, const string& name, const string& identifier) {
        return identifier + " threw \"" + message + "\" (" + name + ")";
    }
    inline static string success() {
        return "";
    }
};
#pragma region Random generation
namespace Generate {
    int fastrand_seed;
    inline int fastrand() {
        fastrand_seed = (214013 * fastrand_seed + 2531011);
        return (fastrand_seed >> 16) & 0x7FFF;
    }
    string variable_name() {
        const static string str = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_____.....0123456789";
        const static int chars = str.length();
        int count = fastrand() % 5 + 1;
        string out(size_t(count), ' ');
        out[0] = fastrand() % (chars - 10);
        for(int i = 1;i < count;i++)
            out[i] = fastrand() % chars;
        return out;
    }
    string valid_variable(ParseCtx& ctx, bool isFunction, int* argCount, bool allowUnit) {
        int type = fastrand() % 4;
        if(type == 1 && (isFunction || !allowUnit)) type = 0;
        //Variable
        if(type == 2) {
            std::map<string, int>& vars = ctx.getVariableList();
            if(vars.size() == 0) type = 0;
            else {
                std::map<string, int>::iterator it = vars.begin();
                std::advance(it, (fastrand() % vars.size()));
                return it->first;
            }
        }
        //Argument
        if(type == 3) {
            if(ctx.argCount() == 0) type = 0;
            else return ctx.getArgName(fastrand() % ctx.argCount());
        }
        //Unit
        if(type == 1) {
            std::unordered_map<string, Unit::Builtin>::const_iterator it = Unit::listOfUnits.begin();
            std::advance(it, fastrand() % Unit::listOfUnits.size());
            string prefix;
            if(std::get<2>(it->second)) {
                auto it = Unit::powers.begin();
                std::advance(it, fastrand() % Unit::powers.size());
                prefix = it->first;
            }
            return prefix + it->first;
        }
        //Global function
        if(type == 0 && isFunction) {
            std::map<string, int>::iterator it = Program::globalFunctionMap.begin();
            std::advance(it, fastrand() % Program::globalFunctionMap.size());
            if(argCount)
                while(!Program::globalFunctions[it->second].assertArgCount(*argCount)) (*argCount)++;
            return it->first;
        }
        return "pi";
    }
    string expression(int nestLeft, ParseCtx& ctx) {
        int type = fastrand();
        if(nestLeft == 0) type %= 3;
        else type %= 8;
        //Number
        if(type == 0) {
            int num = fastrand();
            string str = std::to_string(num);
            if(num & 1) str[fastrand() % str.length()] = '.';
            if(num & 2 && str.length() > 2) str[1 + fastrand() % (str.length() - 2)] = 'e';
            if(num & 4 && str.length() > 2) {
                static const string bases = "btodx";
                str[0] = '0';
                str[1] = bases[fastrand() % bases.length()];
            }
            return str;
        }
        //Variable
        else if(type == 1)
            return valid_variable(ctx, false, nullptr, ctx.useUnits());
        //Parenthesis
        else if(type == 2)
            return "(" + expression(nestLeft, ctx) + ")";
        //Square brackets
        else if(type == 3) {
            ctx.push(0, true);
            string out = "[" + expression(nestLeft - 1, ctx) + "]";
            ctx.pop();
            return out;
        }
        //Function
        else if(type == 4) {
            int argCount = 0;
            string name = valid_variable(ctx, true, &argCount, false);
            if(argCount == 0) return name;
            std::vector<string> args;
            for(int i = 0;i < argCount;i++) args.push_back(expression(nestLeft - 1, ctx));
            string out = name + "(";
            for(int i = 0;i < argCount;i++)  out += (i != 0 ? "," : " ") + args[i];
            return out + ")";
        }
        //Vector
        else if(type == 5) {
            int count = fastrand() % 5;
            string out = "(<";
            for(int i = 0;i < count;i++) {
                out += (i != 0 ? "," : " ") + expression(nestLeft - 1, ctx);
            }
            return out + ">)";
        }
        //Lambda
        else if(type == 90) {
            int argCount = fastrand() % 4;
            string out;
            std::vector<string> args;
            if(argCount == 0) out = "_";
            if(argCount == 1) { args.push_back(variable_name());out = args.back(); }
            else {
                for(int i = 0;i < argCount;i++) {
                    args.push_back(variable_name());
                    out += (i == 0 ? "(" : ",") + args.back();
                }
                out += ")";
            }
            ctx.push(args);
            out += "=>" + expression(nestLeft - 1, ctx);
            ctx.pop();
            return out;
        }
        //Operator
        else if(type == 7) {
            auto it = Expression::operatorList.begin();
            std::advance(it, fastrand() % Expression::operatorList.size());
            return "(" + expression(nestLeft - 1, ctx) + it->first + expression(nestLeft - 1, ctx) + ")";
        }
        //String
        else if(type == 8) {
            return "\"stringy\"";
        }
        return "0";
    }
    string valid_str() {
        static const string validChars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789(())[[]]{{}}<<>>\"\"    ___== +-*/%^$.....,,,,,";
        int count = fastrand() % 50 + 20;
        string out(count, ' ');
        for(int i = 0;i < count;i++) {
            out[i] = validChars[fastrand() % validChars.size()];
        }
        return out;
    }
    string argList() {
        int count = fastrand() % 3 + 1;
        if(count == 1) return "(" + variable_name() + ")";
        else if(count == 2) return "(" + variable_name() + "," + variable_name() + ")";
        else return "(" + variable_name() + "," + variable_name() + "," + variable_name() + ")";
    }
}
#pragma endregion
#pragma region Tests
#define THROW_IF_FAIL_BASE(ident,n) catch(string message) {return TestResult::thrown(message,ident,n);} catch(const char* c) {return TestResult::thrown(string(c),ident,n);} catch(...) {return TestResult::thrown("unknown",ident,n);}
#define THROW_IF_FAIL THROW_IF_FAIL_BASE(identifier,name)
#define THROW_IF_FAIL_RANDOM THROW_IF_FAIL_BASE(identifier,name+ "["+std::to_string(index)+"]")
namespace MatchingXpr {
    const string name = "matching xpr";
    std::map<string, string> tests = {
        {"3+2","5"},
        {"1/2","0.5"},
        {"<1,2>*3","<3,6>"},
        {"sqrt(-1)","-i"},
        {"(1-4)+4","1"},
        {"3*-4","-12"},
        {"2**4","4**2"},
        {"3^4","81"},
        {"24.5","24.5"},
        {"si n(0)","0"},
        {"tan(0)","0"},
        {"[m ]","getu([ft])"},
        {"[m*kg]","getu([ft*lb])"},
        {"run (n=>n+1,4)","5"},
        {"run(run(n=>(j=>(n+j)),4),3)","7"},
        {"sum(n=> n+1,0, 5,1)","21"},
        {"[0A1 ]_16","161"},
        {"[0A1 ]_([10000]_2)","161"},
        {"run((x, y)=>( x + y) , 5, 3 . 4)","8.4"},
        {"0x1A","26"},
        {"0b101.1","5.5"},
        {"0d5 4.3","54.3"},
        {"0o10.1","8+1/8"},
        {"magnitude(<3,4>)","5"},
        {"10(4)","40"},
        {"run(n=>(5)n,4)","20"},
        //"abs(<10,,5>)",},
        {"1+2i","1+2i"},
        {"10.5e2","1050"},
        {"1e1","10"},
        {"[10e3]_4","4^4"},
        {"0e5","0"},
    };
    string validate(std::map<string, string>::iterator it) {
        const string& identifier = it->first;
        try {
            Value first = Expression::evaluate(it->first);
            Value second = Expression::evaluate(it->second);
            if(first != second) {
                if(first->toString() != second->toString())
                    return TestResult::fail(it->first + " does not match " + it->second + " (computed as " + first->toString() + " and " + second->toString() + ")", name);
            }
            return TestResult::success();
        } THROW_IF_FAIL
    }
};
namespace Zeroes {
    const string name = "zero";
    std::vector<string> tests = {
        //Arithmetic
        "i-i", "neg(1)+1", "-1+1", "pow(10,0)-1", "2^0-1", "mod(10,5)", "10%5", "mul(0,10)", "0*10", "10/10-1", "add(neg(1),1)",
        //Trigonometric
        "sin(0)", "cos(0)-1", "floor(tan(1.56))-92", "csc(pi/2)-1", "sec(pi)+1", "floor(1/cot(1.56))-92", "floor(sinh(10.3))-14866", "cosh(0)-1", "tanh(0)", "asin(1)-pi/2", "acos(1)", "atan(1e50)-pi/2", "asinh(sinh(1))-1", "acosh(cosh(1))-1", "round(atanh(tanh(1)))-1",
        //Exponential
        "sqrt(4)-2", "exp(ln(2))-2", "ln(exp(2))-2", "round(log(1000))-3", "round(logb(1000,10))-3", "round(factorial(3))-6", "sgn(i)-i", "abs(-i)-1", "arg(i)-pi/2",
        //Rounding
        "round(0.5)-1", "floor(0.5)", "ceil(0.3)-1", "getr(i)", "geti(10.5[m])", "getu([km])/[m]-1",
        //Comparison
        "(10>4)-1","(-10>4)", "10!=10","(10=10)-1", "equal(10.3,10.3)-1", "min(4,5)-4", "min(5,4)-4", "max(5,4)-5", "max(4,5)-5", "lerp(-1,1,0.5)", "dist(0,3+4i)-5",
        //Binary Operators
        //"not(1)+2", "and(0,5)", "or(0,5)-5", "xor(5,3)-6", "ls(5,1)-10", "rs(5,1)-2",
        //Variables
        "floor(1/(pi-3.14))-627", "floor(1/(e-2.71))-120", "floor(rand)", "ans",
        //Lambda
        "run(x=>x+1,-1)", "sum(x=>x,0,10,1)-55", "product(x=>x,1,10,1)-3628800",
        //Vectors
        "length(<>)", "length(<1,1,1,1,>)-5", "get(<0,1>,0)", "get(<0,1,0>,2)", "magnitude(abs(fill(x=>0,4)))", "magnitude(map(<1,1,sqrt(0.5)*2>,x=>x/2))-1",
        //Numerical parsing
        "1e-2-0.01","1e2-100","1e34-10000000000000000000000000000000000"
    };
    string validate(std::vector<string>::iterator it) {
        const string& identifier = *it;
        try {
            if(!Value::isZero(Expression::evaluate(*it)))
                return TestResult::fail(*it + " did not return zero", name);
            return TestResult::success();
        } THROW_IF_FAIL
    }
};
namespace Highlight {
    const string name = "highlight";
    std::map<string, string> tests = {
        {"1+2","non"},
        {"1p2-4**2","nnnonoon"},
        {"add(5,[kg])","fffbndbuubb"},
        {"x=>x+1","aooaon"},
        {"0x12A3","nnnnnn"},
        {"<3,4,5>","bndndnb"},
        {"(4<3)+(3>2)","bnonbobnonb"},
        {"( 3 + (4-5)","e nno bnonb"},
        {"<5,x=>x+1,x>","bndaooaondeb"},
        {"[<0A1,1F,3G>]_16","bbnnndnndnebbonn"},
        {"run(x=>[0A1]_x,16)","fffbaoobneeboednnb"},
        {"x=><x,2x,0.5>","aoobadnadnnnb"},
        {"\"abc\\\"123\"","ssssssssss"},
        {"\"abc\"+4.5","sssssonnn"},
        {"^%#$%@","ooeeoe"},
    };
    string validate(std::map<string, string>::iterator it) {
        const string& identifier = it->first;
        try {
            string colored(identifier.length(), Expression::ColorType::hl_error);
            Expression::color(identifier, colored.begin(), Program::parseCtx);
            if(colored != it->second)
                return TestResult::fail(identifier + " highlighted as " + colored + " not as " + it->second, name);
            return TestResult::success();
        } THROW_IF_FAIL
    }
};
namespace RandomHighlight {
    const string name = "random highlight";
    string validate(int index) {
        string identifier = Generate::expression(4, Program::parseCtx);
        try {
            string colored(identifier.length(), Expression::ColorType::hl_error);
            Expression::color(identifier, colored.begin(), Program::parseCtx);
            if(colored.find(Expression::ColorType::hl_error) != string::npos) {
                if(identifier[colored.find(Expression::ColorType::hl_error)] != ',')
                    return TestResult::fail(identifier + " highlighted as " + colored + " with error", name + "[" + std::to_string(index) + "]");
                else return TestResult::success();
            }
            else return TestResult::success();
        } THROW_IF_FAIL
    }
}
namespace LibTest {
    const string name = "library";
    string validate(std::map<string, Library::LibFunc>::iterator it) {
        const string& identifier = it->second.fullName;
        try {
            for(int i = 0;i < it->second.dependencies.size();i++) {
                if(Library::functions.find(it->second.dependencies[i]) == Library::functions.end())
                    return TestResult::fail("Could not resolve dependency " + it->second.dependencies[i], name);
                Library::functions[it->second.dependencies[i]].include();
            }
            Tree::parseTree(it->second.inputs + "=>" + it->second.xpr, Program::parseCtx);
            return TestResult::success();
        } THROW_IF_FAIL
            return "";
    }
}
namespace ParsingRandXpr {
    const string name = "rand_parse";
    string validate(int index) {
        string identifier = Generate::expression(3, Program::parseCtx);
        try {
            //Parse random expression
            Value tr = Tree::parseTree(identifier, Program::parseCtx);
            //Test computation for segfault, other errors are ignored
            try {
                tr->compute(Program::computeCtx);
            }
            catch(...) {}
         //Return success
            return TestResult::success();
        } THROW_IF_FAIL_RANDOM
    }
}
namespace ParsingRandChar {
    const string name = "rand_str_parse";
    string validate(int index) {
        string identifier = Generate::valid_str();
        try {
            Tree::parseTree(identifier, Program::parseCtx);
        }
        catch(...) {}
        return TestResult::success();
    }

}
//Highlighting tests
//Help page tests
//Random highlight
//Random compute
#pragma endregion
template<typename T>
void doTestList(T begin, T end, string(*validate)(T)) {
    auto startTime = std::chrono::steady_clock::now();
    for(T it = begin;it != end;it++) {
        TestResult::printIfError(validate(it));
    }
    auto endTime = std::chrono::steady_clock::now();
    std::chrono::duration<double> diff = endTime - startTime;
    std::cout << "Tests took " << diff.count() << std::endl;
}
void doTestRandom(string(*validate)(int index), float timeSeconds = 3.0) {
    auto startTime = std::chrono::steady_clock::now();
    int thousands = 0;
    while(std::chrono::duration<float>(std::chrono::steady_clock::now() - startTime) < std::chrono::duration<float>(timeSeconds)) {
        for(int i = 0;i < 1000;i++) {
            TestResult::printIfError(validate(thousands + i));
        }
        thousands += 1000;
    }
    std::cout << "Did random tests" << std::endl;
}
#define RunTestList(name) doTestList(name::tests.begin(),name::tests.end(),&name::validate)

int main() {
    Program::smallCompute = true;
    Program::startup();
    Generate::fastrand_seed = std::chrono::steady_clock::now().time_since_epoch().count();
    doTestList(MatchingXpr::tests.begin(), MatchingXpr::tests.end(), &MatchingXpr::validate);
    doTestList(Zeroes::tests.begin(), Zeroes::tests.end(), &Zeroes::validate);
    doTestList(Library::functions.begin(), Library::functions.end(), &LibTest::validate);
    doTestList(Highlight::tests.begin(), Highlight::tests.end(), &Highlight::validate);
    doTestRandom(&RandomHighlight::validate);
    doTestRandom(&ParsingRandXpr::validate);
    doTestRandom(&ParsingRandChar::validate);
}