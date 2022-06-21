#include "_header.hpp"

struct Function::Domain {
    //Contains binary data on support for first four arguments
    uint32_t sig;
    Domain(int a = 0, int b = 0, int c = 0, int d = 0) { sig = a + b * 0x100 + c * 0x10000 + d * 0x1000000; }
    Domain(const ValList& input);
    //Whether a matches this
    inline bool match(Domain a)const { return (a.sig & sig) == a.sig; }
    //Only for sorting
    bool operator<(const Domain& a)const { return sig < a.sig; }
    //Get
    int get(int id)const { return (sig >> (id * 8)) & 0b11111111; }
    //Returns domain as list (num,vec,map...)
    string toString()const;
    //Asserts whether arg count is supported
    bool assertArgCount(int c)const;
    //Gets maximum number of args supported
    int maxArgCount()const;
    //Gets minimum number of args supported
    int minArgCount()const;
};

bool Program::assertArgCount(int id, int count) {
    return Program::globalFunctions[id].assertArgCount(count);
}
void Program::buildFunctionNameMap() {
//Cache global functions in a map
    for(int i = 0;i < Program::globalFunctions.size();i++)
        Program::globalFunctionMap[Program::globalFunctions[i].getName()] = i;

}
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
#endif
};
#pragma region ComputeCtx
ComputeCtx::ComputeCtx() {

}
ValPtr ComputeCtx::getLocal(const string& name)const { return local.at(name); }
void ComputeCtx::setLocal(const string& name, ValPtr val) { local[name] = val; }
void ComputeCtx::eraseLocal(const std::vector<string>& names) {
    //Iterates backwards so the erased object is always a leaf
    for(int i = names.size() - 1;i >= 0;i--) local.erase(names[i]);
}
ValPtr ComputeCtx::getArgument(int id)const { return *(argValue.begin() + id); }
void ComputeCtx::pushArgs(const ValList& args) {
    //Replace self referential replacement args with proper pointer
    for(auto it = argValue.begin();it != argValue.end();it++) {
        if((*it)->typeID() == 9) {
            std::static_pointer_cast<Argument>(*it)->id += args.size();
        }
    }
    for(int i = args.size() - 1;i >= 0;i--)
        argValue.push_front(args[i]);
}
void ComputeCtx::popArgs(const ValList& args) {
    for(int i = 0;i < args.size();i++)
        argValue.pop_front();
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
ValPtr Function::operator()(ValList& input, ComputeCtx& ctx) {
    Domain cur(input);
    if(cur.sig == -1) {
        return std::make_shared<Tree>(Program::globalFunctionMap[name], std::forward<ValList>(input));
    }
    //Run immediately if current type is supported
    for(auto& impl : funcs) if(impl.first.match(cur)) {
        return impl.second(input, ctx);
    }
    //Map to new type if domain map supports that conversion
    for(auto& dm : domainMap) if(dm.first.match(cur)) {
        ValList convertedInput(input.size(),Value::zero);
        //Find new map and create converted input
        Domain newMap = domainMap[cur];
        for(int i = 0;i < input.size();i++)
            convertedInput[i] = Value::convert(input[i], newMap.get(i));
        //return function result
        return funcs[newMap](convertedInput, ctx);
    }
    throw "Cannot run '" + name + "' with types " + cur.toString();
}
string Function::getName()const { return name; }
std::vector<string>& Function::getInputNames() { return inputNames; }
string Function::debugStr() {
    string out = "name: " + name + "\nargs: ";
    for(int i = 0;i < inputNames.size();i++) out += inputNames[i] + ", ";
    out += "\nInput maps: ";
    for(auto& m : domainMap) out += m.first.toString() + "->" + m.second.toString() + ", ";
    out += "\nDomains: ";
    for(auto& d : funcs) out += d.first.toString();
    return out + "\n";
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
    for(int i = 0;i < input.size();i++) {
        int id = input[i]->typeID();
        if(id > 7) { sig = -1;return; }
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
        if(get(3) == all | opt) return 1000;
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
#define inp ValList input,ComputeCtx& ctx

#pragma endregion
ValPtr Program::computeGlobal(string name, ValList input, ComputeCtx& ctx) {
    int index = globalFunctionMap[name];
    if(index == 0) throw "function " + name + " not found";
    return Program::globalFunctions[index](input, ctx);
}

#define def(type,name,index) std::shared_ptr<type> name = std::dynamic_pointer_cast<type>(input[index])
#define getV(type,index) std::dynamic_pointer_cast<type>(input[index])
#define ret(type) return std::make_shared<type>

#define getN(index) getV(Number,index)->num
#define getArbN(index) getV(Arb,index)->num
#define getU(index) getV(Number,index)->unit
#define getArbU(index) getV(Arb,index)->unit
#define getS(index) getV(String,index)->str

#define D Function::Domain
#define samePrecision {D(dub,arb),aa},{D(arb,dub),aa}
#define BinVecApply(name) {D(vec_t,dub|arb),applyVecLHS(name)},{D(dub|arb,vec_t),applyVecRHS(name)}
#define Apply2VecMax(name) {vv,applyBinVec(name,[](int a,int b){return std::max(a,b);})}
#define Apply2VecMin(name) {vv,applyBinVec(name,[](int a,int b){return std::min(a,b);})}
#define UnaryVecApply(name) {D(vec_t),applyToVector(name)}
class Function;
#pragma region Apply to vector lambdas
Function::fobj applyToVector(string name) {
    return [name = name](inp) {
        def(Vector, x, 0);
        std::shared_ptr<Vector> out;
        if(x.unique()) out = x;
        else out = std::make_shared<Vector>(x->size());
        for(int i = 0;i < x->size();i++) {
            out->vec[i] = Program::computeGlobal(name, ValList{ x->vec[i] }, ctx);
        }
        return out;
    };
}
Function::fobj applyVecLHS(string name) {
    return [name = name](inp) {
        def(Vector, x, 0);
        std::shared_ptr<Vector> out;
        if(x.unique()) out = x;
        else out = std::make_shared<Vector>(x->size());
        for(int i = 0;i < x->size();i++) {
            out->vec[i] = Program::computeGlobal(name, ValList{ x->vec[i],input[1] }, ctx);
        }
        return out;
    };
}
Function::fobj applyVecRHS(string name) {
    return [name = name](inp) {
        def(Vector, x, 1);
        std::shared_ptr<Vector> out;
        if(x.unique()) out = x;
        else out = std::make_shared<Vector>(x->size());
        for(int i = 0;i < x->size();i++) {
            out->vec[i] = Program::computeGlobal(name, ValList{ input[0], x->vec[i] }, ctx);
        }
        return out;
    };
}
Function::fobj applyBinVec(string name, std::function<int(int, int)> maxormin) {
    return [name = name, maxormin = maxormin](inp) {
        def(Vector, a, 0); def(Vector, b, 1);
        std::shared_ptr<Vector> out;
        if(a.unique()) out = a;
        else if(b.unique()) out = b;
        else out = std::make_shared<Vector>();
        int size = maxormin(a->size(), b->size());
        out->vec.resize(size);
        for(int i = 0;i < size;i++) out->vec[i] = Program::computeGlobal(name, ValList{ a->vec[i], b->vec[i] }, ctx);
        return out;
    };
}
#pragma endregion

using namespace std;
std::vector<Function> Program::globalFunctions = {
    #define Constant(name,...) Function(name,{},{},{{D(),[](inp) {return std::make_shared<Number>(__VA_ARGS__);}}})
    #define default(id,value) if(input.size()<=id) {input.resize(id);input[id]=value;}

    #ifdef USE_ARB
    #define ConstantArb(name,...) Function("name",{},{},{{D(dub),[](inp) {return ret(Arb)(__VA_ARGS__);}}})
    #define UnaryWithUnit(name,formula,unitF,...) Function(name,{"x"},{}, {\
        {D(dub),[](inp) {using T=double;std::complex<T> num=getN(0);Unit unit=getU(0);ret(Number)(formula,unitF);}},\
        {D(arb),[](inp) {using T=mppp::real;std::complex<T> num = getArbN(0);Unit unit=getArbU(0);ret(Arb)(formula,unitF);}},\
        UnaryVecApply(name),__VA_ARGS__})
    #define DoubleArbTemplate(name,formula,...) Function(name,{"x"},{},{{D(dub),[](inp) {using T=double;using R=Number;std::complex<T> num=getN(0);Unit unit=getU(0);formula;}},{D(arb),[](inp) {using T=mppp::real;using R=Arb;std::complex<T> num=getArbN(0);Unit unit=getArbU(0);formula;}},UnaryVecApply(name),__VA_ARGS__})
    #define BinaryWithUnit(name,arg1,arg2,formula,unit,...) Function(name,{arg1,arg2},{samePrecision}, {\
        {dd,[](inp) {using T=double;std::complex<T> num1=getN(0);std::complex<T> num2=getN(1);Unit unit1=getU(0);Unit unit2=getU(1);ret(Number)(formula, unit);}},\
        {aa,[](inp) {using T=mppp::real;std::complex<T> num1=getArbN(0);std::complex<T> num2=getArbN(1);Unit unit1=getArbU(0);Unit unit2=getArbU(1);ret(Arb)(formula, unit);}},BinVecApply(name),__VA_ARGS__})
    #else
    #define UnaryWithUnit(name,formula,unitF,...) Function(name,{"x"},{}, {\
        {D(dub),[](inp) {using T=double;std::complex<T> num=getN(0);Unit unit=getU(0);ret(Number)(formula,unitF);}},\
        UnaryVecApply(name),__VA_ARGS__})
    #define DoubleArbTemplate(name,formula,...) Function(name,{"x"},{},{{D(dub),[](inp) {using T=double;using R=Number;std::complex<T> num=getN(0);Unit unit=getU(0);formula;}},UnaryVecApply(name),__VA_ARGS__})
    #define BinaryWithUnit(name,arg1,arg2,formula,unit,...) Function(name,{arg1,arg2},{samePrecision}, {\
        {dd,[](inp) {using T=double;std::complex<T> num1=getN(0);std::complex<T> num2=getN(1);Unit unit1=getU(0);Unit unit2=getU(1);ret(Number)(formula, unit);}},BinVecApply(name),__VA_ARGS__})
    #endif


    #define Binary(name,arg1,arg2,formula,...) BinaryWithUnit(name,arg1,arg2,formula,unit1+unit2,__VA_ARGS__)
    #define Unary(name,formula,...) UnaryWithUnit(name,formula,unit,__VA_ARGS__)
    #define Unary3(name,real,imag,unitF,...) UnaryWithUnit(name,std::complex(real,imag),unitF,__VA_ARGS__)

