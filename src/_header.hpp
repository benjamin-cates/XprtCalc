#pragma once
#pragma region Include directives
#include <string>
#include <complex>
#include <memory>
#include <cmath>
#include <functional>
#include <cstdlib>
#include <sstream>
#include <iostream>
//STL containers
#include <map>
#include <unordered_map>
#include <vector>
#include <stack>
#include <list>
#include <deque>
#include <set>
#include <unordered_set>

#ifdef USE_ARB
#include <mp++/mp++.hpp>
#include <mp++/real.hpp>
#endif
#pragma endregion

#pragma region Forward declarations
class Value;
class Tree;
class Unit;
class Function;
struct Command;
class Vector;
class ComputeCtx;
class ParseCtx;
typedef std::shared_ptr<Value> ValPtr;
typedef std::vector<ValPtr> ValList;
using string = std::string;
#pragma endregion
#pragma region Program Information
//This namespace holds runtime preferences that are deeply engrained into the system, or are particular to the implementation (like a dark mode on a web version). All implementation files are in program.cpp.
namespace Preferences {
    ValPtr get(string name);
    //Supported types are getAs<double> and getAs<string>
    template<typename T>
    T getAs(string name);
    void set(string name, ValPtr val);

    //Each preference has a value and an optional update function
    extern std::map<string, std::pair<ValPtr, void (*)(ValPtr)>> pref;

};

//Metadata stores important information about the program, these are only defined in the header file.
namespace Metadata {
    const std::map<string, string> info = {
        {"author","Benjamin Cates"},
        {"version","0.1"},
        {"github","https://github.com/benjamin-cates/XprtCalc"},
        {"name","xprtcalc"}
    };
};

//This namespace holds global variables of the program and important commands and functions.
//program.cpp
namespace Program {
    //startup() should be run at the absolute beginning of the main function
    void startup();
    //cleanup should be called right as the program is about to exit, or the -quit or -exit commands are run
    void cleanup();
    //Holds a pointer to a function that will be run on startup
    extern void (*implementationStartup)();
    //Holds a pointer to a function that will be run on program exit.
    extern void (*implementationCleanup)();

    extern std::map<string, Command> commandList;
    //Runs command in str (containing whole name and prefix) and returns the output
    string runCommand(string str);

    //Global variables
    //Contains every value in previous calculations
    extern ValList history;
    //Global compute context
    extern ComputeCtx computeCtx;
    //Global parsing context
    extern ParseCtx parseCtx;

    //Vector of every global function (compute.cpp)
    extern std::vector<Function> globalFunctions;
    //Maps function names to their indicies in globalFunctions (compute.cpp)
    extern std::unordered_map<string, int> globalFunctionMap;
    //Returns index of global function, or -1 if not found
    int getGlobal(const string& name);
    //Asserts whether argument count is supported with id
    bool assertArgCount(int id, int count);
    //Startup function that builds up the cached name map
    void buildFunctionNameMap();

    //Runs the global function of the specified name with the input list (compute.cpp)
    ValPtr computeGlobal(string name, ValList input, ComputeCtx& ctx);
};

//Contains information that are in the help pages
//help.cpp
namespace Help {
    class Page {
        string name;
        string symbol;
        string type;
        string content;
        std::set<string> aliases;
    public:
        string toPlainText();
        string toJSON();
        string getName();
        string getSymbol();
        string getType();
        string getContent();
        std::set<string>& getAliases();
        int calculatePriority(const string& query);
    };
    extern std::vector<Page> pages;
    //Returns list of pointers to pages, sorted by relevance of the query
    std::vector<Page*> search(const string& query);
};

//program.cpp
struct Command {
    //arguments and types are related and should have the same length
    std::vector<string> arguments;
    // Takes "literal", "value", and "expression"
    std::vector<string> types;
    //Whether the output of the command is a highlightable expression
    bool highlightOutput;
    //pointer to command
    string(*run)(std::vector<string>& inputs);
};

