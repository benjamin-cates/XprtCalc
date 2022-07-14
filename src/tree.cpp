#include "_header.hpp"
Tree::Tree(int opId, ValList&& branchList) {
    op = opId;
    branches = std::forward<ValList>(branchList);
}
Tree::Tree(string opStr, ValList&& branchList) {
    auto it = Program::globalFunctionMap.find(opStr);
    if(it == Program::globalFunctionMap.end())
        throw "Function " + opStr + " not found";
    op = it->second;
    branches = std::forward<ValList>(branchList);
}
Tree::Tree(string opStr, Value one, Value two) {
    *this = Tree(opStr, ValList{ one,two });
}
Tree::Tree(int opId) {
    op = opId;
}
bool Tree::operator==(const Tree& a)const {
    if(op != a.op) return false;
    if(branches.size() != a.branches.size()) return false;
    for(int i = 0;i < branches.size();i++) {
        if(branches[i] != a.branches[i]) return false;
    }
    return true;
}
Value Tree::operator[](int index) {
    return branches[index];
}
Value Tree::derivative() {
    return std::make_shared<Number>(0);
}
string Tree::toWebGL() {
    return "Execute";
}
void Tree::simplify() {

}
void Tree::compSimplify() {

}
Value Value::derivative(ValList argDerivatives) {
    int type = ptr->typeID();
    if(type == vec_t) {
        ValList& vec = cast<Vector>()->vec;
        std::shared_ptr<Vector> out = std::make_shared<Vector>();
        for(int i = 0;i < vec.size();i++) out->vec.push_back(vec[i].derivative(argDerivatives));
        return out;
    }
    else if(type == map_t) {
        std::map<Value, Value>& map = cast<Map>()->getMapObj();
        std::shared_ptr<Map> out = std::make_shared<Map>();
        for(auto p : map) out->append(p.first, p.second);
        return out;
    }
    else if(type == arg_t)
    //Other's are treated as constants (preliminary, hopefully they can return dy/dx or something later)
        return argDerivatives[cast<Argument>()->id];
    else if(type == lmb_t) {

    }
    else if(type == tre_t) {
        #define construct(name,...) std::make_shared<Tree>(name,ValList{__VA_ARGS__})
        #define TimesDU(statement) return construct("mul",(statement),dx[0])
        #define TWO std::make_shared<Number>(2)
        #define NEG(oneInp) construct("neg",one)
        #define ADD(oneInp,twoInp) construct("add",oneInp,twoInp)
        #define SUB(oneInp,twoInp) construct("sub",oneInp,twoInp)
        #define MUL(oneInp,twoInp) construct("mul",oneInp,twoInp)
        #define DIV(oneInp,twoInp) construct("div",oneInp,twoInp)
        ValList& branch = cast<Tree>()->branches;
        int op = cast<Tree>()->op;
        string name = Program::globalFunctions[op].getName();
        ValList dx;
        for(int i = 0;i < branch.size();i++) dx.push_back(branch[i].derivative(argDerivatives));
        //Add,  subtract, neg
        if(name == "add" || name == "sub" || name == "neg") return construct(op, dx[0], dx[1]);
        //Multiply d/dx(a*b) = a*db+b*da
        if(name == "mul") return ADD(MUL(branch[0], dx[1]), MUL(branch[1], dx[0]));
        //Divide d/dx (a/b) = (a*db-b*da)/(a^2)
        if(name == "div") return DIV(SUB(MUL(branch[0], dx[1]), MUL(dx[0], branch[1])), construct("pow", branch[0], TWO));
        //Power d/dx (a^b) = a^b * (b/a*da + ln(a)*db)
        if(name == "pow") return MUL(construct("pow", branch[0], branch[1]), ADD(
            MUL(DIV(branch[1], branch[0]), dx[0]),
            MUL(construct("ln", branch[0]), dx[1])));
    //Logarithmic/exponential
        if(name == "ln") return DIV(dx[0], branch[0]);
        if(name == "logb") return Value(DIV(construct("ln", branch[0]), construct("ln", branch[1]))).derivative(argDerivatives);
        if(name == "log") return DIV(dx[0], construct("mul", branch[0], construct("ln", std::make_shared<Number>(10))));
        if(name == "sqrt") return construct("div", dx[0], construct("mul", TWO, construct("sqrt", branch[0])));
        if(name == "abs") TimesDU(DIV(dx[0], construct("abs", branch[0])));
        if(name == "exp") TimesDU(construct("exp", branch[0]));
        //Trig functions
        if(name == "sin") TimesDU(construct("cos", branch[0]));
        if(name == "cos") TimesDU(NEG(construct("sin", branch[0])));
        if(name == "tan") TimesDU(construct("pow", construct("sec", branch[0]), TWO));
        if(name == "cot") TimesDU(NEG(construct("pow", construct("csc", branch[0]), TWO)));
        if(name == "sec") TimesDU(construct("mul", construct("sec", branch[0]), construct("tan", branch[0])));
        if(name == "csc") TimesDU(NEG(MUL(construct("csc", branch[0]), construct("cot", branch[0]))));
        if(name == "asin") return DIV(dx[0], construct("sqrt", SUB(Value::one, construct("pow", branch[0], TWO))));
        if(name == "acos") return DIV(dx[0], NEG(construct("sqrt", SUB(Value::one, construct("pow", branch[0], TWO)))));
        if(name == "atan") return DIV(dx[0], SUB(Value::one, construct("pow", branch[0], TWO)));
        if(name == "sinh") TimesDU(construct("cosh", branch[0]));
        if(name == "cosh") TimesDU(construct("sinh", branch[0]));
        if(name == "tanh") TimesDU(SUB(Value::one, construct("pow", construct("tanh", branch[0]), TWO)));
        if(name == "asinh") return DIV(dx[0], construct("sqrt", ADD(construct("pow", branch[0], TWO), Value::one)));
        if(name == "acosh") return DIV(dx[0], construct("sqrt", SUB(construct("pow", branch[0], TWO), Value::one)));
        if(name == "atanh") return DIV(dx[0], SUB(Value::one, construct("pow", branch[0], TWO)));

        //Lambda stuff
        if(name == "sum") {
            if(branch.size() == 1) return construct("sum", dx[0]);
            else if(branch.size() == 3) return construct("sum", dx[0], branch[1], branch[2]);
            else return construct("sum", dx[0], branch[1], branch[2], branch[3]);
        }

        //Componentual
        if(name == "getr" || name == "geti" || name == "getu")
            return construct(op, dx[0]);
        //Min and max
        if(name == "max" || name == "min" || name == "concat")
            return construct(op, dx[0], dx[1]);
        //Constants
        if(name == "ans" || name == "pi" || name == "round" || name == "floor" || name == "ceil" || name == "i" || name == "true" || name == "false" || name == "arb_pi" || name == "arb_e" || name == "rand" || name == "e" || name == "nan" || name == "inf" || name == "histlen") return Value::zero;
        //Comparison
        if(name == "equal" || name == "not_equal" || name == "gt" || name == "gt_equal" || name == "lt" || name == "lt_equal") return Value::zero;
        //Bitwise operators
        if(name == "or" || name == "xor" || name == "and" || name == "ls" || name == "rs") return Value::zero;
        //Conversion
        if(name == "tonumber" || name == "toarb" || name == "tovector" || name == "tomap" || name == "tostring" || name == "tolambda")
            return construct(op, dx[0]);
        throw "Function " + name + " does not support derivative";

    }
    return Value::zero;
}