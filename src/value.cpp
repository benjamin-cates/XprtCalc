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
Arb::Arb(mppp::real r, mppp::real i, Unit u) {
    num = std::complex<mppp::real>(r, i);
    unit = u;
}
Arb::Arb(std::complex<mppp::real> n, Unit u) {
    num = n;
    unit = u;
}
mpfr_prec_t Arb::digitsToPrecision(int digits) {
    if(digits < 1) throw "Cannot have " + std::to_string(digits) + " digits of precision";
    constexpr double conv = std::log(10) / std::log(2);
    return mpfr_prec_t(conv * digits + 2);
}
int Arb::precisionToDigits(mpfr_prec_t prec) {
    constexpr double conv = std::log(10) / std::log(2);
    return int(1.0 * (prec - 2) / conv);
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
    string out = str;
    out.resize(out.size() * 2);
    int offset = 0;
    const static std::unordered_map<char, char> escapes = { {'\\','\\'},{'"','"'},{'\n','n'},{'\t','t'} };
    for(int i = 0;i < str.length();i++) {
        if(escapes.at(str[i]) != 0) {
            out[i + offset] = '\\';
            out[i + offset + 1] = escapes.at(str[i]);
            offset++;
        }
        else out[i + offset] = str[i];
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
#pragma endregion
#pragma region Value conversion
Value Value::convertTo(int type) {
    int curType = ptr->typeID();
    if(type == curType) return *this;
    if(type == num_t) {
        #ifdef USE_ARB
        if(curType == arb_t) { std::shared_ptr<Arb> a = cast<Arb>(); return std::make_shared<Number>(double(a->num.real()), double(a->num.imag()), a->unit); }
        #endif
        if(curType == lmb_t) throw "Cannot convert lambda to number";
        else if(curType == str_t) return Expression::parseNumeral(cast<String>()->str, 10);
        else if(curType == map_t) throw "Cannot convert map to number";
    }
    #ifdef USE_ARB
    else if(type == arb_t) {
        if(curType == num_t) { std::shared_ptr<Number> n = cast<Number>(); return std::make_shared<Arb>(n->num.real(), n->num.imag(), n->unit); }
        else if(curType == lmb_t) throw "Cannot convert lambda to arb";
        else if(curType == str_t) return Expression::parseNumeral("0a" + cast<String>()->str, 10);
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
        return std::make_shared<Lambda>(std::vector<string>(), *this);
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
    #ifdef USE_ARB
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
double Arb::flatten()const { return double(num.real() * 5.132441 + num.imag() * 7.14441 + double(unit.getBits())); }
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
    ValList unreplacedArgs;
    for(int i = 0;i < inputNames.size();i++) {
        unreplacedArgs.push_back(std::make_shared<Argument>(i));
    }
    //Push arguments to stack
    ctx.pushArgs(unreplacedArgs);
    Value newLambda;
    try { newLambda = func->compute(ctx); }
    catch(...) { ctx.popArgs(unreplacedArgs);throw; }
    ctx.popArgs(unreplacedArgs);
    return std::make_shared<Lambda>(inputNames, newLambda);
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
        computed[i] = branches[i]->compute(ctx);
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
string ValueBaseClass::toString()const { return this->toStr(Program::parseCtx); }
string Number::componentToString(double x, int base) {
    if(x == 0) return "0";
    bool isNegative = x < 0;
    x = std::abs(x);
    double exponent = std::floor(std::log(x * 1.00000000000001) / std::log(double(base)));
    x /= std::pow(base, exponent);
    //Floating point
    if(exponent > 9.0 || exponent < -7.0) {
        return componentToString(x, base) + "e" + componentToString(double(exponent), base);
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
    for(i = 0;i < 10;i++) {
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
    if(i == 10 && digitList.back() >= base / 2) {
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
string Number::toStr(ParseCtx& ctx)const {
    string out;
    if(num.real() != 0) {
        if(Math::isInf(num.real())) out += "inf";
        else if(Math::isNan(num.real())) return "nan";
        else out += Number::componentToString(num.real(), 10);
    }
    if(num.imag() != 0) {
        if(num.imag() > 0 && num.real() != 0) out += '+';
        if(Math::isInf(num.imag())) out += "+inf*";
        else if(Math::isNan(num.imag())) out += "+nan*";
        else out += Number::componentToString(num.imag(), 10);
        out += 'i';
    }
    if(out == "") out = "0";
    if(!unit.isUnitless()) {
        out += "[" + unit.toString() + "]";
    }
    return out;
}
#ifdef USE_ARB
string Arb::componentToString(mppp::real x, int base) {
    string str = x.to_string(base);
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
    //Move decimal point if exponent is close to zero
    int exp;
    if(e == 0) exp = 0, e = str.length();
    else exp = stoi(str.substr(e + 1));
    if(exp < 8 && exp > 0) {
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
        if(i == 0) {str = "1" + str;e++;}
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
    return str;
}
string Arb::toStr(ParseCtx& ctx)const {
    string out;
    if(num.real() != 0) {
        if(Math::isInf(num.real())) out += "inf";
        else if(Math::isNan(num.real())) return "nan";
        else out += Arb::componentToString(num.real(), 10);
    }
    bool imagNeg = num.imag() < 0;
    if(num.imag()) if(imagNeg || num.real() != 0) out += imagNeg ? '-' : '+';
    if(num.imag() != 0) {
        if(Math::isInf(num.imag())) out += "inf*";
        else if(Math::isNan(num.imag())) out += "nan*";
        else out += Arb::componentToString(num.imag(), 10);
        out += 'i';
    }
    if(out == "") out = "0";
    if(!unit.isUnitless()) {
        out += "[" + unit.toString() + "]";
    }
    return out;
}
#endif
string Vector::toStr(ParseCtx& ctx)const {
    string out = "<";
    for(int i = 0;i < vec.size();i++) {
        out += vec[i]->toStr(ctx);
        if(i != vec.size() - 1) out += ",";
    }
    return out + ">";
}
string String::toStr(ParseCtx& ctx)const {
    return "\"" + str + "\"";
}
string Lambda::toStr(ParseCtx& ctx)const {
    string out;
    if(inputNames.size() == 0) out += "_";
    else if(inputNames.size() == 1) out += inputNames[0];
    else {
        out += "(";
        for(int i = 0;i < inputNames.size() - 1;i++) out += inputNames[i] + ",";
        out += inputNames.back() + ")";
    }
    ctx.push(inputNames);
    out += "=>(";
    out += func->toStr(ctx) + ")";
    ctx.pop();
    return out;
}
string Map::toStr(ParseCtx& ctx)const {
    string out = "{";
    //Add each element individually
    for(auto p : mapObj) {
        out += p.first->toStr(ctx);
        out += ":";
        out += p.second->toStr(ctx);
        out += ",";
    }
    //Delete trailing comma
    if(out.back() == ',') out.erase(out.size() - 1);
    out += "}";
    return out;
}
string Tree::toStr(ParseCtx& ctx)const {
    string out = Program::globalFunctions[op].getName();
    if(branches.size() == 0) return out;
    out += "(";
    for(int i = 0;i < branches.size();i++) {
        out += branches[i]->toStr(ctx);
        if(i != branches.size() - 1) out += ",";
    }
    return out + ")";
}
string Argument::toStr(ParseCtx& ctx)const {
    string out = ctx.getArgName(id);
    if(out == "") return "{argument object " + std::to_string(id) + "}";
    return out;
}
#pragma endregion
bool Value::isOne(const Value& x) {
    if(std::shared_ptr<Number> n = x.cast<Number>()) {
        if(n->num == std::complex<double>(1)) return true;
    }
    #ifdef USE_ARB
    else if(std::shared_ptr<Arb> n = x.cast<Arb>()) {
        if(n->num == std::complex<mppp::real>(1)) return true;
    }
    #endif
    return false;
}
bool Value::isZero(const Value& x) {
    if(std::shared_ptr<Number> n = x.cast<Number>()) {
        if(n->num == std::complex<double>(0)) return true;
    }
    #ifdef USE_ARB
    else if(std::shared_ptr<Arb> n = x.cast<Arb>()) {
        if(n->num == std::complex<mppp::real>(0)) return true;
    }
    #endif
    return false;
}

const std::vector<string> Value::typeNames = { "number","arb","vector","lambda","string","map","set" };