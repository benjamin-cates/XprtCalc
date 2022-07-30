#include "_header.hpp"

#pragma region namespace Math
namespace Math {
    //Returns vector of integer factors
    std::vector<int> getFactors(long n);
    //Greatest common factor
    long gcf(long a, long b);
    //Least commmon multiple
    long lcm(long a, long b);
    bool isPrime(long a);

    double log(double x, double b) { return std::log(x) / std::log(b); }
    double ln(double x) { return std::log(x); }
    double abs(double x) { return fabs(x); }
    double Inf(bool negative) { if(negative) return -INFINITY;return INFINITY; }
    double NaN() { return NAN; }
    bool isNan(double x) { return std::isnan(x); }
    bool isInf(double x) { return std::isinf(x); }
    int sgn(double x) { return (0 < x) - (x < 0); }
    int getAccu(double x) { return 0; }
    double getE() { return 2.7182818284590452353602874713527; }
    double getPi() { return 3.141592653589793238462643383279; }
    double gamma(double x) { return tgamma(x); }
    double leftShift(double x, long shift) { return long(x) << shift; }
    double rightShift(double x, long shift) { return long(x) >> shift; }
    std::complex<double> pow(std::complex<double> a, std::complex<double> b) {
        if(a.imag() == 0 && b.imag() == 0) {
            if(a.real() == std::floor(a.real()) && b.real() == std::floor(b.real())) {
                return std::complex<double>(std::round(std::pow(a.real(), b.real())), 0.0);
            }
            return std::complex<double>(std::pow(a.real(), b.real()), 0.0);
        }
        return std::pow(a, b);
    }
    #ifdef USE_ARB
    mppp::real abs(const mppp::real& x) { return mppp::abs(x); }
    mppp::real ln(const mppp::real& x) { return mppp::log(x); }
    mppp::real log(const mppp::real& x, const mppp::real& b) { return mppp::log(x) / mppp::log(b); }
    //Variables
    mppp::real getE_arb(int accuracy) { return mppp::real_euler(Arb::digitsToPrecision(accuracy)); }
    mppp::real getPi_arb(int accuracy) { return mppp::real_pi(Arb::digitsToPrecision(accuracy)); }
    mppp::real leftShift(const mppp::real& x, long shift) { return mul_2si(x, shift); }
    mppp::real rightShift(const mppp::real& x, long shift) { return div_2si(x, shift); }
    mppp::real gamma(const mppp::real& x) { return mppp::gamma(x); }
    int sgn(const mppp::real& x) { if(nan_p(x)) return 0;return x.sgn(); }
    int getAccu(const mppp::real& x) { return Arb::precisionToDigits(x.get_prec()); }
    mppp::real isNan(const mppp::real& x) { return nan_p(x); }
    mppp::real isInf(const mppp::real& x) { return inf_p(x); }
    mppp::real NaN(int accu) { mppp::real r("0.0", Arb::digitsToPrecision(accu));set_nan(r); return r; }
    mppp::real Inf(int accu, bool negative) { mppp::real r("0.0", Arb::digitsToPrecision(accu));set_inf(r, negative); return r; }
    std::complex<mppp::real> pow(std::complex<mppp::real> a, std::complex<mppp::real> b) {
        if(a.imag().zero_p() && b.imag().zero_p()) {
            if(a.real().integer_p() && b.real().integer_p()) {
                return std::complex<mppp::real>(mppp::round(mppp::pow(a.real(),b.real())),mppp::real(0));
            }
            return std::complex<mppp::real>(mppp::pow(a.real(),b.real()),mppp::real(0));
        }
        return std::pow(a,b);

    }
    #endif
};
#pragma endregion
#pragma region ComputeCtx
ComputeCtx::ComputeCtx() {

}
Value ComputeCtx::getArgument(int id)const {
    if(id >= argValue.size()) throw string("internal argument out of bounds");
    return *(argValue.begin() + id);
}
void ComputeCtx::pushArgs(const ValList& args) {
    //Replace self referential replacement args with proper pointer
    for(auto it = argValue.begin();it != argValue.end();it++) {
        if((*it)->typeID() == Value::arg_t) {
            it->cast<Argument>()->id += args.size();
        }
    }
    //Push args to front
    for(int i = args.size() - 1;i >= 0;i--)
        argValue.push_front(args[i]);
}
void ComputeCtx::popArgs(const ValList& args) {
    //Delete front args
    for(int i = 0;i < args.size();i++)
        argValue.pop_front();
    //Replace self referential args with proper pointer
    for(auto it = argValue.begin();it != argValue.end();it++) {
        if((*it)->typeID() == Value::arg_t) {
            it->cast<Argument>()->id -= args.size();
        }
    }
}
void ComputeCtx::setVariable(const string& n, Value value) {
    std::map<string, std::vector<Value>>::iterator it = variables.find(n);
    if(it == variables.end()) variables[n] = { value.deepCopy() };
    else it->second.back() = value.deepCopy();
}
void ComputeCtx::defineVariable(const string& n, Value value) {
    std::map<string, std::vector<Value>>::iterator it = variables.find(n);
    if(it == variables.end()) {
        variables[n] = { value.deepCopy() };
    }
    else it->second.push_back(value.deepCopy());
}
//Erases all variables in the list
void ComputeCtx::undefineVariables(const std::vector<string>& vars) {
    for(int i = 0;i < vars.size();i++) {
        auto it = variables.find(vars[i]);
        if(it->second.size() == 1) variables.erase(it);
        else it->second.pop_back();
    }
}
Value ComputeCtx::getVariable(const string& name) {
    auto it = variables.find(name);
    if(it == variables.end()) return nullptr;
    return it->second.back();
}
#pragma endregion
#pragma region class Function
Function::Function() {

}
Function::Function(string argName, std::vector<string> inputs, std::map<Domain, Domain> domainMapping, std::map<Domain, fobj> functions) {
    name = argName;
    inputNames = inputs;
    funcs = functions;
    domainMap = domainMapping;
}
Function::Function(string argName, fobj func) {
    name = argName;
    funcs = std::map<Domain, fobj>{ {Domain(0),func } };
}
Value Function::operator()(ValList& input, ComputeCtx& ctx) {
    Domain cur(input);
    if(cur.sig == -1) {
        return std::make_shared<Tree>(Program::globalFunctionMap[name], std::forward<ValList>(input));
    }
    //Run immediately if current type is supported
    for(auto& impl : funcs) if(impl.first.match(cur)) {
        return impl.second(input, ctx, name);
    }
    //Map to new type if domain map supports that conversion
    for(auto& dm : domainMap) if(dm.first.match(cur)) {
        ValList convertedInput(input.size(), Value::zero);
        //Find new map and create converted input
        Domain newMap = domainMap[cur];
        for(int i = 0;i < input.size();i++)
            convertedInput[i] = input[i].convertTo(newMap.get(i));
        //return function result
        return funcs[newMap](convertedInput, ctx, name);
    }
    throw "Cannot run '" + name + "' with types " + cur.toString();
}
string Function::getName()const { return name; }
std::vector<string>& Function::getInputNames() { return inputNames; }
std::vector<Function::Domain> Function::getDomain()const {
    std::vector<Function::Domain> out;
    for(auto it = domainMap.begin();it != domainMap.end();it++) out.push_back(it->first);
    for(auto it = funcs.begin();it != funcs.end();it++) out.push_back(it->first);
    return out;
}
string Function::debugStr() {
    string out = "name: " + name + "\nargs: ";
    for(int i = 0;i < inputNames.size();i++) out += inputNames[i] + ", ";
    out += "\nInput maps: ";
    for(auto& m : domainMap) out += m.first.toString() + "->" + m.second.toString() + ", ";
    out += "\nDomains: ";
    for(auto& d : funcs) out += d.first.toString();
    return out + "\n";
}
bool Program::assertArgCount(int id, int count) {
    return Program::globalFunctions[id].assertArgCount(count);
}
void Program::buildFunctionNameMap() {
//Cache global functions in a map
    for(int i = 0;i < Program::globalFunctions.size();i++)
        Program::globalFunctionMap[Program::globalFunctions[i].getName()] = i;
}
#pragma endregion
#pragma region Function::Domain