//Library functions are functions whose expressions are included via strings, but they are not compiled when the program starts, they can be added using the include command while the program is running. (program.cpp)
namespace Library {
    struct LibFunc {
        string name;
        std::vector<string> inputs;
        string fullName;
        string type;
        string xpr;
        //Compiles xpr and adds to globalFunctions
        bool include();
        //LibrFunc constructor
        LibFunc(string n, std::vector<string> in, string fullN, string t, string expression);
    };
    //Include all library functions of a certain type (library.cpp)
    void includeAll(string type);
    //Map of function name to the function itself, allows for quick lookups
    extern std::map<string, LibFunc> functions;
}
#pragma endregion
#pragma region Parsing
//Stores variable names, bases, and use units of current parsing context (parse.cpp)
class ParseCtx {

    //Stores a stack of argument names with the most recent at the front
    std::deque<string> argStack;
    //Stores size of arg stack for each new context push
    std::deque<int> argStackSizes;
    //List of all named variables, storing number of declarations
    std::map<string, int> variables;
    //Stores stack of bases with current base at top
    std::stack<int> bases;
    //Stores whether to use units with the current one on top
    std::stack<bool> useUnitsStack;

public:
    //Empty constructor
    ParseCtx();
    //Push a parenthesis context that has a new base or useUnits
    void push(int base = 0, bool useUnits = false);
    //Push a new function context that has additional arguments
    void push(const std::vector<string>& arguments);
    //Pop most recent context
    void pop();
    //Adds variable definition
    void pushVariable(const string& name);
    //Undefines variables
    void popVariables(const std::vector<string>& names);
    //Returns bool whether variable exists
    bool variableExists(const string& name)const;
    //Returns whether units are allowed in current context
    bool useUnits()const;
    //Returns base of current context
    int getBase()const;
    //Adds local variable to current frame
    void pushLocal(const string& name);
    //Returns value with either Tree type with id OR Argument type with id. Returns nullptr if variable not found.
    ValPtr getVariable(const string& name)const;
    //Returns argument name at given index
    string getArgName(int index)const;
};

//All of this is stored in parse.cpp
namespace Expression {
    //Maps brackets to their types
    extern const std::unordered_map<char, int> bracs;
    //Maps brackets to whether they are start or end
    extern const std::unordered_map<char, bool> isStart;
    //Map of operators such as +, -, * to their function name and precedence
    extern const std::map<string, std::pair<string, int>> operatorList;
    //Operators that are before an expression
    extern const std::map<char, string> prefixOperators;
    //Operators that are after an expression
    extern const std::map<char, string> suffixOperators;
    //List of characters that are present in operators
    extern std::unordered_set<char> operatorChars;
    //Types of parsed expressions
    enum Section {
        // section not understood
        undefined = 0,
        // ()
        parenthesis = 1,
        // []
        square = 2,
        // {}
        curly = 3,
        // <>
        vect = 4,
        // ""
        quote = 5,
        // []_b
        squareUnit,
        numeral,
        variable,
        function,
        // a=>... or (a)=>...
        lambda,
        //Operator
        operat
    };
    //Returns string with color data of the expression
    string color(string str, ParseCtx& ctx);
    //Returns string with color data of the expression and command support
    string colorLine(string str, ParseCtx& ctx);
    //Remove non quoted spaces from an expression
    string removeSpaces(const string& str);
    //List of base prefixes e.g. 0b is base 2
    const extern std::unordered_map<char, int> basesPrefix;
    //Parses scalar from string into a value (number or arb if 0a prefix used)
    ValPtr parseNumeral(const string& str, int defaultBase);
    //Returns a list of strings split by given delimiter (start and end not included in output)
    std::vector<string> splitBy(const string& str, int start, int end, char delimiter = ',');
    /**
     * @brief Returns index of the next section
     * @param str
     * @param start Starting index of current section
     * @param type Output of type as Expression::Section (takes pointer). Can be nullptr if type data not needed.
     * @param ctx Parsing context
     * @return Start of next section as index
     */
    int nextSection(const string& str, int start, Expression::Section* type, ParseCtx& ctx);
    //Convert expression into a list of sections with types, such as "(1.4m)*2.5e" becomes "(1.4m)","*","2.5","e"
    std::vector<std::pair<string, Expression::Section>> getSections(const string& str, ParseCtx& ctx);
    //Matches bracket at index start, supports () [] {} <> and ""
    int matchBracket(const string& str, int start);
    //Finds next instance of find ignoring brackets
    int findNext(const string& str, int index, char find);
    //Computes string without context
    ValPtr evaluate(const string& str);
};

