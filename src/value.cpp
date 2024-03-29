#include "_header.hpp"


#pragma region Number
Number::Number(double r, double i, Unit u) {
    num = std::complex<double>(r, i);
    unit = u;
}
Number::Number(std::complex<double> n, Unit u) {
    num = n;
    unit = u;
}
#pragma endregion
#pragma region Arb
#ifdef USE_ARB
Arb::Arb(mppp::real r, double i, Unit u) {
    num = r;
    unit = u;
}
Arb::Arb(mppp::real n, Unit u) {
    num = n;
    unit = u;
}
mpfr_prec_t Arb::digitsToPrecision(int digits) {
    if(digits > 10000) digits = 10000;
    if(digits < 1) throw "Cannot have " + std::to_string(digits) + " digits of precision";
    constexpr double conv = std::log(10) / std::log(2);
    return mpfr_prec_t(conv * digits + 2);
}
int Arb::precisionToDigits(mpfr_prec_t prec) {
    constexpr double conv = std::log(10) / std::log(2);
    return int(1.0 * (prec - 2) / conv) + 1;
}
#endif
#pragma endregion
#pragma region Vector
Vector::Vector(ValList&& v) {
    vec = std::forward<ValList>(v);
}
Vector::Vector(Value topLeft) {
    vec.resize(1);
    vec[0] = topLeft;
}
Vector::Vector(int x) {
    vec.resize(x, Value::zero);
}
int Vector::size()const {
    return vec.size();
}
Value Vector::get(unsigned int x) {
    if(x >= vec.size()) {
        return std::make_shared<Number>(0);
    }
    return vec[x];
}
Value Vector::operator[](unsigned int x) {
    return vec[x];
}
void Vector::set(int x, Value val) {
    if(x >= vec.size()) {
        vec.resize(x + 1, Value::zero);
    }
    vec[x] = val;
}
#pragma endregion
#pragma region Lambda
Lambda::Lambda(std::vector<string> inputs, Value funcTree) {
    inputNames = inputs;
    func = funcTree;
}
Value Lambda::operator()(ValList inputs, ComputeCtx& ctx) {
    for(int i = 0;i < inputs.size();i++) if(inputs[i] == nullptr) throw "err";
    if(inputs.size() < inputNames.size()) throw "not enough arguments for lambda";
    if(inputs.size() != inputNames.size()) inputs.resize(inputNames.size());
    ctx.pushArgs(inputs);
    Value out;
    try {
        out = func->compute(ctx);
    }
    catch(...) { ctx.popArgs(inputs);throw; }
    ctx.popArgs(inputs);
    return out;
}
#pragma endregion
#pragma region String
string String::safeBackspaces(const string& str) {
    string out = "";
    for(int i = 0;i < str.length();i++) {
        if(str[i] == '\\') out += "\\\\";
        else if(str[i] == '"') out += "\\\"";
        else if(str[i] == '\n') out += "\\n";
        else if(str[i] == '\t') out += "\\t";
        else out += str[i];
    }
    return out;
}
#pragma endregion
#pragma region Map
Value& Map::operator[](const Value& idx) {
    if(has(idx)) return mapObj[idx];
    else return Value::zero;
}
bool Map::has(const Value& idx) {
    return mapObj.find(idx) != mapObj.end();
}
void Map::append(const Value& key, const Value& val) {
    mapObj[key] = val;
}
int Map::size()const { return mapObj.size(); }
#pragma endregion
#pragma region Value comparison
bool operator==(const Value& lhs, const Value& rhs) {
    if(lhs.get() == rhs.get()) return true;
    if(lhs.get() == nullptr || rhs.get() == nullptr) return false;
    if(lhs->typeID() != rhs->typeID()) return false;
    #define cast(type,name1,name2) name1=lhs.cast<type>();name2=rhs.cast<type>();
    std::shared_ptr<Number> n1, n2;
    #ifdef USE_ARB
    std::shared_ptr<Arb> a1, a2;
    #endif
    #ifdef GMP_WASM
    std::shared_ptr<Arb> a1, a2;
    #endif
    std::shared_ptr<Vector> v1, v2;
    std::shared_ptr<Lambda> l1, l2;
    std::shared_ptr<String> s1, s2;
    std::shared_ptr<Map> m1, m2;
    std::shared_ptr<Tree> t1, t2;
    switch(lhs->typeID()) {
    case Value::num_t:
        cast(Number, n1, n2);
        if(!(n1->unit == n2->unit)) return false;
        if(n1->num == n2->num) return true;
        return false;
    case Value::arb_t:
        #ifdef USE_ARB
        cast(Arb, a1, a2)
            if(!(a1->unit == a2->unit)) return false;
        if(a1->num == a2->num) return true;
        #endif
        #ifdef GMP_WASM
        cast(Arb, a1, a2)
            if(!(a1->unit == a2->unit)) return false;
        if(a1->num == a2->num) return true;
        #endif
        return false;
    case Value::vec_t:
        cast(Vector, v1, v2);
        if(v1->size() != v2->size()) return false;
        for(int i = 0;i < v1->size();i++) if(!(v1->vec[i] == v2->vec[i])) return false;
        return true;
    case Value::lmb_t:
        cast(Lambda, l1, l2);
        if(l1->inputNames.size() != l2->inputNames.size()) return false;
        if(l1->func == l2->func) return true;
        return false;
    case Value::str_t:
        cast(String, s1, s2);
        if(s1->str == s2->str) return true;
        return false;
    case Value::map_t:
        cast(Map, m1, m2);
        if(true) {
            auto& map1 = m1->getMapObj();
            auto& map2 = m2->getMapObj();
            if(map1.size() != map2.size()) return false;
            for(auto member : map1)
                if((*m2)[member.first] != member.second) return false;
            return true;
        }
    case Value::arg_t:
        if(lhs.cast<Argument>()->id != rhs.cast<Argument>()->id) return false;
        return true;
    case Value::tre_t:
        cast(Tree, t1, t2);
        if(t1->op != t2->op) return false;
        if(t1->branches.size() != t2->branches.size()) return false;
        for(int i = 0;i < t1->branches.size();i++)
            if(!(t1->branches[i] == t2->branches[i])) return false;
        return true;
    case Value::var_t:
        if(lhs.cast<Variable>()->name != rhs.cast<Variable>()->name) return false;
        return true;
    default:
        return false;
    }
    return false;
}
bool Value::isInteger() {
    if(ptr->typeID() == Value::num_t) {
        Number* n = cast<Number>().get();
        if(n->num.real() != std::floor(n->num.real())) return false;
        if(n->num.imag() != 0) return false;
        if(!n->unit.isUnitless()) return false;
        return true;
    }
    return false;
}