#define dub 0b00000001
#define arb 0b00000010
#define vec_t 0b00000100
#define lmb 0b00001000
#define str_t 0b00010000
#define map_t 0b00100000
#define set 0b01000000
#define opt 0b10000000

#define all 0b01111111
Function::Domain::Domain(const ValList& input) {
    sig = 0;
    for(int i = 0;i < std::min(input.size(), size_t(4));i++) {
        int id = input[i]->typeID();
        if(id >= Value::tre_t) { sig = -1;return; }
        sig += (1 << (id - 1)) * (1 << (8 * i));
    }
}
bool Function::Domain::assertArgCount(int c)const {
    if(c >= minArgCount() && c <= maxArgCount()) return true;
    return false;
}
bool Function::assertArgCount(int count)const {
    for(auto it = funcs.begin();it != funcs.end();it++)
        if(it->first.assertArgCount(count)) return true;
    for(auto it = domainMap.begin();it != domainMap.end();it++)
        if(it->first.assertArgCount(count)) return true;
    return false;
}
int Function::Domain::maxArgCount()const {
    if(sig > 0xffffff) {
        //If final variable is optional and takes all types, allow many arguments
        if(get(3) == (all | opt)) return 1000;
        return 4;
    }
    if(sig > 0xffff) return 3;
    if(sig > 0xff) return 2;
    if(sig > 0) return 1;
    return 0;
}
int Function::Domain::minArgCount()const {
    if((sig & 0xff) == 0) return 0;
    else if(sig & 0x80) return 0;
    else if((sig & 0xff00) == 0) return 1;
    else if(sig & 0x8000) return 1;
    else if((sig & 0xff0000) == 0) return 2;
    else if(sig & 0x800000) return 2;
    else if((sig & 0xff000000) == 0) return 3;
    else if(sig & 0x80000000) return 3;
    else return 4;
}
string Function::Domain::toString()const {
    const static std::vector<string> names = { "num","arb","vec","lambda","str","map","set","opt" };
    string out = "(";
    for(int i = 0;i < maxArgCount();i++) {
        int sec = sig >> (i * 8) & 0xff;
        bool orBool = false;
        for(int x = 0;x < 8;x++) {
            if(sec >> x & 1) {
                if(orBool)out += "|";
                else orBool = true;
                out += names[x];
            }
        }
        if(i != maxArgCount() - 1) out += ",";
    }
    return out + ";" + std::to_string(sig) + ")";
}
#pragma endregion
#pragma region Preprocessor statements

#define aa D(arb,arb)
#define dd D(dub,dub)
#define vv D(vec_t,vec_t)
#define inp ValList input,ComputeCtx& ctx,const string& self
#define def(type,name,index) std::shared_ptr<type> name = input[index].cast<type>()
#define getV(type,index) input[index].cast<type>()
#define ret(type) return std::make_shared<type>

#define getN(index) getV(Number,index)->num
#define getArbN(index) getV(Arb,index)->num
#define getU(index) getV(Number,index)->unit
#define getArbU(index) getV(Arb,index)->unit
#define getS(index) getV(String,index)->str