#pragma endregion
#pragma region Computation
//Contains data relating to builtin function
class Function {

public:
    //std::function that takes in a list of values and a context and returns a value
    typedef std::function<ValPtr(ValList, ComputeCtx&)> fobj;
    struct Domain;
private:
    //Callable name of this function
    string name;
    //Vector of input names (solely semantic)
    std::vector<string> inputNames;
    //Map from one domain to another. If an input domain matches the left hand side, it will convert it to the right hand side and run the function
    std::map<Domain, Domain> domainMap;
    //Map of domains to their given function objects
    std::map<Domain, fobj> funcs;
public:
    Function();
    Function(string argName, std::vector<string> inputs, std::map<Domain, Domain> domainMap, std::map<Domain, fobj> functions);
    Function(string argName, fobj func);
    //Run function with input list and context ctx
    ValPtr operator()(ValList& input, ComputeCtx& ctx);
    //Get name of function
    string getName()const;
    //Get input name from index
    std::vector<string>& getInputNames();
    //Assert whether argument count is supported
    bool assertArgCount(int count)const;
    //Returns semantic data about the function to be used in documentation commands
    string debugStr();
    //Returns a list of acceptable domains
    std::vector<Domain> getDomain()const;
};
//Holds common math functions
namespace Math {
    //Returns positive or negative infinity
    double Inf(bool negative = false);
    //Returns NaN
    double NaN();
    //Tests if x is NaN
    bool isNan(double x);
    //Tests if x is an infinite type
    bool isInf(double x);
    #ifdef USE_ARB
    //Tests if x is NaN
    mppp::real isNan(const mppp::real& x);
    //Tests if x is an infinite type
    mppp::real isInf(const mppp::real& x);
    //Returns NaN given accuracy in decimal digits
    mppp::real NaN(int accu);
    //Returns inifity given accuracy in decimal digits and positive/negative bool
    mppp::real Inf(int accu, bool negative);
    #endif

};
//compute.cpp
class ComputeCtx {
public:
    //Map of variables to their values
    std::map<string, std::vector<ValPtr>> variables;
    //Indexed list of arguments
    std::deque<ValPtr> argValue;

    //Empty constructor
    ComputeCtx();

    //Sets variable that is highest on the stack
    void setVariable(const string& n, ValPtr value);
    //Creates new definition of a variable
    void defineVariable(const string& n, ValPtr value);
    //Pops most recent definition of each variable
    void undefineVariables(const std::vector<string>& vars);
    //Get variable from name, nullptr if not found
    ValPtr getVariable(const string& name);

    //Get argument from id
    ValPtr getArgument(int id)const;
    //Push argument list in new context
    void pushArgs(const ValList& args);
    //Pop argument list at end of context
    void popArgs(const ValList& args);
};
#pragma endregion
#pragma region Value types
//unit.cpp
class Unit {
    //This variable stores the exponent of eight base units (each 8 bits each) as a signed char
    unsigned long long bits;
public:
    //List of baseUnit variable signs
    static const std::vector<string> baseUnits;
    //List of units, mapping their symbol to their bits, their coefficient, and whether it supports prefixes
    static const std::unordered_map<string, std::tuple<Unit, double, bool, string>> listOfUnits;
    //Map of every metric prefix to its coefficient
    static const std::unordered_map<char, double> powers;
    //Unit constructor
    Unit(unsigned long long b = 0);
    //Returns the ith base unit power
    signed char operator[](int i);
    //Returns the ith base unit power
    signed char get(int i)const;
    //Sets the ith base unit power to value
    void set(int i, signed char value);
    //Returns true if unit has no dimensions
    bool isUnitless()const;
    //Returns the bitset of the unit
    unsigned long long getBits()const;
    //Prints unit as a list of powers multiplied together, e.g. m*s^-1
    string toString()const;
    //Multiplies two units (adds powers)
    Unit operator*(Unit a);
    //Divides two units (subtracts powers)
    Unit operator/(Unit a);
    //Raises unit to a power (multiplies exponents by p)
    Unit operator^(double p);
    //Returns resulting unit only if they are compatible (both the same, or one is unitless) else throws error
    Unit operator+(Unit a);
    //Returns resulting unit only if they are compatible (both the same, or one is unitless) else throws error
    Unit operator|(Unit a);
    //Test whether two units are equal
    bool operator==(const Unit& comp)const;
    //Parses a singular unit symbol, such as "kg" or "Mb". Sets outCoefficient if it is not metric
    static Unit parseName(const string& name, double& outCoefficient);
    //Output preference, this could be something like "kg"->"lb" and every weight unit will be expressed in pounds
    static std::map<Unit, string> outputPreference();
    typedef std::tuple<Unit, double, bool, string> Builtin;
};

