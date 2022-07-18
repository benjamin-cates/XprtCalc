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
double Value::evaluateToReal() {
    try {
        Value out = ptr->compute(Program::computeCtx);
        if(out == nullptr) return NAN;
        if(out->typeID() == Value::num_t) {
            Number* n = out.cast<Number>().get();
            if(n->num.imag() != 0) return NAN;
            if(!n->unit.isUnitless()) return NAN;
            return n->num.real();
        }
        return NAN;
    }
    catch(...) {
        return NAN;
    }
}

#define construct(name,...) std::make_shared<Tree>(name,ValList{__VA_ARGS__})
#define TWO std::make_shared<Number>(2)
#define NEG(oneInp) construct("neg",oneInp)
#define ADD(oneInp,twoInp) construct("add",oneInp,twoInp)
#define SUB(oneInp,twoInp) construct("sub",oneInp,twoInp)
#define MUL(oneInp,twoInp) construct("mul",oneInp,twoInp)
#define DIV(oneInp,twoInp) construct("div",oneInp,twoInp)
bool decimalPollution(double source, double next) {
    if(source != 1 && source != -1) if(source == std::floor(source) && next != std::floor(next)) return true;
    return std::isnan(next);
}
void combineValueList(std::list<Value>& ls, string op) {
    while(ls.size() > 1) {
        ls.front() = std::make_shared<Tree>(op, ValList{ ls.front(),*(std::next(ls.begin())) });
        ls.erase(std::next(ls.begin()));
    }
}
class OperatorGroup {
    //Maps flatten() id to a pair of the value itself and it's magnitude
    std::multimap<double, std::pair<Value, double>> members;
public:
    std::multimap<double, std::pair<Value, double>>::iterator find(const Value& val, double flat = 0) {
        if(flat == 0) flat = val->flatten();
        auto x = members.lower_bound(flat);
        if(flat == x->first) {
            if(x == members.end()) return x;
            while(val != x->second.first) {
                x++;
                if(x->first != flat) return members.end();
            }
            return x;
        }
        return members.end();
    }
    double getMagnitude(const Value& val, double flat = 0) {
        if(flat == 0) flat = val->flatten();
        auto x = find(val, flat);
        if(x == members.end()) return 0;
        else return x->second.second;
    }
    void addElement(const Value& val, double magnitude = 1) {
        double flat = val->flatten();
        auto x = find(val, flat);
        if(x == members.end()) members.insert(std::make_pair(flat, std::make_pair(val, magnitude)));
        else x->second.second += magnitude;
    }
    std::multimap<double, std::pair<Value, double>>& getMap() { return members; }
    string printGroup();
};
string OperatorGroup::printGroup() {
    string out;
    for(auto element : members)
        out += element.second.first->toString() + " -> " + std::to_string(element.second.second) + "\n";
    return out;
}
class ProductGroup : public OperatorGroup {
public:
    Value toTree() {
        std::list<Value> positives;
        std::list<Value> negatives;
        double negativeCoef = 1;
        double positiveCoef = 1;
        auto& map = getMap();
        for(auto element : map) {
            double& pow = element.second.second;
            Value& val = element.second.first;
            if(val.isInteger()) {
                double intgr = val->getR();
                if(pow != 1) intgr = std::pow(intgr, pow);
                if(pow > 0) positiveCoef *= intgr;
                else negativeCoef *= intgr;
                continue;
            }
            Value toAdd;
            if(std::abs(pow) == 0.5) toAdd = std::make_shared<Tree>("sqrt", ValList{ val });
            else if(std::abs(pow) == 1) toAdd = val;
            else if(std::abs(pow) == 0) continue;
            else toAdd = std::make_shared<Tree>("pow", ValList{ val,std::make_shared<Number>(std::abs(pow)) });
            if(pow > 0) positives.push_back(toAdd);
            else negatives.push_back(toAdd);
        }
        if(positiveCoef != 1 && positiveCoef / negativeCoef == std::floor(positiveCoef / negativeCoef))
            positives.push_front(std::make_shared<Number>(positiveCoef / negativeCoef));
        else {
            if(positiveCoef != 1) positives.push_front(std::make_shared<Number>(positiveCoef));
            if(negativeCoef != 1) negatives.push_front(std::make_shared<Number>(negativeCoef));
        }
        combineValueList(positives, "mul");
        combineValueList(negatives, "mul");
        if(negatives.size() != 0) {
            if(positives.size() != 0) return std::make_shared<Tree>("div", ValList{ positives.front(),negatives.front() });
            else return std::make_shared<Tree>("div", ValList{ Value::one,negatives.front() });
        }
        else {
            if(positives.size() != 0) return positives.front();
            return Value::one;
        }
    }
    ProductGroup(Value tree) { ProductGroup::Generate(tree, *this, 1); }
    static void Generate(const Value& val, ProductGroup& output, double coef = 1) {
        if(val->typeID() == Value::tre_t) {
            std::shared_ptr<Tree> tr = val.cast<Tree>();
            string opName = Program::globalFunctions[tr->op].getName();
            if(opName == "sqrt")
                return Generate(tr->branches[0], output, coef * 0.5);
            if(opName == "exp" || opName == "pow") {
                double power = tr->branches[opName == "pow" ? 1 : 0].evaluateToReal();
                if(!decimalPollution(coef, power)) {
                    if(opName == "pow") Generate(tr->branches[0], output, coef * power);
                    if(opName == "exp") output.addElement(Value(std::make_shared<Tree>("e", ValList{})), coef * power);
                    return;
                }
            }
            if(opName == "mul" || opName == "div") {
                Generate(tr->branches[0], output, coef);
                Generate(tr->branches[1], output, (opName == "mul" ? 1 : -1) * coef);
                return;
            }
            if(opName == "neg") {
                output.addElement(std::make_shared<Number>(-1), coef);
                return Generate(tr->branches[0], output, coef);
            }
        }
        if(val != Value::one) output.addElement(val, coef);
    }
};
class SumGroup : public OperatorGroup {
public:
    Value toTree() {
        auto& map = getMap();
        std::list<Value> positives;
        std::list<Value> negatives;
        double extraIntegers = 0;
        for(auto element : map) {
            double& coef = element.second.second;
            Value& val = element.second.first;
            Value toAdd;
            if(val.isInteger()) {
                double intgr = val->getR();
                if(coef != 1) intgr = std::pow(intgr, coef);
                extraIntegers += intgr;
                continue;
            }
            if(std::abs(coef) == 1) toAdd = val;
            else if(std::abs(coef) == 0) continue;
            else toAdd = std::make_shared<Tree>("mul", ValList{ std::make_shared<Number>(std::abs(coef)),val });
            if(coef > 0) positives.push_back(toAdd);
            else negatives.push_back(toAdd);
        }
        if(extraIntegers != 0) positives.push_back(std::make_shared<Number>(extraIntegers));
        combineValueList(positives, "add");
        combineValueList(negatives, "add");
        if(negatives.size() != 0) {
            if(positives.size() != 0) return std::make_shared<Tree>("sub", ValList{ positives.front(),negatives.front() });
            else return std::make_shared<Tree>("neg", ValList{ negatives.front() });
        }
        else {
            if(positives.size() != 0) return positives.front();
            return Value::zero;
        }
    }
    SumGroup(Value tree) { SumGroup::Generate(tree, *this, 1); }
    static void Generate(const Value& val, SumGroup& output, double coef = 1) {
        if(val->typeID() == Value::tre_t) {
            std::shared_ptr<Tree> tr = val.cast<Tree>();
            string opName = Program::globalFunctions[tr->op].getName();
            if(opName == "mul") {
                double a = tr->branches[0].evaluateToReal();
                if(!decimalPollution(coef, a))
                    return Generate(tr->branches[1], output, coef * a);
                double b = tr->branches[1].evaluateToReal();
                if(!decimalPollution(coef, b))
                    return Generate(tr->branches[0], output, coef * b);
            }
            if(opName == "div" && coef != std::floor(coef)) {
                double denom = tr->branches[1].evaluateToReal();
                if(!std::isnan(denom))
                    return Generate(tr->branches[0], output, coef / denom);
            }
            if(opName == "add" || opName == "sub") {
                Generate(tr->branches[0], output, coef);
                Generate(tr->branches[1], output, (opName == "add" ? 1 : -1) * coef);
                return;
            }
            if(opName == "neg")
                return Generate(tr->branches[0], output, -coef);
        }
        if(val != Value::zero) output.addElement(val, coef);
    }
};
Value Value::simplify(bool useAddGroup, bool useMultGroup) {
    int type = ptr->typeID();
    if(type == vec_t) {
        ValList& vec = cast<Vector>()->vec;
        std::shared_ptr<Vector> out = std::make_shared<Vector>();
        for(int i = 0;i < vec.size();i++) out->vec.push_back(vec[i].simplify());
        return out;
    }
    else if(type == map_t) {
        std::map<Value, Value>& map = cast<Map>()->getMapObj();
        std::shared_ptr<Map> out = std::make_shared<Map>();
        for(auto p : map) out->append(p.first.deepCopy().simplify(), p.second.simplify());
        return out;
    }
    else if(type == lmb_t) {
        return std::make_shared<Lambda>(cast<Lambda>()->inputNames, cast<Lambda>()->func.simplify());
    }
    else if(type == tre_t) {
        string name = Program::globalFunctions[cast<Tree>()->op].getName();
        ValList& branch = cast<Tree>()->branches;
        if(name == "add" || name == "sub") if(branch[1] == Value::zero) return branch[0].simplify(true, useMultGroup);
        if(name == "add") if(branch[0] == Value::zero) return branch[1].simplify(true, useMultGroup);
        if(name == "mul" || name == "div") {
            if(branch[0] == Value::zero) return Value::zero;
        }
        if(name == "mul") {
            if(branch[1] == Value::zero) return Value::zero;
            if(branch[0] == Value::one) return branch[1].simplify(useAddGroup, true);
            if(branch[1] == Value::one) return branch[0].simplify(useAddGroup, true);
        }
        if(useAddGroup) if(name == "add" || name == "sub") {
            ValList newArgs{ branch[0].simplify(false,true),branch[1].simplify(false,true) };
            SumGroup g(std::make_shared<Tree>(cast<Tree>()->op, std::move(newArgs)));
            Value out = g.toTree();
            return out;
        }
        if(useMultGroup) if(name == "mul" || name == "div" || name == "pow") {
            ValList newArgs{ branch[0].simplify(true,false),branch[1].simplify(true,false) };
            ProductGroup g(std::make_shared<Tree>(cast<Tree>()->op, std::move(newArgs)));
            Value out = g.toTree();
            return out;
        }
        else {
            bool areIntegers = true;
            ValList newBranch;
            for(int i = 0;i < branch.size();i++) {
                newBranch.push_back(branch[i].simplify());
                if(!newBranch.back().isInteger()) areIntegers = false;
            }
            if(areIntegers) {
                Value out = ptr->compute(Program::computeCtx);
                if(out.isInteger()) return out;
            }
            return std::make_shared<Tree>(cast<Tree>()->op, std::move(newBranch));
        }
    }
    return *this;
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
        for(auto p : map) out->append(p.first.deepCopy().derivative(argDerivatives), p.second.derivative(argDerivatives));
        return out;
    }
    else if(type == arg_t)
    //Other's are treated as constants (preliminary, hopefully they can return dy/dx or something later)
        return argDerivatives[cast<Argument>()->id];
    else if(type == lmb_t) {

    }
    else if(type == tre_t) {
        #define TimesDU(statement) return construct("mul",(statement),dx[0])
        ValList& branch = cast<Tree>()->branches;
        int op = cast<Tree>()->op;
        string name = Program::globalFunctions[op].getName();
        ValList dx;
        for(int i = 0;i < branch.size();i++) dx.push_back(branch[i].derivative(argDerivatives));
        //Add,  subtract, neg
        if(name == "add" || name == "sub") return construct(op, dx[0], dx[1]);
        if(name == "neg") return construct(op, dx[0]);
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