#define D Function::Domain
#define samePrecision {D(dub,arb),aa},{D(arb,dub),aa}
#define BinVecApply {D(vec_t,dub|arb),applyVecLHS},{D(dub|arb,vec_t),applyVecRHS},{vv,applyBinVec}
#define UnaryVecApply {D(vec_t),applyToVector}

#pragma endregion
#pragma region Apply to vector lambdas
auto applyToVector = [](inp) {
    def(Vector, x, 0);
    std::shared_ptr<Vector> out;
    if(x.unique()) out = x;
    else out = std::make_shared<Vector>(x->size());
    for(int i = 0;i < x->size();i++)
        out->vec[i] = Program::computeGlobal(self, ValList{ x->vec[i] }, ctx);
    return Value(out);
};
auto applyVecLHS = [](inp) {
    def(Vector, x, 0);
    std::shared_ptr<Vector> out;
    if(x.unique()) out = x;
    else out = std::make_shared<Vector>(x->size());
    for(int i = 0;i < x->size();i++)
        out->vec[i] = Program::computeGlobal(self, ValList{ x->vec[i],input[1] }, ctx);
    return Value(out);
};
auto applyVecRHS = [](inp) {
    def(Vector, x, 1);
    std::shared_ptr<Vector> out;
    if(x.unique()) out = x;
    else out = std::make_shared<Vector>(x->size());
    for(int i = 0;i < x->size();i++)
        out->vec[i] = Program::computeGlobal(self, ValList{ input[0], x->vec[i] }, ctx);
    return Value(out);
};
auto applyBinVec = [](inp) {
    bool useSizeMin = false;
    if(self == "mul" || self == "div" || self == "mod" || self == "logb") useSizeMin = true;
    def(Vector, a, 0); def(Vector, b, 1);
    std::shared_ptr<Vector> out;
    if(a.unique()) out = a;
    else if(b.unique()) out = b;
    else out = std::make_shared<Vector>();
    int size = 0;
    if(useSizeMin) size = std::min(a->size(), b->size());
    else {
        size = std::max(a->size(), b->size());
        //Fill remaining elements with zeroes
        if(a->size() < b->size()) a->vec.resize(size, Value::zero);
        else if(b->size() < a->size()) b->vec.resize(size, Value::zero);
    }
    out->vec.resize(size, Value::zero);
    for(int i = 0;i < size;i++) out->vec[i] = Program::computeGlobal(self, ValList{ a->vec[i], b->vec[i] }, ctx);
    return Value(out);
};
#pragma endregion
Value Program::computeGlobal(string name, ValList input, ComputeCtx& ctx) {
    int index = globalFunctionMap[name];
    if(index == 0) throw "function " + name + " not found";
    return Program::globalFunctions[index](input, ctx);
}
using namespace std;
std::vector<Function> Program::globalFunctions = {
    #define Constant(name,...) Function(name,{},{},{{D(),[](inp) {return std::make_shared<Number>(__VA_ARGS__);}}})
    #define DefaultInp(id,value) if(input.size()<=id) {input.resize(id+1,Value::zero);input[id]=value;}

    #ifdef USE_ARB
    #define ConstantArb(name,...) Function("name",{},{},{{D(dub),[](inp) {return ret(Arb)(__VA_ARGS__);}}})
    #define UnaryWithUnit(name,formula,unitF,...) Function(name,{"x"},{}, {\
        {D(dub),[](inp) {using T=double;std::complex<T> num=getN(0);Unit unit=getU(0);ret(Number)(formula,unitF);}},\
        {D(arb),[](inp) {using T=mppp::real;std::complex<T> num = getArbN(0);Unit unit=getArbU(0);ret(Arb)(formula,unitF);}},\
        UnaryVecApply,__VA_ARGS__})
    #define DoubleArbTemplate(name,formula,...) Function(name,{"x"},{},{{D(dub),[](inp) {using T=double;using R=Number;std::complex<T> num=getN(0);Unit unit=getU(0);formula;}},{D(arb),[](inp) {using T=mppp::real;using R=Arb;std::complex<T> num=getArbN(0);Unit unit=getArbU(0);formula;}},BinVecApply,__VA_ARGS__})
    #define BinaryBaseTemplate(name,arg1,arg2,returnType,...) Function(name,{arg1,arg2},{samePrecision}, {\
        {dd,[](inp) {using T=double;using R=Number;std::complex<T> num1=getN(0);std::complex<T> num2=getN(1);Unit unit1=getU(0);Unit unit2=getU(1);return returnType;}},\
        {aa,[](inp) {using T=mppp::real;using R=Arb;std::complex<T> num1=getArbN(0);std::complex<T> num2=getArbN(1);Unit unit1=getArbU(0);Unit unit2=getArbU(1);return returnType;}},BinVecApply,__VA_ARGS__})
    #else
    #define UnaryWithUnit(name,formula,unitF,...) Function(name,{"x"},{}, {\
        {D(dub),[](inp) {using T=double;std::complex<T> num=getN(0);Unit unit=getU(0);ret(Number)(formula,unitF);}},\
        UnaryVecApply,__VA_ARGS__})
    #define DoubleArbTemplate(name,formula,...) Function(name,{"x"},{},{{D(dub),[](inp) {using T=double;using R=Number;std::complex<T> num=getN(0);Unit unit=getU(0);formula;}},UnaryVecApply,__VA_ARGS__})
    #define BinaryBaseTemplate(name,arg1,arg2,returnType,...) Function(name,{arg1,arg2},{samePrecision}, {\
        {dd,[](inp) {using T=double;using R=Number;std::complex<T> num1=getN(0);std::complex<T> num2=getN(1);Unit unit1=getU(0);Unit unit2=getU(1);return returnType;}},BinVecApply,__VA_ARGS__})
    #endif


    #define BinaryWithUnit(name,arg1,arg2,formula,unit,...) BinaryBaseTemplate(name,arg1,arg2,std::make_shared<R>(formula,unit),__VA_ARGS__)
    #define Binary(name,arg1,arg2,formula,...) BinaryWithUnit(name,arg1,arg2,formula,unit1+unit2,__VA_ARGS__)
    #define Unary(name,formula,...) UnaryWithUnit(name,formula,unit,__VA_ARGS__)
    #define Unary3(name,real,imag,unitF,...) UnaryWithUnit(name,std::complex(real,imag),unitF,__VA_ARGS__)