//Value is a base-class that uses polymorphism to implement different types. Usually it is wrapped in ValPtr which is a shared_ptr of this type. (value.cpp)
class Value : public std::enable_shared_from_this<Value> {
public:
    //Virtual functions
    Value() {}
    virtual ~Value() = default;
    string toString()const;
    virtual string toStr(ParseCtx& ctx)const { return "<error-type>"; }
    //Returns a uniqueish double for each value so they can be sorted without regard for type
    virtual double flatten()const { return 0; }
    //Returns id, see value type enum for type list
    virtual int typeID()const { return -1; }
    enum { num_t = 1, arb_t = 2, vec_t = 3, lmb_t = 4, str_t = 5, map_t = 6, tre_t = 7, arg_t = 8, var_t = 9 };
    //Returns the real component, or the first value in a vector, useful for converting into an integer value
    virtual double getR()const { return 0; }
    //Flattens down to single value
    virtual ValPtr compute(ComputeCtx& ctx) {
        return shared_from_this();
    }
    //Compare two values
    bool operator==(ValPtr v);
    bool operator!=(ValPtr v);
    //Compares flatten() of each value
    bool operator<(const Value& v)const;
    //Returns true only if type is num or arb and real=1 and imag=0 and unit=0
    static bool isOne(ValPtr x);
    //Returns true only if type is num or arb and real, imag, and unit are zero
    static bool isZero(ValPtr x);
    static ValPtr zero;
    //List of human-readable type names
    static const std::vector<string> typeNames;
    //Converts value into the return type with given type, throws error if incompatible
    static ValPtr convert(ValPtr value, int type);
};
#pragma endregion
#pragma region Value Types
//value.cpp
class Number : public Value {
public:
    std::complex<double> num;
    Unit unit;
    Number(double r, double i = 0.0, Unit u = Unit());
    Number(std::complex<double> n, Unit u = Unit());
    //Convert string to double given base
    static double parseDouble(string in, int base);
    //Convert double to string given base
    static string componentToString(double x, int base);
    //Virtual functions
    double flatten()const;
    double getR()const { return num.real(); }
    string toStr(ParseCtx& ctx)const;
    int typeID()const { return Value::num_t; }
};
#ifdef USE_ARB
//value.cpp
class Arb : public Value {
public:
    std::complex<mppp::real> num;
    Unit unit;
    Arb(mppp::real r, mppp::real i = 0.0, Unit u = Unit());
    Arb(std::complex<mppp::real> n, Unit u = Unit());
    //Get the mpfr precision given decimal digits
    static mpfr_prec_t digitsToPrecision(int digits);
    //Get decimal digits given binary precision
    static int precisionToDigits(mpfr_prec_t prec);
    //Converts mppp::real to string given base
    static string componentToString(mppp::real x, int base);
    //Virtual functions
    double flatten()const;
    string toStr(ParseCtx& ctx)const;
    double getR()const { return double(num.real()); }
    int typeID()const { return Value::arb_t; }
};
#endif
//value.cpp
class Vector : public Value {
public:
    ValList vec;
    //Creates vector with singular value on left
    Vector(ValPtr first);
    //Creates vector with given size
    Vector(int size = 0);
    Vector(ValList&& v);
    int size()const;
    //Get ith value in vector
    ValPtr get(unsigned int x);
    //Get ith value in vector
    ValPtr operator[](unsigned int index);
    //Set ith value in vector to val
    void set(int x, ValPtr val);
    //Virtual functions
    ValPtr compute(ComputeCtx& ctx);
    double flatten()const;
    string toStr(ParseCtx& ctx)const;
    double getR()const { return vec[0]->getR(); }
    int typeID()const { return Value::vec_t; }
};
//value.cpp
class Lambda : public Value {
public:
    //List of input names
    std::vector<string> inputNames;
    //Pointer to compute tree
    ValPtr func;
    //Takes in input list and funcTree, then sets funcTree to nullptr for memory safety
    Lambda(std::vector<string> inputs, ValPtr funcTree);
    //Computes with argument list
    ValPtr operator()(ValList inputs, ComputeCtx& ctx);
    //Virtual functions
    ValPtr compute(ComputeCtx& ctx);
    double flatten()const;
    string toStr(ParseCtx& ctx)const;
    int typeID()const { return Value::lmb_t; }
};
//value.cpp
class String : public Value {
public:
    string str;
    String(string argStr) { str = argStr; }
    //Adds backspaces to make it printable
    static string safeBackspaces(const string& str);
    //Virtual functions
    double flatten()const;
    string toStr(ParseCtx& ctx)const;
    int typeID()const { return Value::str_t; }
};
//value.cpp
class Map : public Value {
    //Cache of leafKey.flatten()
    double flatKey;
public:
    //Implemented as a binary search tree
    std::shared_ptr<Map> left;
    std::shared_ptr<Map> right;
    ValPtr leaf;
    ValPtr leafKey;
    Map() {};
    ValPtr find(ValPtr key, double flat = 0);
    void append(ValPtr key, ValPtr val, double flat = 0);
    //Virtual functions
    double flatten()const;
    string toStr(ParseCtx& ctx)const;
    int typeID()const { return Value::map_t; }
};
//tree.cpp
class Tree : public Value {
public:
    //Operator id in Program::globalFunctions
    int op;
    ValList branches;
    //Create global function from id and list of branches
    Tree(int opId, ValList&& branchList);
    Tree(string opStr, ValList&& branchList);
    //Create binary tree from global function name and two trees
    Tree(string opStr, ValPtr one, ValPtr two);
    //Creates leafless, branchless, variable tree from type and id
    Tree(int id);
    //Parses expression using ctx and creates tree
    static ValPtr parseTree(const string& str, ParseCtx& ctx);