#pragma endregion
#pragma region Value conversion
Value Value::convertTo(int type, int precision) {
    int curType = ptr->typeID();
    if(type == curType) return *this;
    if(type == num_t) {
        #ifdef USE_ARB
        if(curType == arb_t) { std::shared_ptr<Arb> a = cast<Arb>(); return std::make_shared<Number>(double(a->num), 0.0, a->unit); }
        #endif
        #ifdef GMP_WASM
        if(curType == arb_t) { std::shared_ptr<Arb> a = cast<Arb>(); return std::make_shared<Number>(double(a->num), 0.0, a->unit); }
        #endif
        if(curType == vec_t) { return cast<Vector>()->get(0).convertTo(num_t); }
        if(curType == lmb_t) throw "Cannot convert lambda to number";
        else if(curType == str_t) return Expression::parseNumeral(cast<String>()->str, 10);
        else if(curType == map_t) throw "Cannot convert map to number";
    }
    #ifdef USE_ARB
    else if(type == arb_t) {
        if(curType == num_t) { std::shared_ptr<Number> n = cast<Number>(); return std::make_shared<Arb>(mppp::real(n->num.real(), Arb::digitsToPrecision(precision)), n->unit); }
        else if(curType == vec_t) { return cast<Vector>()->get(0).convertTo(arb_t, precision); }
        else if(curType == lmb_t) throw "Cannot convert lambda to arb";
        else if(curType == str_t) return Expression::parseNumeral(cast<String>()->str + "p" + std::to_string(cast<String>()->str.length()), 10);
        else if(curType == map_t) throw "Cannot convert map to arb";
    }
    #endif
    #ifdef GMP_WASM
    else if(type == arb_t) {
        if(curType == num_t) { std::shared_ptr<Number> n = cast<Number>(); return std::make_shared<Arb>(mpfr_t(n->num.real(), precision), n->unit); }
        else if(curType == vec_t) { return cast<Vector>()->get(0).convertTo(arb_t, precision); }
        else if(curType == lmb_t) throw "Cannot convert lambda to arb";
        else if(curType == str_t) return Expression::parseNumeral(cast<String>()->str + "p" + std::to_string(cast<String>()->str.length()), 10);
        else if(curType == map_t) throw "Cannot convert map to arb";
    }
    #endif
    else if(type == vec_t) {
        if(curType == map_t) {
            std::map<Value, Value>& m = cast<Map>()->getMapObj();
            std::shared_ptr<Vector> out = std::make_shared<Vector>();
            for(auto p : m) {
                if(p.first->typeID() == num_t) {
                    double idx = p.first->getR();
                    if(idx == floor(idx) && idx <= 1000 && idx >= 0)
                        out->set(int(idx), p.second);
                }
            }
            return out;
        }
        return std::make_shared<Vector>(*this);
    }
    //To lambda
    else if(type == lmb_t) {
        if(curType == str_t) {
            string str = cast<String>()->str;
            if(Program::globalFunctionMap.find(str) == Program::globalFunctionMap.end()) throw "function " + str + " not found";
            int id = Program::globalFunctionMap.at(str);
            std::vector<string> inputs = Program::globalFunctions[id].getInputNames();
            ValList treeArgs{ inputs.size() };
            for(int i = 0;i < treeArgs.size();i++) treeArgs[i] = std::make_shared<Argument>(i);
            Value tree = std::make_shared<Tree>(id, std::move(treeArgs));
            return std::make_shared<Lambda>(inputs, tree);
        }
        else return std::make_shared<Lambda>(std::vector<string>(), *this);
    }
    //To string
    else if(type == str_t) {
        return std::make_shared<String>(ptr->toString());
    }
    //To map
    else if(type == map_t) {
        std::shared_ptr<Map> out = std::make_shared<Map>();
        if(curType == vec_t) {
            std::shared_ptr<Vector> v = cast<Vector>();
            for(unsigned int i = 0;i < v->size();i++)
                out->append(std::make_shared<Number>(i), v->get(i));
        }
        else out->append(Value::zero, *this);
        return out;
    }
    if(curType == vec_t) return cast<Vector>()->get(0).convertTo(type);
    return std::make_shared<Number>(0);
}
Value Value::deepCopy()const {
    int type = ptr->typeID();
    if(type == num_t)
        return std::make_shared<Number>(cast<Number>()->num, cast<Number>()->unit);
    #if defined(USE_ARB) || defined(GMP_WASM)
    else if(type == arb_t)
        return std::make_shared<Arb>(cast<Arb>()->num, cast<Arb>()->unit);
    #endif
    else if(type == vec_t) {
        int size = cast<Vector>()->vec.size();
        std::shared_ptr<Vector> out = std::make_shared<Vector>(size);
        for(int i = 0;i < size;i++) {
            out->vec[i] = cast<Vector>()->vec[i].deepCopy();
        }
        return out;
    }
    else if(type == map_t) {
        std::shared_ptr<Map> out = std::make_shared<Map>();
        std::map<Value, Value>& cur = cast<Map>()->getMapObj();
        for(auto p : cur)
            out->append(p.first.deepCopy(), p.second.deepCopy());
        return out;
    }
    else if(type == lmb_t)
        return std::make_shared<Lambda>(cast<Lambda>()->inputNames, cast<Lambda>()->func.deepCopy());
    else if(type == tre_t) {
        std::shared_ptr<Tree> out = std::make_shared<Tree>(cast<Tree>()->op);
        int count = cast<Tree>()->branches.size();
        out->branches.resize(count);
        for(int i = 0;i < count;i++)
            out->branches[i] = cast<Tree>()->branches[i].deepCopy();
        return out;
    }
    else if(type == str_t)
        return std::make_shared<String>(cast<String>()->str);
    else if(type == var_t)
        return std::make_shared<Variable>(cast<Variable>()->name);
    else if(type == arg_t)
        return std::make_shared<Argument>(cast<Argument>()->id);
    return std::make_shared<Number>(0);
}
void Value::set(Value& val, ValList indicies, Value setTo) {
    Value* head = &val;
    for(int i = 0;i < indicies.size();i++) {
        Value& idx = indicies[i];
        int type = (*head)->typeID();
        bool isInteger = idx->typeID() == num_t && idx->getR() == std::floor(idx->getR()) && idx->getR() >= 0 && idx->getR() < 100000;
        //Assign to string
        if(type == str_t && isInteger) {
            if(i != indicies.size() - 1) throw "Character type cannot be indexed";
            int index = idx->getR();
            if(index > head->cast<String>()->str.length()) throw "Index out of bounds of string";
            if(setTo->typeID() != str_t) throw "Cannot assign non-string to string index";
            head->cast<String>()->str[index] = setTo.cast<String>()->str[0];
            return;
        }
        //Convert to map or vector if type is not right
        if(type != map_t && (type != vec_t || !isInteger)) {
            if(isInteger) *head = head->deepCopy().convertTo(vec_t);
            else *head = head->deepCopy().convertTo(map_t);
            type = (*head)->typeID();
        }
        //Assign to map
        if(type == map_t)
            head = &head->cast<Map>()->getMapObj()[idx];
        //Assign to vector
        else if(type == vec_t && isInteger) {
            int index = idx->getR();
            //Resize if too small
            if(head->cast<Vector>()->vec.size() <= index) head->cast<Vector>()->vec.resize(index + 1, Value::zero);
            head = &head->cast<Vector>()->vec[index];
        }
    }
    (*head) = setTo;
}
#pragma endregion
#pragma region flatten
double Number::flatten()const { return num.real() * 4.59141 + num.imag() * 2.12941 + double(unit.getBits()); }
#ifdef USE_ARB
double Arb::flatten()const { return double(num) * 5.132441 + double(unit.getBits()); }
#endif
#ifdef GMP_WASM
double Arb::flatten()const { return double(num) * 5.132441 + double(unit.getBits()); }
#endif
double Vector::flatten()const {
    double out = 0;
    for(int i = 0;i < vec.size();i++) out += vec[i]->flatten() * (i * 1.1508623 + 5.144124);
    return out;
}
double Lambda::flatten()const { return func->flatten(); }
double String::flatten()const {
    double hash = 0;
    for(int i = 0;i < str.size();i++) {
        hash += str[i] * 0.388 * i + 0.590809;
    }
    return hash;
}
double Map::flatten()const {
    double out = 0;
    int i = 0;
    for(auto p : mapObj) {
        out += p.first->flatten() * 5.6 + 4.2 * i + 3.99;
        out += p.second->flatten() * 84.6 + 8.2 * i + 9.99;
        i++;
    }
    return out;
}
double Tree::flatten()const {
    double out = 0.0;
    for(int i = 0;i < branches.size();i++) out += (i * 0.890532 + 1.98235) * branches[i]->flatten();
    return out + op * 1.15241312;
}
bool operator<(const Value& lhs, const Value& rhs) {
    return lhs->flatten() < rhs->flatten();
}
#pragma endregion
#pragma region compute
Value Vector::compute(ComputeCtx& ctx) {
    ValList a(vec.size());
    for(int i = 0;i < vec.size();i++) {
        a[i] = vec[i]->compute(ctx);
    }
    return std::make_shared<Vector>(std::move(a));
}
Value Lambda::compute(ComputeCtx& ctx) {
    if(ctx.argValue.size() == 0) return shared_from_this();
    ValList unreplacedArgs{ inputNames.size() };
    for(int i = 0;i < inputNames.size();i++) unreplacedArgs[i] = std::make_shared<Argument>(i);
    //Push arguments to stack
    ctx.pushArgs(unreplacedArgs);
    Value newLambda;
    try { newLambda = func->compute(ctx); }
    catch(...) { ctx.popArgs(unreplacedArgs);throw; }
    ctx.popArgs(unreplacedArgs);
    return std::make_shared<Lambda>(inputNames, newLambda);
}
Value Lambda::replaceArgs(ComputeCtx& ctx) {
    return std::make_shared<Lambda>(inputNames, func->compute(ctx));
}
Value Map::compute(ComputeCtx& ctx) {
    std::map<Value, Value> newMap;
    for(auto p : mapObj) {
        newMap.insert({ p.first->compute(ctx),p.second->compute(ctx) });
    }
    return std::make_shared<Map>(std::move(newMap));
}
Value Tree::compute(ComputeCtx& ctx) {
    ValList computed(branches.size());
    bool computable = true;
    for(int i = 0;i < branches.size();i++) {
        computed[i] = std::move(branches[i]->compute(ctx));
        if(computed[i]->typeID() >= Value::tre_t) computable = false;
    }
    if(computable) return Program::globalFunctions[op](computed, ctx);
    else return std::make_shared<Tree>(op, std::forward<ValList>(computed));
}
Value Argument::compute(ComputeCtx& ctx) {
    return ctx.getArgument(id);
}
#pragma endregion
#pragma region toString
string ValueBaseClass::toString(int base)const { return this->toStr(Program::parseCtx, base); }
string Number::componentToString(double x, int base) {
    if(x == 0) return "0";
    bool isNegative = x < 0;
    x = std::abs(x);
    double exponent = std::floor(std::log(x * 1.00000000000001) / std::log(double(base)));
    x /= std::pow(base, exponent);
    //Floating point
    int precision = 11;
    if(base != 10) precision = std::log(10) / std::log(base) * 11.0;
    if(exponent > precision || exponent < -7.0) {
        return (isNegative ? "-" : "") + componentToString(x, base) + "e" + componentToString(double(exponent), base);
    }
    string out;
    if(isNegative) out += "-";
    //Leading zeroes
    if(exponent < 0) {
        out += "0.";
        out += string((-exponent) - 1, '0');
    }
    //Create digit list
    std::vector<int> digitList;
    int i;
    for(i = 0;i < precision + 1;i++) {
        if(std::floor(x * 1000000000) == 0 && i > exponent) break;
        if(std::floor(x) >= base) {
            digitList.push_back(1);
            x -= base;
        }
        digitList.push_back(std::floor(x));
        x -= std::floor(x);
        x *= base;
    }
    //Rounding digit
    if(i == precision + 1 && digitList.back() >= base / 2) {
        int x = digitList.size() - 2;
        digitList.resize(digitList.size() - 1);
        digitList[x] += 1;
        while(digitList[x] >= base) {
            digitList[x] = 0;
            if(x == 0) { digitList.insert(digitList.begin(), 1);exponent += 1; }
            else { digitList[x - 1] += 1;x--; }
        }
    }
    //Remove trailing zeroes
    while(digitList.size() > std::max(exponent + 1, 1.0) && digitList.back() == 0) digitList.resize(digitList.size() - 1);
    //Add digits to string
    const static string baseChars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    for(int d = 0;d < digitList.size();d++) {
        out += baseChars[digitList[d]];
        if(d == exponent && d != digitList.size() - 1) out += '.';
    }
    if(digitList.size() < exponent) out += string(exponent - digitList.size(), '0');
    return out;
}
string Number::toStr(ParseCtx& ctx, int base)const {
    string out;
    string unitStr;
    double real = num.real();
    double imag = num.imag();
    if(!unit.isUnitless()) {
        double outputRatio = 1;
        unitStr = "[" + unit.toString(&outputRatio) + "]";
        real /= outputRatio;
        imag /= outputRatio;
        //wrap in brackets if both are not
        if(real != 0 && imag != 0) {
            out += "(";
            unitStr = ")*" + unitStr;
        }
    }
    if(real != 0) {
        if(Math::isInf(real)) return "inf";
        else if(Math::isNan(real)) return "undefined";
        else out += Number::componentToString(real, base);
    }
    if(imag != 0) {
        if(imag > 0 && real != 0) out += '+';
        if(Math::isInf(imag)) out += "+inf*";
        else if(Math::isNan(imag)) out += "+undefined*";
        else out += Number::componentToString(imag, base);
        out += 'i';
        //Prevent accessor notation problem
        if(unitStr.length() != 0 && real == 0) unitStr = "*" + unitStr;
    }
    if(out == "") out = "0";
    out += unitStr;
    return out;
}
#if defined( USE_ARB) || defined(GMP_WASM)
string Arb::componentToString(
    #ifdef USE_ARB
    mppp::real x
    #elif defined(GMP_WASM)
    mpfr_t x
    #endif
    , int base) {
    string str = x.to_string(base);
    //Test if negative
    bool negative = false;
    if(str[0] == '-') {
        negative = true;
        str = str.substr(1);
    }
    //Find exponent and set characters to uppercase
    int e = 0;
    for(int i = 0;i < str.length();i++) {
        //Find exponent
        if((str[i] == 'e' || str[i] == '@') && (str[i + 1] == '+' || str[i + 1] == '-')) {
            str[i] = 'e';
            e = i;
        }
        //Set characters to uppercase
        else if(str[i] >= 'a' && str[i] <= 'z') {
            str[i] = str[i] + 'A' - 'a';
        }
    }
    int exp;
    #ifdef USE_ARB
    int prec = Arb::precisionToDigits(x.get_prec());
    #else
    int prec = x.prec();
    #endif
    //Move decimal point if exponent is close to zero
    if(e == 0) exp = 0, e = str.length();
    else exp = stoi(str.substr(e + 1));
    if(exp < 12 && exp > 0 && exp < prec) {
        str.erase(str.begin() + e, str.end());
        str.erase(str.begin() + 1);
        str.insert(str.begin() + exp + 1, '.');
    }
    if(exp > -8 && exp < 0) {
        str.erase(str.begin() + e, str.end());
        str.erase(str.begin() + 1);
        str = "0." + string(-exp - 1, '0') + str;
        e += -exp;
    }
    //Round up trailing characters
    const static string baseChars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    int i = e - 2;
    while(str[i] >= baseChars[(i == e - 2) ? base / 2 : base]) {
        str[i] = '0';
        if(i == 0) { str = "1" + str;e++; }
        else {
            if(str[i - 1] == '.') i--;
            str[i - 1] = baseChars[baseChars.find(str[i - 1]) + 1];
            i--;
        }
    }
    //Erase extra precision
    str.erase(str.begin() + e - 2, str.begin() + e);
    //Remove trailing zeroes
    i = e - 3;
    while(str[i] == '0') {
        str.erase(str.begin() + i);
        i--;
    }
    if(str[i] == '.') str.erase(str.begin() + i);
    if(negative) str = "-" + str;
    str += "p" + std::to_string(prec);
    return str;
}
string Arb::toStr(ParseCtx& ctx, int base)const {
    string out;
    string unitStr;
    arbType n = num;
    if(!unit.isUnitless()) {
        double outputRatio = 1;
        unitStr = "[" + unit.toString(&outputRatio) + "]";
        if(outputRatio != 1) {
            n = n / arbType(outputRatio, getPrec());
        }
    }
    if(Math::isInf(n)) out += "inf";
    else if(Math::isNan(n)) return "nan";
    else out += Arb::componentToString(n, base);
    return out + unitStr;
}
#endif
string Vector::toStr(ParseCtx& ctx, int base)const {
    string out = "<";
    for(int i = 0;i < vec.size();i++) {
        out += vec[i]->toStr(ctx, base);
        if(i != vec.size() - 1) out += ",";
    }
    return out + ">";
}
string String::toStr(ParseCtx& ctx, int base)const {
    return "\"" + String::safeBackspaces(str) + "\"";
}
string Lambda::toStr(ParseCtx& ctx, int base)const {
    string out;
    if(inputNames.size() == 0) out += "_";
    else if(inputNames.size() == 1) out += inputNames[0];
    else {
        out += "(";
        for(int i = 0;i < inputNames.size() - 1;i++) out += inputNames[i] + ",";
        out += inputNames.back() + ")";
    }
    ctx.push(inputNames);
    out += "=>";
    out += func->toStr(ctx, base);
    ctx.pop();
    return out;
}
string Map::toStr(ParseCtx& ctx, int base)const {
    string out = "{";
    //Add each element individually
    for(auto p : mapObj) {
        out += p.first->toStr(ctx, base);
        out += ":";
        out += p.second->toStr(ctx, base);
        out += ",";
    }
    //Delete trailing comma
    if(out.back() == ',') out.erase(out.size() - 1);
    out += "}";
    return out;
}
string Tree::toStr(ParseCtx& ctx, int base)const {
    string out = Program::globalFunctions[op].getName();
    //Reduce run(a,1,2,3) to a(1,2,3) if a is a variable
    if(out == "run") {
        if(branches[0]->typeID() == Value::var_t || branches[0]->typeID() == Value::arg_t) {
            out = branches[0]->toStr(ctx, base);
            out += "(";
            for(int i = 1;i < branches.size();i++) {
                out += branches[i]->toStr(ctx, base);
                if(i != branches.size() - 1) out += ",";
            }
            out += ")";
            return out;
        }
    }
    //Negation
    if(out == "neg") {
        //Complex numbers
        if(branches[0]->typeID() == Value::num_t && branches[0].cast<Number>()->num.imag() != 0)
            return "neg(" + branches[0]->toStr(ctx, base) + ")";
        else if(branches[0]->typeID() == Value::tre_t) {
            string opName = Program::globalFunctions[branches[0].cast<Tree>()->op].getName();
            if(opName != "add" && opName != "sub") {
                return "-" + branches[0]->toStr(ctx, base);
            }
            else return "neg(" + branches[0]->toStr(ctx, base) + ")";
        }
        else return "-" + branches[0]->toStr(ctx, base);
    }
    //Operators
    if(out == "add" || out == "sub" || out == "mul" || out == "div" || out == "pow") {
        const static std::map<string, char> ops = { {"add",'+'},{"sub",'-'},{"mul",'*'},{"div",'/'},{"pow",'^'} };
        char operatorSymbol = ops.at(out);
        bool lhsParenthesis = false, rhsParenthesis = false;
        int lhsType = branches[0]->typeID();
        int rhsType = branches[1]->typeID();
        //Parenthesis on complex numbers
        if(out != "add" && out != "sub")
            if(lhsType == Value::num_t && branches[0].cast<Number>()->num.imag() != 0) lhsParenthesis = true;
        if(out != "add")
            if(rhsType == Value::num_t && branches[1].cast<Number>()->num.imag() != 0) rhsParenthesis = true;
        //left hand side parenthesis
        if(lhsType == Value::tre_t) {
            string name = Program::globalFunctions[branches[0].cast<Tree>()->op].getName();
            if(out == "mul" && (name == "add" || name == "sub")) lhsParenthesis = true;
            if(out == "div" && (name == "add" || name == "sub")) lhsParenthesis = true;
            if(out == "pow" && (name == "add" || name == "sub" || name == "mul" || name == "div")) lhsParenthesis = true;
        }
        //right hand side parenthesis
        if(rhsType == Value::tre_t) {
            string name = Program::globalFunctions[branches[1].cast<Tree>()->op].getName();
            if(out == "sub" && (name == "add" || name == "sub")) rhsParenthesis = true;
            if(out == "mul" && (name == "add" || name == "sub")) rhsParenthesis = true;
            if(out == "div" && (name == "add" || name == "sub" || name == "mul" || name == "div")) rhsParenthesis = true;
            if(out == "pow" && (name == "add" || name == "sub" || name == "mul" || name == "div" || name == "pow")) rhsParenthesis = true;
        }
        //Convert number and variable multiplication, 20*x -> 20x
        if(out == "mul") if(lhsType == Value::num_t) {
            std::shared_ptr<Number> n = branches[0].cast<Number>();
            if(n->num.imag() == 0 && n->unit.isUnitless())
                if(rhsType == Value::var_t || rhsType == Value::arg_t)
                    return branches[0]->toStr(ctx, base) + branches[1]->toStr(ctx, base);
        }
        out = "";
        if(lhsParenthesis) out += "(";
        out += branches[0]->toStr(ctx, base);
        if(lhsParenthesis) out += ")";
        out += operatorSymbol;
        if(rhsParenthesis) out += "(";
        out += branches[1]->toStr(ctx, base);
        if(rhsParenthesis) out += ")";
        return out;
    }
    //Comparison operators
    if(out == "equal" || out == "not_equal" || out == "lt" || out == "lt_equal" || out == "gt" || out == "gt_equal") {
        const static std::map<string, string> ops = { {"equal","=="},{"not_equal","!="},{"lt","<"},{"lt_equal","<="},{"gt",">"},{"gt_equal",">="} };
        string symbol = ops.at(out);
        return "(" + branches[0]->toStr(ctx, base) + symbol + branches[1]->toStr(ctx, base) + ")";
    }
    if(branches.size() == 0) return out;
    out += "(";
    for(int i = 0;i < branches.size();i++) {
        out += branches[i]->toStr(ctx, base);
        if(i != branches.size() - 1) out += ",";
    }
    return out + ")";
}
string Argument::toStr(ParseCtx& ctx, int base)const {
    string out = ctx.getArgName(id);
    if(out == "") return "{argument object " + std::to_string(id) + "}";
    return out;
}
#pragma endregion
bool Value::isOne(const Value& x) {
    if(std::shared_ptr<Number> n = x.cast<Number>()) {
        if(n->num == std::complex<double>(1)) return true;
    }
    return false;
}
bool Value::isZero(const Value& x) {
    if(std::shared_ptr<Number> n = x.cast<Number>()) {
        if(n->num == std::complex<double>(0)) return true;
    }
    return false;
}

const std::vector<string> Value::typeNames = { "number","arb","vector","lambda","string","map","set" };