#pragma region Elementary Functions
    Unary("neg",-num),
    Binary("add","a","b",num1 + num2,{D(str_t,str_t),[](inp) {ret(String)(getS(0) + getS(1));}}),
    Binary("sub","a","b",num1 - num2),
    BinaryWithUnit("mul","a","b",num1 * num2,unit1 * unit2),
    BinaryWithUnit("div","a","b",num1 / num2,unit1 / unit2),
    BinaryWithUnit("pow","a","b",Math::pow(num1,num2),unit1 ^ double(num2.real())),
    Binary("mod","a","b",fmod(num1.real(),num2.real())),

    UnaryWithUnit("sqrt",sqrt(num),unit ^ 0.5),
    Unary("exp",exp(num)),
    Unary("ln",log(num)),
    Unary("log",log10(num)),

    Binary("logb","x","b",log(num1) / log(num2)),
    DoubleArbTemplate("gamma",if(num.imag() != 0) throw "Gamma function does not support complex";ret(R)(Math::gamma(num.real()),unit)),
    DoubleArbTemplate("factorial",if(num.imag() != 0) throw "Factorial function does not support complex";ret(R)(Math::gamma(num.real() + 1),unit)),
    DoubleArbTemplate("erf",if(num.imag() != 0) throw "Error function does not support complex";ret(R)(erf(num.real()),unit)),
#pragma endregion
#pragma region Trig
    Unary("sin",sin(num)),
    Unary("cos",cos(num)),
    Unary("tan",tan(num)),
    Unary("csc",std::complex<T>(1,0) / sin(num)),
    Unary("sec",std::complex<T>(1,0) / cos(num)),
    Unary("cot",std::complex<T>(1,0) / tan(num)),
    Unary("sinh",sinh(num)),
    Unary("cosh",cosh(num)),
    Unary("tanh",tanh(num)),
    Unary("asin",asin(num)),
    Unary("acos",acos(num)),
    Unary("atan",atan(num)),
    Unary("asinh",asinh(num)),
    Unary("acosh",acosh(num)),
    Unary("atanh",atanh(num)),
