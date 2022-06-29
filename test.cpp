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
#define THROW_IF_FAIL catch(string message) {return TestResult::thrown(message,identifier,name);} catch(const char* c) {return TestResult::thrown(string(c),identifier,name);} catch(...) {return TestResult::thrown("unknown",identifier,name);}
namespace Generate {
    int fastrand_seed;
    inline int fastrand() {
        fastrand_seed = (214013 * fastrand_seed + 2531011);
        return (fastrand_seed >> 16) & 0x7FFF;
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
            if(num & 2) str[fastrand() % str.length()] = 'e';
            if(num & 4) {
            #ifdef USE_ARB
                static const string bases = "btodxa";
            #else
                static const string bases = "btodx";
            #endif
                str[0] = '0';
                str[1] = bases[fastrand() % bases.length()];
            }
            return str;
        }
        //Variable
        else if(type == 1) {

        }
        //Parenthesis
        else if(type == 2) {

        }
        //Square brackets
        else if(type == 3) {

        }
        //Function
        else if(type == 4) {

        }
        //Vector
        else if(type == 5) {

        }
        //Lambda
        else if(type == 6) {

        }
        //String
        else if(type == 7) {

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
    string argList() {
        int count = fastrand() % 3 + 1;
        if(count == 1) return "(" + variable_name() + ")";
        else if(count == 2) return "(" + variable_name() + "," + variable_name() + ")";
        else return "(" + variable_name() + "," + variable_name() + "," + variable_name() + ")";
    }
}
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
            ValPtr first = Expression::evaluate(it->first);
            ValPtr second = Expression::evaluate(it->second);
            if(*first != second) {
                if(first->toString()!=second->toString())
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
        "i-i", "neg(1)+1", "-1+1", "pow(10,0)-1", "2^0-1", "mod(10,5)", "10%5", "mult(0,10)", "0*10", "10/10-1", "add(neg(1),1)",
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
namespace LibTest {
    const string name = "library";
    string validate(std::map<string, Library::LibFunc>::iterator it) {
        const string& identifier = it->second.fullName;
        try {
            for(int i=0;i<it->second.dependencies.size();i++) {
                Library::functions[it->second.dependencies[i]].include();
            }
            Tree::parseTree(it->second.inputs+"=>"+it->second.xpr,Program::parseCtx);
            return TestResult::success();
        } THROW_IF_FAIL
            return "";
    }

}
namespace ParsingRand {
    const string name = "rand_parse";
    string validate(int index) {
        const string identifier;
        try {

        } THROW_IF_FAIL
            return "";
    }


}
//Highlighting tests
//Help page tests
//Random highlight
//Random compute
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
void doTestRandom(string(*validate)(int index)) {
    auto startTime = std::chrono::steady_clock::now();
    int thousands = 0;
    while((std::chrono::steady_clock::now() - startTime).count() < 3) {
        for(int i = 0;i < 1000;i++) {
            TestResult::printIfError(validate(thousands + i));
        }
        thousands += 1000;
    }
}
#define RunTestList(name) doTestList(name::tests.begin(),name::tests.end(),&name::validate)

int main() {
    Program::startup();
    Generate::fastrand_seed = std::chrono::steady_clock::now().time_since_epoch().count();
    doTestList(MatchingXpr::tests.begin(), MatchingXpr::tests.end(), &MatchingXpr::validate);
    doTestList(Zeroes::tests.begin(), Zeroes::tests.end(), &Zeroes::validate);
    doTestList(Library::functions.begin(), Library::functions.end(), &LibTest::validate);
    doTestRandom(&ParsingRand::validate);
}