#pragma region Elementary Functions
    Function("neg",{"a"},{},{{D(all),[](inp) {
        ret(Number)(0.0);
    }}}),
    Binary("add","a","b",num1 + num2,{D(str_t,str_t),[](inp) {ret(String)(getS(0) + getS(1));}}),
    Binary("sub","a","b",num1 - num2),
    BinaryWithUnit("mult","a","b",num1 * num2,unit1 * unit2),
    BinaryWithUnit("div","a","b",num1 / num2,unit1 / unit2),
    Binary("mod","a","b",fmod(num1.real(),num2.real())),

    UnaryWithUnit("sqrt",sqrt(num),unit ^ 0.5),
    Unary("exp",exp(num)),
    Unary("ln",log(num)),
    Unary("log",log10(num)),

    Binary("logb","x","b",log(num1) / log(num2),BinVecApply("logb"),Apply2VecMin("logb")),
    DoubleArbTemplate("gamma",if(num.imag() != 0) throw "Gamma function does not support complex";ret(R)(Math::gamma(num.real()),unit)),
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
    Unary("getr",std::complex<T>(num.real(),0)),
    Unary("geti",std::complex<T>(num.imag(),1)),
    UnaryWithUnit("getu",std::complex<T>(1.0,0.0),unit),
    Binary("max","a","b",num1.real() > num2.real() ? num1 : num2,Apply2VecMax("max")),
    Binary("min","a","b",num1.real() > num2.real() ? num2 : num1,Apply2VecMax("min")),
    Function("lerp",{"a","b","x"},{},{{D(dub | arb | vec_t,dub | arb | vec_t,dub | arb | vec_t),[](inp) {
        //return  (a*(1-f)) + (b*f)
        #define CptBin(name,input0,input1) Program::computeGlobal(name,ValList{input0,input1},ctx)
        return CptBin("add",CptBin("mult",input[0],CptBin("sub",std::make_shared<Number>(1),input[2])),CptBin("mult",input[1],input[2]));
    }}}),
    Binary("dist","a","b",hypot(num1.real() - num2.real(),num1.imag() - num2.imag()),{vv,[](inp) {
        //running total = d1^2 + d2^2 + d3^2 ....
        def(Vector,v1,0);def(Vector,v2,1);
        ValPtr runningTotal = std::make_shared<Number>(0);
        for(int i = 0;i < std::max(v1->size(),v2->size());i++) {
            ValPtr diff = CptBin("sub",v1->get(i),v2->get(i));
            runningTotal = CptBin("add",runningTotal,CptBin("mult",diff,diff));
        }
        return Program::computeGlobal("sqrt",ValList{runningTotal},ctx);
    }}),
    Unary("sgn", num / abs(num)),
    Unary("abs",abs(num)),
    Unary("arg",arg(num)),
    UnaryWithUnit("atan2",atan2(num.imag(),num.real()),unit),
    #pragma endregion
    #pragma region Binary logic
    Function("equal",{"a","b"},{},{{D(all,all),[](inp) {
        if(*(input[0]) == input[1]) ret(Number)(1);
        else ret(Number)(0);
    }}}),
    Function("not_equal",{"a","b"},{},{{D(all,all),[](inp) {
        if(*(input[0]) == input[1]) ret(Number)(0);
        else ret(Number)(1);
    }}}),