#pragma endregion
#pragma region Numeric Properties
    Unary("round",std::complex<T>(round(num.real()),round(num.imag()))),
    Unary("floor",std::complex<T>(floor(num.real()),floor(num.imag()))),
    Unary("ceil",std::complex<T>(ceil(num.real()),ceil(num.imag()))),
    UnaryWithUnit("getr",std::complex<T>(num.real(),0),0),
    UnaryWithUnit("geti",std::complex<T>(num.imag(),0),0),
    UnaryWithUnit("getu",std::complex<T>(1.0,0.0),unit),
    Binary("max","a","b",num1.real() > num2.real() ? num1 : num2,{D(vec_t),[](inp) {
        def(Vector,v,0);
        Value max = Value::zero;
        for(int i = 0;i < v->vec.size();i++) {
            if(v->vec[i]->typeID() == Value::num_t || v->vec[i]->typeID() == Value::arb_t)
                if(max->getR() < v->vec[i]->getR())
                    max = v->vec[i];
        }
        return max;
    }}),
    Binary("min","a","b",num1.real() > num2.real() ? num2 : num1,{D(vec_t),[](inp) {
        def(Vector,v,0);
        Value min = make_shared<Number>(INFINITY);
        for(int i = 0;i < v->vec.size();i++) {
            if(v->vec[i]->typeID() == Value::num_t || v->vec[i]->typeID() == Value::arb_t)
                if(min->getR() > v->vec[i]->getR())
                    min = v->vec[i];
        }
        return min;
    }}),
    Function("lerp",{"a","b","x"},{},{{D(dub | arb | vec_t,dub | arb | vec_t,dub | arb | vec_t),[](inp) {
        //return  (a*(1-f)) + (b*f)
        #define CptBin(name,input0,input1) Program::computeGlobal(name,ValList{input0,input1},ctx)
        return CptBin("add",CptBin("mul",input[0],CptBin("sub",Value::one,input[2])),CptBin("mul",input[1],input[2]));
    }}}),
    Binary("dist","a","b",hypot(num1.real() - num2.real(),num1.imag() - num2.imag()),{vv,[](inp) {
        //running total = d1^2 + d2^2 + d3^2 ....
        def(Vector,v1,0);def(Vector,v2,1);
        Value runningTotal = std::make_shared<Number>(0);
        for(int i = 0;i < std::max(v1->size(),v2->size());i++) {
            Value diff = CptBin("sub",v1->get(i),v2->get(i));
            runningTotal = CptBin("add",runningTotal,CptBin("lt",diff,diff));
        }
        return Program::computeGlobal("sqrt",ValList{runningTotal},ctx);
    }}),
    Unary("sgn", num / abs(num)),
    Unary("abs",abs(num)),
    Unary("arg",arg(num)),
    UnaryWithUnit("atan2",atan2(num.imag(),num.real()),unit,{D(dub | arb,dub | arb),[](inp) {
        if(input[0]->typeID() == Value::num_t && input[1]->typeID() == Value::num_t) {
            double i = getN(0).real(), r = getN(1).real();
            return Program::computeGlobal("atan2",ValList{make_shared<Number>(complex<double>(r,i))},ctx);
        }
        #ifdef USE_ARB
        else {
            mppp::real i = getArbN(0).real(), r = getArbN(1).real();
            return Program::computeGlobal("atan2",ValList{make_shared<Arb>(complex<mppp::real>(r,i))},ctx);
        }
        #endif
        return Value::zero;
    } }),
    Function("gcd",{"a","b"},{{D(dub | arb,dub | arb),D(dub,dub)}},{{D(dub,dub),[](inp) {
        uint64_t a = std::abs(input[0]->getR()) + 0.5;
        uint64_t b = std::abs(input[1]->getR()) + 0.5;
        if(a == 0 || b == 0) return Value::zero;
        //Euclidian algorithm for gcd
        while(b != 0) {
            std::swap(a,b);
            b = b % a;
        }
        return Value(std::make_shared<Number>(a));
    }}}),
    Function("lcm",{"a","b"},{{D(dub | arb,dub | arb),D(dub,dub)}},{{D(dub,dub),[](inp) {
        uint64_t a = std::abs(input[0]->getR()) + 0.5;
        uint64_t b = std::abs(input[1]->getR()) + 0.5;
        if(a == 0 || b == 0) return Value::zero;
        uint64_t max = std::max(a,b);
        uint64_t min = std::min(a,b);
        uint64_t cur = max;
        while(cur % min != 0) cur += max;
        return Value(std::make_shared<Number>(cur));
    }}}),
    Function("factors", {"x"}, {{D(arb),D(dub)}}, {{D(dub),[](inp) {
        uint64_t a = std::abs(input[0]->getR()) + 0.5;
        if(a == 0) return std::make_shared<Vector>(Value::zero);
        std::shared_ptr<Vector> out = std::make_shared<Vector>();
        Value two,three;
        while(a % 2 == 0) {
            if(two == nullptr) two = make_shared<Number>(2);
            out->vec.push_back(two);
            a /= 2;
        }
        while(a % 3 == 0) {
            if(three == nullptr) three = make_shared<Number>(3);
            out->vec.push_back(three);
            a /= 3;
        }
        //Prime factors other than 2 and 3 can be written as either 6n-1 or 6n+1
        for(uint64_t n6 = 6;a != 1;n6 += 6) {
            Value n6m1,n6p1;
            while(a % (n6 - 1) == 0) {
                if(n6m1 == nullptr) n6m1 = std::make_shared<Number>(n6 - 1);
                out->vec.push_back(n6m1);
                a /= n6 - 1;
            }
            while(a % (n6 + 1) == 0) {
                if(n6p1 == nullptr) n6p1 = std::make_shared<Number>(n6 + 1);
                out->vec.push_back(n6p1);
                a /= n6 + 1;
            }
        }
        return out;
    }}}),
    #pragma endregion
#pragma region Binary logic
    Function("equal",{"a","b"},{},{{D(all,all),[](inp) {return input[0] == input[1] ? Value::one : Value::zero;}}}),
    Function("not_equal",{"a","b"},{},{{D(all,all),[](inp) {return input[0] == input[1] ? Value::zero : Value::one;}}}),
    BinaryBaseTemplate("lt", "a","b", num1.real() < num2.real() ? Value::one : Value::zero),
    BinaryBaseTemplate("gt", "a","b", num1.real() > num2.real() ? Value::one : Value::zero),
    BinaryBaseTemplate("lt_equal","a","b", (num1.real() < num2.real() || num1 == num2) ? Value::one : Value::zero),
    BinaryBaseTemplate("gt_equal","a","b", (num1.real() < num2.real() || num1 == num2) ? Value::one : Value::zero),
    #define BitwiseOperator(name,operat) Function(name,{"a","b"},{},{{D(dub | arb,dub | arb),[](inp) {uint64_t a = std::abs(input[0]->getR() + 0.5), b = std::abs(input[1]->getR() + 0.5);return std::make_shared<Number>(a operat b);}},BinVecApply})
    Function("not",{"x"},{{D(arb),D(dub)}},{{D(dub),[](inp) {return input[0]->getR() == 0 ? Value::one : Value::zero;}}}),
    BitwiseOperator("or", | ),
    BitwiseOperator("and",&),
    BitwiseOperator("xor",^),
    BitwiseOperator("ls", << ),
    BitwiseOperator("rs", >> ),