    bool operator==(const Tree& a)const;
    //Returns the ith branch
    ValPtr operator[](int index);

    //Computes the tree using the compute context
    ValPtr compute(ComputeCtx& ctx);
    //Returns the derivative of this
    ValPtr derivative();
    //Returns a webgl script that computes the tree given function inputs
    string toWebGL();
    //Simplifies tree for readability
    void simplify();
    //Simplifies tree for computational efficiency
    void compSimplify();

    //Returns true if it is a leaf with value one
    bool isOne()const;
    //Returns true if it is a leaf with value zero
    bool isZero()const;
    //Returns true if leaf
    bool isLeaf()const;

    //Virtual functions
    string toStr(ParseCtx& ctx)const;
    double flatten()const;
    int typeID()const { return Value::tre_t; }
};
class Argument : public Value {
public:
    int id;
    Argument(int i) { id = i; }
    //Virtual functions
    string toStr(ParseCtx& ctx)const;
    ValPtr compute(ComputeCtx& ctx);
    int typeID()const { return Value::arg_t; }
};
class Variable : public Value {
public:
    string name;
    Variable(const string& n) { name = n; }
    //Virtual functions
    string toStr(ParseCtx& ctx) { return name; }
    ValPtr compute(ComputeCtx& ctx) { return ctx.getVariable(name); }
    int typeID()const { return Value::var_t; }
};
#pragma endregion