//    Function("lt", { "a","b" }, TypeDomain(nmr | str, nmr | str), [](vector<Value> input) {
//        ret Value(0.0);
//    }),
//    Function("gt", { "a","b" }, TypeDomain(nmr | str, nmr | str), [](vector<Value> input) {
//        ret Value(0.0);
//    }),
//    Function("lt_equal", { "a","b" }, TypeDomain(nmr | str, nmr | str), [](vector<Value> input) {
//        ret Value(0.0);
//    }),
//    Function("gt_equal", { "a","b" }, TypeDomain(nmr | str, nmr | str), [](vector<Value> input) {
//        ret Value(0.0);
//    }),
//    Function("not", { "x" }, TypeDomain(nmr | set), [](vector<Value> input) {
//        ret Value(0.0);
//    }),
//    Function("or", { "a","b" }, TypeDomain(nmr, nmr), [](vector<Value> input) {
//        ret Value(0.0);
//    }),
//    Function("and", { "a","b" }, TypeDomain(nmr, nmr), [](vector<Value> input) {
//        ret Value(0.0);
//    }),
//    Function("xor", { "a","b" }, TypeDomain(nmr, nmr), [](vector<Value> input) {
//        ret Value(0.0);
//    }),
//    Function("ls", { "a","b" }, TypeDomain(dub | arb, dub | arb), [](vector<Value> input) {
//        return new Value(0.0);
//    }),
//    Function("rs", { "a","b" }, TypeDomain(dub | arb, dub | arb), [](vector<Value> input) {
//        ret Value(0.0);
//    }),
#pragma endregion
#pragma region Constants
    Constant("i",0,1.0),
    Constant("pi",3.14159265358979323),
    #define downscale {D(arb),D(dub)}
    #ifdef USE_ARB
    Function("arb_pi",{"prec"},{},{{D(dub),[](inp) {ret(Arb)(mppp::real_pi(Arb::digitsToPrecision(input[0]->getR())));}}}),
    Function("arb_e",{"prec"},{},{{D(dub),[](inp) {ret(Arb)(mppp::real_euler(Arb::digitsToPrecision(input[0]->getR())));}}}),
    //Function("arb_rand", [](vector<Value> input) {
    //    ret Value(0.0);
    //}),
    #endif
    Function("rand",{},{},{{D(),[](inp) {double upLim = RAND_MAX + 1.;ret(Number)(rand() / upLim + rand() / upLim / upLim);}}}),
    Function("srand",{"seed"},{downscale},{{D(dub),[](inp) { srand(getN(0).real() * 100); ret(Number)(0); }}}),
    Constant("e",2.71828182845904523),
    Constant("nan",NAN),
    Constant("inf",INFINITY),
    Constant("histlen",Program::history.size()),
    Function("ans",{"index"},{downscale},{{D(dub | opt),[](inp) {
        if(input.size() == 0) return Program::history.back();
        int index = getN(0).real();if(index < 0) index += Program::history.size();
        return Program::history[index];}}}),