#pragma endregion
#pragma region Constants
    Constant("true",1),
    Constant("false",0),
    Constant("i",0,1.0),
    Constant("pi",3.14159265358979323),
    #define downscale {D(arb),D(dub)}
    #ifdef USE_ARB
    Function("arb_pi",{"prec"},{},{{D(dub),[](inp) {double prec = input[0]->getR();if(Program::smallCompute) prec = std::min(prec,50.0);ret(Arb)(mppp::real_pi(Arb::digitsToPrecision(prec)));}}}),
    Function("arb_e",{"prec"},{},{{D(dub),[](inp) {double prec = input[0]->getR();if(Program::smallCompute) prec = std::min(prec,50.0);ret(Arb)(mppp::exp(mppp::real(1.0,Arb::digitsToPrecision(prec))));}}}),
    Function("arb_rand",{"prec"},{},{{D(dub),[](inp) {
        double prec = input[0]->getR();
        if(Program::smallCompute) prec = std::min(prec,50.0);
        mpfr_prec_t p = Arb::digitsToPrecision(prec);
        mppp::real out(0,p);
        int binDigits = p;
        int i = 0;
        if(RAND_MAX >= 0xffffffff) while(i * 32 < binDigits) {
            i++;
            mppp::real cur{(unsigned long)(0xffffffff & rand()),mpfr_exp_t(-i * 32),p};
            out += cur;
        }
        else while(i * 12 < binDigits) {
            i++;
            mppp::real cur{(unsigned long)(0xfff & rand()),mpfr_exp_t(-i * 12),p};
            out += cur;
        }
        return std::make_shared<Arb>(out);
    }}}),
    #endif
    Function("rand",{},{},{{D(),[](inp) {double upLim = RAND_MAX + 1.;ret(Number)(rand() / upLim + rand() / upLim / upLim);}}}),
    Function("srand",{"seed"},{downscale},{{D(dub),[](inp) { srand(getN(0).real() * 100); ret(Number)(0); }}}),
    Constant("e",2.71828182845904523),
    Constant("undefined",NAN),
    Constant("inf",INFINITY),
    Constant("histlen",Program::history.size()),
    Function("ans",{"index"},{},{{D(dub | arb | opt),[](inp) {
        int index = Program::history.size() - 1;
        if(input.size() == 1) index = input[0]->getR();
        if(index < 0) index += Program::history.size();
        if(index < 0 || index >= Program::history.size()) return Value::zero;
        return Program::history[index];
    }}}),
#pragma endregion
#pragma region Functional
    Function("run", { "func","..." }, {}, {{D(lmb,all | opt,all | opt,all | opt),[](inp) {
        def(Lambda,func,0);
        ValList args(func->inputNames.size(),Value::zero);
        for(int i = 0;i < std::min(input.size() - 1,args.size());i++) args[i] = input[i + 1];
        return (*func)(args,ctx);
    }}}),
    Function("apply",{"func","args"},{},{{D(lmb,vec_t),[](inp) {
        def(Vector,v,1);
        ValList toRun = ValList{input[0]};
        for(int i = 0;i < v->size();i++) toRun.push_back(v->vec[i]);
        Value out = Program::computeGlobal("run",toRun,ctx);
        return out;
    }},{D(str_t,vec_t),[](inp) {
        return Program::computeGlobal(getV(String,0)->str,getV(Vector,1)->vec,ctx);
    }}}),
    Function("sum",{"func","begin","end","step"},{},{{D(lmb,arb | dub,arb | dub,arb | dub | opt), [](inp) {
        def(Lambda,func,0);
        double begin = input[1]->getR(), end = input[2]->getR();
        double step = 1.0;
        if(input.size() == 4) step = input[3]->getR();
        if(Program::smallCompute) if((end - begin) / step > 10.0) end = begin + step * 10.0;
        shared_ptr<Number> n = make_shared<Number>(0);
        ValList lambdaInput{n};
        Value out = make_shared<Number>(0);
        for(double index = begin;index <= end;index += step) {
            n->num = {index,0.0};
            out = Program::computeGlobal("add",ValList{out,(*func)(lambdaInput,ctx)},ctx);
        }
        return out;
    }},{D(vec_t),[](inp) {
        def(Vector,vec,0);
        Value out = Value::zero;
        for(int i = 0;i < vec->vec.size();i++)
            out = Program::computeGlobal("add",ValList{out,vec->vec[i]},ctx);
        return out;
    }}}),
    Function("product",{"func","begin","end","step"},{},{{D(lmb,arb | dub,arb | dub,arb | dub | opt), [](inp) {
        def(Lambda,func,0);
        double begin = input[1]->getR(), end = input[2]->getR();
        double step = 1.0;
        if(input.size() == 4) step = input[3]->getR();
        if(Program::smallCompute) if((end - begin) / step > 10.0) end = begin + step * 10.0;
        shared_ptr<Number> n = make_shared<Number>(0);
        ValList lambdaInput{n};
        Value out = make_shared<Number>(1);
        for(double index = begin;index <= end;index += step) {
            n->num = {index,0.0};
            out = Program::computeGlobal("mul",ValList{out,(*func)(lambdaInput,ctx)},ctx);
        }
        return out;
    }},{D(vec_t),[](inp) {
        def(Vector,vec,0);
        Value out = std::make_shared<Number>(1);
        for(int i = 0;i < vec->vec.size();i++)
            out = Program::computeGlobal("mul",ValList{out,vec->vec[i]},ctx);
        return out;
    }}}),
    Function("infinite_sum",{"func"},{},{{D(lmb,dub | arb | opt),[](inp) {
        def(Lambda,func,0);
        DefaultInp(1,Value::zero);
        Value out = Value::zero;
        Value old = out;
        shared_ptr<Number> n = make_shared<Number>(input[1]->getR());
        ValList lambdaInput{n};
        while(true) {
            Value inc = (*func)(lambdaInput,ctx);
            out = Program::computeGlobal("add",{out,inc},ctx);
            n->num = {n->num.real() + 1,0};
            if(n->num.real() == 100000) return out;
            if(Program::smallCompute) if(n->num.real() == 20) return out;
            if(out == old) break;
            old = out;
        }
        return out;
    }}}),
    Function("dint",{"func","lower","uppper"},{},{{D(lmb,dub,dub),[](inp) {
        def(Lambda,func,0);
        double lower = input[1]->getR();
        double upper = input[2]->getR();
        std::map<uint32_t,double> outputs;
        uint32_t divisionSize = 0x40000000;
        double calc = 0, prevCalc = INFINITY;
        shared_ptr<Number> x = make_shared<Number>(0);
        ValList lambdaInput{x};
        x->num = {lower,0}; outputs[0] = (*func)(lambdaInput,ctx)->getR();
        x->num = {upper,0}; outputs[0xffffffff] = (*func)(lambdaInput,ctx)->getR();
        while(true) {
            int i = 0;
            //Add first and last
            calc = outputs[0] + outputs[0xffffffff];
            for(uint32_t ind = divisionSize;ind != 0;ind += divisionSize) {
                //Add elements if not found
                if(outputs.find(ind) == outputs.end()) {
                    x->num = {lower + (upper - lower) * (ind / double(0xffffffff)),0};
                    outputs[ind] = (*func)(lambdaInput,ctx)->getR();
                }
                //Add intermediate elements
                if(i % 2 == 0) calc += 4 * outputs[ind];
                else calc += 2 * outputs[ind];
                i++;
            }
            //Multiply by deltax/3
            calc *= divisionSize / double(0x100000000) * (upper - lower) / 3.0;
            //Divide deltax by two
            divisionSize >>= 1;
            //If deltax is really low, end loop
            if(divisionSize <= 0x0004ffff) break;
            prevCalc = calc;
        }
        return Value(std::make_shared<Number>(calc));

    }}}),
    Function("derivative",{"exp","wrt"},{},{{D(lmb,dub | arb | opt),[](inp) {
        def(Lambda,func,0);
        DefaultInp(1,Value::zero);
        ValList argDx(func->inputNames.size(),Value::zero);
        int wrt = input[0]->getR();
        if(wrt >= func->inputNames.size()) wrt = func->inputNames.size() - 1;
        if(wrt < 0) wrt = 0;
        argDx[wrt] = Value::one;
        Value tr = func->func.derivative(argDx).simplify();
        return make_shared<Lambda>(func->inputNames,tr);
    }}}),
    Function("simplify",{"xpr"},{},{{D(all),[](inp) {
        return input[0].simplify();
    }}}),
