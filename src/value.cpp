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
Vector::Vector(ValPtr topLeft) {
    vec.resize(1);
    vec[0] = topLeft;
}
Vector::Vector(int x) {
    vec.resize(x);
}
int Vector::size()const {
    return vec.size();
}
ValPtr Vector::get(unsigned int x) {
    if(x >= vec.size()) {
        return std::make_shared<Number>(0);
    }
    return vec[x];
}
ValPtr Vector::operator[](unsigned int x) {
    return vec[x];
}
void Vector::set(int x, ValPtr val) {
    if(x >= vec.size()) {
        vec.resize(x + 1);
    }
    vec[x] = val;
}
#pragma endregion
#pragma region Lambda
Lambda::Lambda(std::vector<string> inputs, ValPtr funcTree) {
    inputNames = inputs;
    func = funcTree;
}
ValPtr Lambda::operator()(ValList inputs) {
    if(inputs.size() != inputNames.size()) throw "error wrong number of arguments for lambda";
    ComputeCtx ctx;
    ctx.pushArgs(inputs);
    return func->compute(ctx);
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
ValPtr Map::find(ValPtr key, double flat) {
    if(*key == leafKey) return leaf;
    if(flat == 0) flat = key->flatten();
    if(flat < flatKey) {
        if(left) return left->find(key);
        else return nullptr;
    }
    else {
        if(right) return right->find(key);
        else return nullptr;
    }
    return nullptr;
}
void Map::append(ValPtr key, ValPtr val, double flat) {
    if(*key == leafKey) {
        leaf = val;
        return;
    }
    if(flat == 0) flat = key->flatten();
    if(flat < flatKey) {
        if(left) return left->append(key, val);
        else {
            left = std::make_shared<Map>();
            left->leaf = val;
            left->leafKey = key;
            left->flatKey = flat;
        }
    }
    else {
        if(right) return right->append(key, val);
        else {
            right = std::make_shared<Map>();
            right->leaf = val;
            right->leafKey = key;
            right->flatKey = flat;
        }
    }
}
#pragma endregion
#pragma endregion



#pragma region Value comparison
bool Value::operator==(ValPtr comp) {
    ValPtr sharedThis = shared_from_this();
    if(typeid(this) != typeid(*comp)) return false;
#define cast(type,name1,name2) name1=std::static_pointer_cast<type>(sharedThis);name2=std::static_pointer_cast<type>(comp);
    std::shared_ptr<Number> n1, n2;
#ifdef USE_ARB
    std::shared_ptr<Arb> a1, a2;
#endif
    std::shared_ptr<Vector> v1, v2;
    std::shared_ptr<Lambda> l1, l2;
    std::shared_ptr<String> s1, s2;
    std::shared_ptr<Map> m1, m2;
    switch(typeID()) {
    case Value::num_t:
        cast(Number, n1, n2)
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
        cast(Vector, v1, v2)
            if(v1->size() != v2->size()) return false;
        for(int i = 0;i < v1->size();i++) if(*(v1->vec[i]) != v2->vec[i]) return false;
        return true;
    case Value::lmb_t:
        cast(Lambda, l1, l2)
            if(l1->inputNames.size() != l2->inputNames.size()) return false;
        if(*l1->func == l2->func) return true;
        return false;
    case Value::str_t:
        cast(String, s1, s2)
            if(s1->str == s2->str) return true;
        return false;
    case Value::map_t:
        cast(Map, m1, m2)
            if(*m1->leaf != m2->leaf) return false;
        if((m1->right == nullptr) ^ (m2->right == nullptr)) return false;
        if((m1->left == nullptr) ^ (m2->left == nullptr)) return false;
        if(m1->right) if(*m1->right != m2->right) return false;
        if(m1->left) if(*m1->left != m2->left) return false;
        return true;
    default:
        return false;
    }
    return false;
}
bool Value::operator!=(ValPtr comp) { return !(*this == comp); }
bool Value::operator<(const Value& comp)const { return flatten() < comp.flatten(); }
#pragma endregion
#pragma region Value conversion
ValPtr Value::convert(ValPtr value, int type) {
    int curType = value->typeID();
    if(type == curType) return value;

#define def(type,name) std::shared_ptr<type> name=std::static_pointer_cast<type>(value)
    if(curType == vec_t && type != map_t) { def(Vector, v); return Value::convert(v->get(0), type); }
    if(type == num_t) {
    #ifdef USE_ARB
        if(curType == arb_t) { def(Arb, a); return std::make_shared<Number>(double(a->num.real()), double(a->num.imag()), a->unit); }
    #endif
        if(curType == lmb_t) throw "Cannot convert lambda to number";
        else if(curType == str_t) { def(String, s);return Expression::parseNumeral(s->str, 10); }
        else if(curType == map_t) throw "Cannot convert map to number";
    }
#ifdef USE_ARB
    else if(type == arb_t) {
        if(curType == num_t) { def(Number, n); return std::make_shared<Arb>(n->num.real(), n->num.imag(), n->unit); }
        else if(curType == lmb_t) throw "Cannot convert lambda to arb";
        else if(curType == str_t) { def(String, s);return Expression::parseNumeral("0a" + s->str, 10); }
        else if(curType == map_t) throw "Cannot convert map to arb";
    }
#endif
    else if(type == vec_t) {
        if(curType == map_t) {
            throw "Map to vector conversion not supported yet";
        }
    }
    //To lambda
    else if(type == lmb_t) {
        return std::make_shared<Lambda>(std::vector<string>(), value);
    }
    //To string
    else if(type == str_t) {
        return std::make_shared<String>(value->toString());
    }
    //To map
    else if(type == map_t) {
        std::shared_ptr<Map> out = std::make_shared<Map>();
        if(curType == vec_t) {
            def(Vector, v);
            for(unsigned int i = 0;i < v->size();i++)
                out->append(std::make_shared<Number>(i), v->get(i));
        }
        else out->append(std::make_shared<Number>(0), value);
        return out;
    }
    return std::make_shared<Number>(0);
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
    double hash = 0;
    if(left) hash += left->flatten() * 2.098058329;
    if(right) hash += right->flatten() * 3.209804;
    hash += leafKey->flatten() * 4.012485;
    hash += leaf->flatten() * 1.02305093;
    return hash;
}
double Tree::flatten()const {
    double out = 0.0;
    for(int i = 0;i < branches.size();i++) out += (i * 0.890532 + 1.98235) * branches[i]->flatten();
    return out + op * 1.15241312;
}
#pragma endregion
#pragma region compute
ValPtr Vector::compute(ComputeCtx& ctx) {
    ValList a(vec.size());
    for(int i = 0;i < vec.size();i++) {
        a[i] = vec[i]->compute(ctx);
    }
    return std::make_shared<Vector>(std::move(a));
}
ValPtr Lambda::compute(ComputeCtx& ctx) {
    ValList unreplacedArgs;
    for(int i = 0;i < inputNames.size();i++) {
        unreplacedArgs.push_back(std::make_shared<Argument>(i));
    }
    ctx.pushArgs(unreplacedArgs);
    ValPtr newLambda = func->compute(ctx);
    ctx.popArgs(unreplacedArgs);
    return std::make_shared<Lambda>(inputNames, newLambda);
}
ValPtr Tree::compute(ComputeCtx& ctx) {
    ValList computed(branches.size());
    bool computable = true;
    for(int i = 0;i < branches.size();i++) {
        if(branches[i] == nullptr) branches[i] = Value::zero;
        computed[i] = branches[i]->compute(ctx);
        if(computed[i]->typeID() >= Value::tre_t) computable = false;
    }
    if(computable) return Program::globalFunctions[op](computed, ctx);
    else return std::make_shared<Tree>(op, std::forward<ValList>(computed));
}
ValPtr Argument::compute(ComputeCtx& ctx) {
    return ctx.getArgument(id);
}
#pragma endregion
#pragma region toString
string Value::toString()const { ParseCtx ctx;return this->toStr(ctx); }
string Number::componentToString(double x, int base) {
    std::stringstream s;
    s << x;
    return s.str();
}
string Number::toStr(ParseCtx& ctx)const {
    string out;
    if(num.real() != 0) {
        if(Math::isInf(num.real())) out += "inf";
        else if(Math::isNan(num.real())) return "nan";
        else out += Number::componentToString(num.real(), 10);
    }
    bool imagNeg = num.imag() < 0;
    if(num.imag() != 0) if(imagNeg || num.real() != 0) out += imagNeg ? '-' : '+';
    if(num.imag() != 0) {
        if(Math::isInf(num.imag())) out += "inf*";
        else if(Math::isNan(num.imag())) out += "nan*";
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
    return x.to_string(base);
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
    return out;
}
string Map::toStr(ParseCtx& ctx)const {
    return "{map object}";
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
bool Value::isOne(ValPtr x) {
    if(std::shared_ptr<Number> n = std::dynamic_pointer_cast<Number>(x)) {
        if(n->num == std::complex<double>(1)) return true;
    }
#ifdef USE_ARB
    else if(std::shared_ptr<Arb> n = std::dynamic_pointer_cast<Arb>(x)) {
        if(n->num == std::complex<mppp::real>(1)) return true;
    }
#endif
    return false;
}
bool Value::isZero(ValPtr x) {
    if(std::shared_ptr<Number> n = std::dynamic_pointer_cast<Number>(x)) {
        if(n->num == std::complex<double>(0)) return true;
    }
#ifdef USE_ARB
    else if(std::shared_ptr<Arb> n = std::dynamic_pointer_cast<Arb>(x)) {
        if(n->num == std::complex<mppp::real>(0)) return true;
    }
#endif
    return false;
}

const std::vector<string> Value::typeNames = { "number","arb","vector","lambda","string","map","set" };