#pragma endregion
#pragma region Functional
    Function("run", { "func","..." }, {}, {{D(lmb,all | opt,all | opt,all | opt),[](inp) {
        def(Lambda,func,0);
        ValList args(func->inputNames.size(),Value::zero);
        for(int i = 0;i < input.size() - 1;i++) args[i] = input[i + 1]->compute(ctx);
        ctx.pushArgs(args);
        ValPtr out = func->func->compute(ctx);
        ctx.popArgs(args);
        return out;
    }}}),
    Function("apply",{"func","args"},{},{{D(lmb,vec_t),[](inp) {
        def(Vector,v,1);
        ValList toRun = ValList{input[0]};
        for(int i = 0;i < v->size();i++) toRun.push_back(v->vec[i]);
        ValPtr out = Program::computeGlobal("run",toRun,ctx);
        return out;
    }},{D(str_t,vec_t),[](inp) {
        return Program::computeGlobal(getV(String,0)->str,getV(Vector,1)->vec,ctx);
    }}}),
//    Function("sum", { "func","begin","end","step" }, TypeDomain({ lbm,com,com,com }, 4, 3), [](vector<Value> input) {
//        ret Value(0.0);
//    }),
//    Function("product", { "func","begin","end","step" }, TypeDomain({ lbm,com,com,com }, 4, 3), [](vector<Value> input) {
//        ret Value(0.0);
//        }),
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
//#pragma endregion
//#pragma region Vector
//    Function("length", { "v" }, TypeDomain(vec | str | map), [](vector<Value> input) {
//        ret Value(0.0);
//    }),
//    Function("normalize", { "v" }, TypeDomain(vec), [](vector<Value> input) {
//        ret Value(0.0);
//    }),
//    Function("get", { "map","key" }, TypeDomain(vec | map, all), [](vector<Value> input) {
//        ret Value(0.0);
//    }),
//    Function("fill", { "func","count" }, TypeDomain(lbm, com), [](vector<Value> input) {
//        ret Value(0.0);
//    }),
//    Function("map_vec", { "map","func" }, TypeDomain(vec | map, lbm), [](vector<Value> input) {
//        ret Value(0.0);
//    }),
//    Function("concat", { "a","b" }, TypeDomain(all, all), [](vector<Value> input) {
//        ret Value(0.0);
//    }),
//    Function("sort", { "vec" }, TypeDomain({ vec,lbm }, 2, 1), [](vector<Value> input) {
//        ret Value(0.0);
//    }),
//#pragma endregion
//#pragma region String
//    Function("eval", { "str" }, TypeDomain(str), [](vector<Value> input) {
//        ret Value(0.0);
//    }),
//    Function("print", { "str" }, TypeDomain(str), [](vector<Value> input) {
//        ret Value(0.0);
//    }),
//    Function("error", { "str" }, TypeDomain(str), [](vector<Value> input) {
//        ret Value(0.0);
//    }),
//    Function("replace", { "str","find","rep" }, TypeDomain({ str | vec,all,all }), [](vector<Value> input) {
//        ret Value(0.0);
//    }),
//    Function("indexof", { "str","find" }, TypeDomain(str | vec, all), [](vector<Value> input) {
//        ret Value(0.0);
//    }),
//    Function("substr", { "str","begin","end" }, TypeDomain({ str | vec,com,com }, 3, 2), [](vector<Value> input) {
//        ret Value(0.0);
//    }),
//    Function("lowercase", { "str" }, TypeDomain(str), [](vector<Value> input) {
//        ret Value(0.0);
//    }),
//    Function("uppercase", { "str" }, TypeDomain(str), [](vector<Value> input) {
//        ret Value(0.0);
//    }),
//#pragma endregion
//#pragma region Conversion and variables
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
//    Function("double", { "val" }, TypeDomain(all), [](vector<Value> input) {
//        ret Value(0.0);
//    }),
//    Function("arb", { "val" }, TypeDomain(all), [](vector<Value> input) {
//        ret Value(0.0);
//    }),
//    Function("vec", { "val" }, TypeDomain(all), [](vector<Value> input) {
//        ret Value(0.0);
//    }),
//    Function("map", { "val" }, TypeDomain(all), [](vector<Value> input) {
//        ret Value(0.0);
//    }),
//    Function("string", { "val" }, TypeDomain(all), [](vector<Value> input) {
//        ret Value(0.0);
//    }),
//    Function("set", { "val" }, TypeDomain(all), [](vector<Value> input) {
//        ret Value(0.0);
//    }),
//    Function("typeof", { "val" }, TypeDomain(all), [](vector<Value> input) {
//        ret Value(0.0);
//    }),
#pragma endregion

};