//    Function("infinite_sum", { "func" }, TypeDomain(lbm), [](vector<Value> input) {
//        ret Value(0.0);
//    }),
//    Function("dint", { "func","a","b" }, TypeDomain({ lbm,com,com }), [](vector<Value> input) {
//        ret Value(0.0);
//    }),
//    Function("derivative", { "func","wrt" }, TypeDomain({ lbm,com }, 2, 1), [](vector<Value> input) {
//        ret Value(0.0);
//    }),
//    Function("approx_derivative", { "func","point" }, TypeDomain(lbm, com), [](vector<Value> input) {
//        ret Value(0.0);
//    }),
//    Function("range", { "func" }, TypeDomain({ lbm,set }, 2, 1), [](vector<Value> input) {
//        ret Value(0.0);
//    }),
//    Function("domain", { "func" }, TypeDomain(lbm), [](vector<Value> input) {
//        ret Value(0.0);
//    }),
#pragma endregion
#pragma region Vector
    Function("length", {"obj"}, {}, {{D(vec_t),[](inp) {
        def(Vector,v,0); ret(Number)(v->size());
    }},{D(str_t),[](inp) {
        def(String,s,0); ret(Number)(s->str.length());
    }},{D(map_t),[](inp) {
        ret(Number)(input[0].cast<Map>()->size());
    }}}),
    Function("magnitude",{"vec"},{}, {{D(vec_t),[](inp) {
        def(Vector,v,0);
        Value out = make_shared<Number>(0);
        Value two = make_shared<Number>(2);
        for(int i = 0;i < v->size();i++) {
            out = Program::computeGlobal("add",ValList{out,Program::computeGlobal("pow",ValList{v->vec[i],two},ctx)},ctx);
        }
        return Program::computeGlobal("sqrt",ValList{out},ctx);
    }}}),
    Function("normalize",{"vec"},{},{{D(vec_t),[](inp) {
        return Program::computeGlobal("div",ValList{input[0],Program::computeGlobal("magnitude",ValList{input[0]},ctx)},ctx);
    }}}),
    Function("get",{"map","key"},{},{{D(vec_t,dub | arb),[](inp) {
        def(Vector,v,0);
        int index = input[1]->getR();
        if(index < 0 || index >= v->size()) return Value::zero;
        return v->vec[index];
    }},{D(map_t,all),[](inp) {
        def(Map,m,0);
        return (*m)[input[1]];
    }}}),
    Function("fill",{"func","count"},{},{{D(lmb,arb | dub),[](inp) {
        def(Lambda,func,0);
        int count = input[1]->getR();
        if(Program::smallCompute) if(count > 10) count = 10;
        shared_ptr<Vector> out = make_shared<Vector>();
        shared_ptr<Number> index = make_shared<Number>(0);
        ValList lambdaInput = ValList{index};
        for(int i = 0;i < count;i++) {
            index->num = {double(i),0.0};
            out->vec.push_back((*func)(lambdaInput,ctx));
        }
        return out;
    }}}),
    Function("map_vector",{"map","func"},{},{{D(vec_t,lmb),[](inp) {
        def(Vector,v,0);def(Lambda,func,1);
        shared_ptr<Vector> out = make_shared<Vector>();
        shared_ptr<Number> index = make_shared<Number>(0);
        ValList lambdaInput{nullptr,index};
        for(int i = 0;i < v->size();i++) {
            lambdaInput[0] = v->vec[i];
            index->num = {double(i),0};
            out->vec.push_back((*func)(lambdaInput,ctx));
        }
        return out;
    }}}),
    Function("every",{"vec","func"},{},{{D(vec_t,lmb),[](inp) {
        def(Vector,v,0);def(Lambda,func,1);
        for(int i = 0;i < v->vec.size();i++) {
            if((*func)(ValList{v->vec[i]},ctx) == Value::zero) return Value::zero;
        }
        return Value::one;
    }}}),
    Function("concat",{"a","b"},{},{{D(vec_t,vec_t),[](inp) {
        def(Vector,a,0);def(Vector,b,1);
        shared_ptr<Vector> out = make_shared<Vector>();
        for(int i = 0;i < a->size();i++) out->vec.push_back(a->vec[i]);
        for(int i = 0;i < b->size();i++) out->vec.push_back(b->vec[i]);
        return Value(out);
    }},{D(map_t,map_t),[](inp) {
        def(Map,a,0);def(Map,b,1);
        shared_ptr<Map> out = make_shared<Map>();
        for(auto p : a->getMapObj()) out->append(p.first,p.second);
        for(auto p : b->getMapObj()) out->append(p.first,p.second);
        return out;
    }}}),
    Function("sort",{"vec","comp"},{},{{D(vec_t),[](inp) {
        def(Vector,v,0);
        auto compare = [&ctx = ctx](Value a,Value b) {
            return Program::computeGlobal("lt",ValList{a,b},ctx)->getR();
        };
        shared_ptr<Vector> out = make_shared<Vector>(std::forward<ValList>(v->vec));
        std::sort(out->vec.begin(),out->vec.end(),compare);
        return out;
    }},{D(vec_t,lmb),[](inp) {
        def(Vector,v,0); def(Lambda,func,1);
        auto compare = [&ctx = ctx,func = func](Value a,Value b) {
            return (*func)(ValList{a,b},ctx)->getR();
        };
        shared_ptr<Vector> out = make_shared<Vector>(std::forward<ValList>(v->vec));
        std::sort(out->vec.begin(),out->vec.end(),compare);
        return out;
    }}}),
