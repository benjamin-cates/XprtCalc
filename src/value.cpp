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
    return 0;
}
int Arb::precisionToDigits(mpfr_prec_t prec) {
    return 0;
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
//map<ValPtr, ValPtr> Vector::toMap() {
//    map<ValPtr, ValPtr> out;
//    for(int i = 0;i < vec.size();i++) {
//        if(!vec[i]->isZero()) out[make_shared<Number>(i)] = vec[i];
//    }
//    return out;
//}
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
#pragma region Set
ValPtr Set::orOperator(Set& a) {
    std::shared_ptr<Set> out = std::make_shared<Set>(*this);
    //Add ranges from a
    for(auto it = a.ranges.begin();it != a.ranges.end();it++)
        out->addRange(it->first, it->second);
    //Remove excluded points that are being included by a
    for(auto& point : exclusive)
        if(a.test(point))
            out->addPoint(point);
    //Remove points that are excluded from a
    for(auto& point : a.exclusive) {
        if(!out->test(point)) out->removePoint(point);
    }
    out->optimize();
    return out;
}
ValPtr Set::andOperator(Set& a) {
    return std::make_shared<Set>();
}
ValPtr Set::notOperator() {
    optimize();
    std::shared_ptr<Set> out;
    //Add -infinity to first range
    if(ranges.begin()->first != -INFINITY)
        out->addRange(-INFINITY, ranges.begin()->first);
    //Add ranges to return between current ranges
    for(auto it = std::next(ranges.begin());it != ranges.end();it++) {
        out->addRange(std::prev(it)->second, it->first);
        if(it->first == it->second) out->removePoint(it->first);
    }
    //Add end of ranges to infinity
    if(ranges.end()->second != INFINITY)
        out->addRange(ranges.end()->second, INFINITY);
    //Add point ranges for exclusions
    for(auto it : exclusive)
        out->addRange(it, it);
    return out;
}
void Set::addRange(double start, double end) {
    ranges[start] = end;
    optimize();
}
void Set::removeRange(double start, double end) {
    optimize();
    for(auto it = ranges.begin();it != ranges.end();it++) {
        //If start of removal is within given range
        if(it->first<start && it->second>start) {
            //If entire removal is within one range
            if(it->second > end) addRange(end, it->second);
            //In both cases, move end of range to
            it->second = start;
        }
        //If end of removal is within given range
        else if(it->first<end && it->second>end) {
            double rangeEnd = it->second;
            ranges.erase(it);
            addRange(end, rangeEnd);
        }
    }
}
void Set::removePoint(double r) {
    //If r is a point-sized range, remove the range
    if(ranges.find(r) != ranges.end() && ranges[r] == r)
        ranges.erase(ranges.find(r));
    //Else add it to the excluded list
    else exclusive.insert(r);
}
void Set::addPoint(double a) {
    auto exclusiveIT = exclusive.find(a);
    //If the point is found in the excluded list, remove it from that list
    if(exclusiveIT != exclusive.end()) {
        exclusive.erase(exclusiveIT);
        return;
    }
    //Else add a point-sized range
    else addRange(a, a);
    optimize();
}
bool Set::test(double n) {
    //If n is an excluded point, return false
    if(exclusive.count(n) != 0) return false;
    //If the range starting before it extends past it, return true
    if(ranges.lower_bound(n)->second > n) return true;
    //Else return false
    return false;
}
void Set::optimize() {
    //Find overlapping ranges and merge
    for(auto it = ranges.begin();it != ranges.end();it++) {
        while(it->second >= std::next(it)->first) {
            it->second = it->first;
            ranges.erase(std::next(it));
        }
    }
    //Remove unecessary excluded points
    for(auto it = exclusive.begin();it != exclusive.end();it++) {
        if(ranges.lower_bound(*it)->second < *it)
            exclusive.erase(it);
    }
}
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
    std::shared_ptr<Set> st1, st2;
    switch(typeID()) {
    case 1:
        cast(Number, n1, n2)
            if(!(n1->unit == n2->unit)) return false;
        if(n1->num == n2->num) return true;
        return false;
    case 2:
    #ifdef USE_ARB
        cast(Arb, a1, a2)
            if(!(a1->unit == a2->unit)) return false;
        if(a1->num == a2->num) return true;
    #endif
        return false;
    case 3:
        cast(Vector, v1, v2)
            if(v1->size() != v2->size()) return false;
        for(int i = 0;i < v1->size();i++) if(*(v1->vec[i]) != v2->vec[i]) return false;
        return true;
    case 4:
        cast(Lambda, l1, l2)
            if(l1->inputNames.size() != l2->inputNames.size()) return false;
        if(*l1->func == l2->func) return true;
        return false;
    case 5:
        cast(String, s1, s2)
            if(s1->str == s2->str) return true;
        return false;
    case 6:
        cast(Map, m1, m2)
            if(*m1->leaf != m2->leaf) return false;
        if((m1->right == nullptr) ^ (m2->right == nullptr)) return false;
        if((m1->left == nullptr) ^ (m2->left == nullptr)) return false;
        if(m1->right) if(*m1->right != m2->right) return false;
        if(m1->left) if(*m1->left != m2->left) return false;
        return true;
    case 7:
        cast(Set, st1, st2)
            if(st1->exclusive.size() != st2->exclusive.size()) return false;
        if(st1->ranges.size() != st2->ranges.size()) return false;
        for(auto aIt = st1->exclusive.begin(), bIt = st2->exclusive.begin();aIt != st1->exclusive.end();aIt++) {
            if(*aIt != *bIt) return false;
            bIt++;
        }
        for(auto aItR = st1->ranges.begin(), bItR = st2->ranges.begin();aItR != st1->ranges.end();aItR++) {
            if(aItR->first != bItR->first) return false;
            if(aItR->second != bItR->second) return false;
            bItR++;
        }
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
    if(curType == 3 && type != 6) { def(Vector, v); return Value::convert(v->get(0), type); }
    //To number
    if(type == 1) {
    #ifdef USE_ARB
        if(curType == 2) { def(Arb, a); return std::make_shared<Number>(double(a->num.real()), double(a->num.imag()), a->unit); }
    #endif
        if(curType == 4) throw "Cannot convert lambda to number";
        else if(curType == 5) { def(String, s);return Expression::parseNumeral(s->str, 10); }
        else if(curType == 6) throw "Cannot convert map to number";
        else if(curType == 7) throw "Cannot convert set to number";
    }
#ifdef USE_ARB
    //To arb
    else if(type == 2) {
        if(curType == 1) { def(Number, n); return std::make_shared<Arb>(n->num.real(), n->num.imag(), n->unit); }
        else if(curType == 4) throw "Cannot convert lambda to arb";
        else if(curType == 5) { def(String, s);return Expression::parseNumeral("0a" + s->str, 10); }
        else if(curType == 6) throw "Cannot convert map to arb";
        else if(curType == 7) throw "Cannot convert set to arb";
    }
#endif
    //To vector from map
    else if(type == 3) {
        if(curType == 6) {
            throw "Map to vector conversion not supported yet";
        }
    }
    //To lambda
    else if(type == 4) {
        return std::make_shared<Lambda>(std::vector<string>(), value);
    }
    //To string
    else if(type == 5) {
        return std::make_shared<String>(value->toString());
    }
    //To map
    else if(type == 6) {
        std::shared_ptr<Map> out = std::make_shared<Map>();
        if(curType == 3) {
            def(Vector, v);
            for(unsigned int i = 0;i < v->size();i++)
                out->append(std::make_shared<Number>(i), v->get(i));
        }
        else out->append(std::make_shared<Number>(0), value);
        return out;
    }
    //To set
    else if(type == 7) {
        if(curType == 1 || curType == 2) {
            std::shared_ptr<Set> set = std::make_shared<Set>();
            set->addPoint(value->getR());
            return set;
        }
        if(curType == 3) {
            def(Vector, v);
            std::shared_ptr<Set> set = std::make_shared<Set>();
            for(unsigned int i = 0;i < v->size();i++) {
                ValPtr element = v->get(i);
                double val;
                if(element == nullptr) val = 0;
                else val = element->getR();
                set->addPoint(val);
            }
            return set;
        }
        else if(curType == 4) throw "Cannot convert lambda to set";
        else if(curType == 4) throw "Cannot convert string to set";
        else if(curType == 6) throw "Cannot convert map to set";
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
double Set::flatten()const {
    double out = 0;
    for(auto it = exclusive.begin();it != exclusive.end();it++) out += *it;
    for(auto it = ranges.begin();it != ranges.end();it++) out += it->first * 0.40238877 + it->second;
    return out;
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
        computed[i] = branches[i]->compute(ctx);
        if(computed[i]->typeID() > 7) computable = false;
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
string Set::toStr(ParseCtx& ctx)const {
    return "{set object}";
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