#pragma endregion
#pragma region String
    Function("eval",{"str"},{},{{D(str_t),[](inp) {
        def(String,str,0);
        return Expression::evaluate(str->str);
    }}}),
    Function("error",{"str"},{},{{D(str_t),[](inp) {
        def(String,str,0);
        throw str;
        return Value::zero;
    }}}),
    Function("substr",{"str","begin","len"},{},{{D(str_t,dub | arb,dub | arb | opt),[](inp) {
        def(String,str,0);
        int start = input[1]->getR();
        if(start < 0) throw "Start of substring cannot be negative";
        if(input.size() == 2)
            return make_shared<String>(str->str.substr(start));
        int len = input[2]->getR();
        if(start < 0) throw "Start of substring cannot be negative";
        return make_shared<String>(str->str.substr(start,len));
    }}}),
    Function("lowercase",{"str"},{},{{D(str_t),[](inp) {
        def(String,in,0);
        string out(in->str.length(),' ');
        for(int i = 0;i < in->str.length();i++)
            out[i] = std::tolower(in->str[i]);
        return make_shared<String>(out);
    }}}),
    Function("uppercase",{"str"},{},{{D(str_t),[](inp) {
        def(String,in,0);
        string out(in->str.length(),' ');
        for(int i = 0;i < in->str.length();i++)
            out[i] = std::toupper(in->str[i]);
        return make_shared<String>(out);
    }}}),
    Function("indexof",{"str","query"},{},{{D(str_t,str_t),[](inp) {
        def(String,str,0);def(String,find,1);
        int index = str->str.find(find->str);
        if(index == string::npos) return make_shared<Number>(-1);
        else return make_shared<Number>(double(index));
    }}}),
    Function("replace",{"str","find","rep"},{},{{D(str_t,str_t,str_t),[](inp) {
        def(String,str,0);def(String,find,1);def(String,rep,2);
        int findLen = find->str.length();
        int position = -1;
        vector<int> positions;
        while(true) {
            position = str->str.find(find->str,position + 1);
            if(position == string::npos) break;
            positions.push_back(position);
        }
        positions.push_back(str->str.length());
        string out = str->str.substr(0,positions[0]);
        for(int i = 0;i < positions.size() - 1;i++) {
            out += rep->str;
            out += str->str.substr(positions[i] + findLen,positions[i + 1] - positions[i] - findLen);
        }
        return make_shared<String>(out);
    }}}),
//    Function("print", { "str" }, TypeDomain(str), [](vector<Value> input) {
//        ret Value(0.0);
//    }),
#pragma endregion
#pragma region Conversion and variables
//    Function("getlocal", { "name" }, TypeDomain(str), [](vector<Value> input) {
//        ret Value(0.0);
//    }),
//    Function("setlocal", { "name","val" }, TypeDomain(str, all), [](vector<Value> input) {
//        ret Value(0.0);
//    }),
//    Function("getglobal", { "name" }, TypeDomain(str), [](vector<Value> input) {
//        ret Value(0.0);
//    }),
//    Function("setglobal", { "name","val" }, TypeDomain(str, all), [](vector<Value> input) {
//        ret Value(0.0);
//    }),
#undef vec_t
#undef map_t
#undef str_t
    Function("tonumber",{"val"},{},{{D(all),[](inp) {return input[0].convertTo(Value::num_t);}}}),
    Function("toarb",{"val"},{},{{D(all),[](inp) {return input[0].convertTo(Value::arb_t);}}}),
    Function("tovec",{"val"},{},{{D(all),[](inp) {return input[0].convertTo(Value::vec_t);}}}),
    Function("tomap",{"val"},{},{{D(all),[](inp) {return input[0].convertTo(Value::map_t);}}}),
    Function("tostring",{"val"},{},{{D(all),[](inp) {return input[0].convertTo(Value::str_t);}}}),
    Function("tolambda",{"val"},{},{{D(all),[](inp) {return input[0].convertTo(Value::lmb_t);}}}),
    Function("typeof",{"val"},{},{{D(all),[](inp) {return make_shared<Number>(input[0]->typeID());}}}),
#pragma endregion

};