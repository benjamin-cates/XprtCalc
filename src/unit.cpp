//This file contains the implementations for the unit type. It also contains additional functions that deal with units.
/*
    The unit is stored as a 64-bit integer. Internally, it is treated as an array of eight 8-bit signed integers, using bit shifts to separate them.

    unit[0] = m (meter)
    unit[1] = kg (kilogram)
    unit[2] = s (second)
    unit[3] = A (Ampere)
    unit[4] = K (Kelvin)
    unit[5] = mol (mole)
    unit[6] = $ (currency)
    unit[7] = bit (bit, information)

    Each value in the array is an 8-bit signed integer (from -128 to 127), defaulted to zero;
    This sytem allows a combination of units, for example m/s is {1,0,-1,0,0,0,0,0}
*/
//Unit list constructor techniques
Unit meter = Unit(0x1);
Unit kilogram = Unit(0x100);
Unit second = Unit(0x10000);
Unit amp = Unit(0x1000000);
Unit kelvin = Unit(0x100000000);
Unit mole = Unit(0x10000000000);
Unit dollar = Unit(0x1000000000000);
Unit bit = Unit(0x100000000000000);
Unit newton = kilogram * meter / (second ^ 2);
Unit watt = newton * meter / second;
#include "_header.hpp"

Unit::Unit(unsigned long long b) {
    bits = b;
}
signed char Unit::operator[](int i) {
    return (bits >> (i * 8)) & 0xff;
};
signed char Unit::get(int i)const {
    if(i < 0 || i >= 8) return 0;
    return (bits >> (i * 8)) & 0xff;
}
void Unit::set(int i, signed char value) {
    if(i < 0 || i >= 8) return;
    //Erase bits in current slot
    bits &= 0xFFFFFFFFFFFFFFFFULL ^ (0xFFULL << (i * 8));
    //Set bits in slot
    bits |= ((unsigned long long)(value) << (i * 8));
}
bool Unit::isUnitless()const {
    return bits == 0;
}
unsigned long long Unit::getBits()const {
    return bits;
}
Unit Unit::operator*(Unit a) {
    Unit out;
    //set each to this+a
    for(int i = 0;i < 8;i++)
        out.set(i, get(i) + a.get(i));
    return out;
}
Unit Unit::operator/(Unit a) {
    Unit out;
    //set each to this-a
    for(int i = 0;i < 8;i++)
        out.set(i, get(i) - a.get(i));
    return out;
}
Unit Unit::operator^(double p) {
    Unit out;
    // Set each to unit*p
    for(int i = 0;i < 8;i++)
        out.set(i, get(i) * p);
    return out;
}
Unit Unit::operator+(Unit a) {
    //Returns error unless they are compatible
    if(a.isUnitless()) return (*this);
    if(isUnitless()) return a;
    if(a.getBits() != getBits()) throw "incompatible units";
    return (*this);
}
bool Unit::operator==(const Unit& comp)const {
    return bits == comp.bits;
}
string Unit::toString(double* outputRatio)const {
    //Unit output environment variable
    Value unit_output = Program::computeCtx.getVariable("unit_output");
    if(outputRatio && unit_output != nullptr) {
        if(unit_output->typeID() == Value::map_t) {
            //Iterate through map to find a unit match
            //The map is something like: {[kg]:"lb",[m]:"ft"}. It maps pure units to a string of a different unit name
            std::map<Value, Value>& map = unit_output.cast<Map>()->getMapObj();
            for(auto it = map.begin();it != map.end();it++) {
                if(it->first->typeID() == Value::num_t) {
                    if(it->first.cast<Number>()->unit.bits == bits) {
                        //Set output ratio and return name within map
                        Value ratio = Expression::evaluate("[" + it->second.cast<String>()->str + "]");
                        *outputRatio = ratio->getR();
                        return it->second.cast<String>()->str;
                    }
                }
            }
        }
    }
    //Iterate through each base unit and appends to string
    if(bits == newton.bits) return "N";
    if(bits == (newton * meter).bits) return "J";
    if(bits == watt.bits) return "W";
    if(bits == (newton / (meter ^ 2)).bits) return "Pa";
    if(bits == (watt / amp).bits) return "V";
    string out;
    int exponents[8];
    int negatives = 0;
    int positives = 0;
    for(int i = 0;i < 8;i++) {
        exponents[i] = get(i);
        if(exponents[i] < 0) negatives++;
        if(exponents[i] > 0) positives++;
    }
    if(negatives == 1) {
        string out;
        int i;
        for(i = 0;i < 8;i++) if(exponents[i] < 0) break;
        if(positives == 0)
            out = "1/";
        else for(int x = 0;x < 8;x++) if(x != i && exponents[x] != 0) {
            if(out.length() != 0) out += "*";
            out += Unit::baseUnits[x];
            if(exponents[x] != 1) out += "^" + std::to_string(exponents[x]);
        }
        out += "/";
        out += Unit::baseUnits[i];
        if(exponents[i] != -1) out += "^" + std::to_string(-exponents[i]);
        return out;
    }
    for(int i = 0;i < 8;i++) {
        if(exponents[i] != 0) {
            if(out != "") out += "*";
            //Add base unit and power
            out += Unit::baseUnits[i];
            if(exponents[i] != 1) out += "^" + std::to_string(exponents[i]);
        }
    }
    return out;
}
Unit Unit::parseName(const string& name, double& outCoefficient) {
    //Parse first character being a prefix
    char firstChar = name[0];
    if(name.length() != 1 && Unit::powers.find(firstChar) != Unit::powers.end()) {
        string baseName = name.substr(1);
        if(Unit::listOfUnits.find(baseName) != Unit::listOfUnits.end()) {
            auto data = Unit::listOfUnits.find(baseName)->second;
            if(std::get<2>(data)) {
                //Multiplier * exponent
                outCoefficient = std::get<1>(data) * Unit::powers.find(firstChar)->second;
                //return bits
                return Unit(std::get<0>(data));
            }

        }
    }
    //Regular name search
    if(Unit::listOfUnits.find(name) != Unit::listOfUnits.end()) {
        auto data = Unit::listOfUnits.find(name)->second;
        outCoefficient = std::get<1>(data);
        return Unit(std::get<0>(data));
    }
    return Unit(0);
}
const std::unordered_map<char, double> Unit::powers = {
    {'y',1e-24},//yocto
    {'z',1e-21},//zepto
    {'a',1e-18},//atto
    {'f',1e-15},//fempto
    {'p',1e-12},//pico
    {'n',1e-9},//nano
    {'u',1e-6},//micro
    {'m',1e-3},//milli
    {'c',1e-2},//centi
    {'k',1e3},//kilo
    {'M',1e6},//Mega
    {'G',1e9},//Giga
    {'T',1e12},//Tera
    {'P',1e15},//Peta
    {'Z',1e18},//Zetta
    {'Y',1e21}//Yotta
};
const std::vector<string> Unit::baseUnits = {
    "m","kg","s","A","K","mol","$","bit"
};

std::pair<string, Unit::Builtin> unitConstructor(string symbol, string fullName, Unit u, double coef, bool allowPrefix) {
    return std::pair<string, Unit::Builtin>(symbol, Unit::Builtin(u, coef, allowPrefix, fullName));
}
auto newUnt(string symbol, string fullName, Unit u, double coef = 1.0) {
    return unitConstructor(symbol, fullName, u, coef, false);
}
auto metric(string symbol, string fullName, Unit u, double coef = 1.0) {
    return unitConstructor(symbol, fullName, u, coef, true);
}

//Unit list intself
const std::unordered_map<string, Unit::Builtin> Unit::listOfUnits = {
    metric("m","Meter",meter),
    newUnt("kg","Kilogram",kilogram),
    metric("s","Second",second),
    metric("A","Amp",amp),
    metric("mol","Mole",mole),
    metric("K","Kelvin",kelvin),
    metric("$","Dollar",dollar),
    newUnt("bit","Bit",bit),
    metric("b","Bit",bit),

    //Derived metric base units
    metric("N","Newton",newton),
    metric("J","Joule",newton * meter),
    metric("W","Watt",watt),
    metric("V","Volt",watt / amp),
    metric("Pa","Pascal",newton / (meter ^ 2)),
    metric("bps","Bits per second",bit / second),
    metric("Hz","Hertz",Unit() / second),

    //Normal metric units
    metric("Wh","Watt hour",watt * second,3600),
    metric("Ah","Amp hour",amp * second,3600),
    metric("B","Byte",bit,8),
    metric("Bps","Bytes per second",bit / second,8),
    metric("are","Are",meter ^ 2,1000),
    metric("bar","Bar",newton / (meter ^ 2),100000),
    metric("min","Minute",second,60),
    metric("hr","Hour",second,3600),
    metric("kph","Kilometer per hour",meter / second,5.0 / 18.0),
    metric("tn","Tonne",kilogram,1000),
    metric("g","Gram",kilogram,0.001),

    //Units from the physical world
    newUnt("c","Speed of light",meter / second,299792458),
    newUnt("atm","Atmosphere",newton / (meter ^ 2), 101352),
    metric("eV","Electron Volt",watt * second,0.0000000000000000001602176620898),
    newUnt("mach","Mach",meter / second,340.3),
    metric("pc","Parsec",meter,30857000000000000),

    //Non-metric units
    newUnt("acre","Acre",meter ^ 2,4046.8564224),
    newUnt("btu","British Thermal Unit",kilogram * (meter ^ 2) / (second ^ 2),1054.3503),
    newUnt("ct","Carat",kilogram,0.0002),
    newUnt("day","Day",second,86400.0),
    newUnt("floz","Fluid Ounce",meter ^ 3,0.0000295735295625),
    newUnt("ft","Foot",meter,0.3048),
    newUnt("gallon","Gallon",meter ^ 3,0.00454609),
    newUnt("in","Inch",meter,0.0254),
    newUnt("lb","Pound",kilogram,0.45359237),
    newUnt("lbf","Pound force",newton,4.44822),
    newUnt("mi","Mile",meter,1609.344),
    newUnt("mph","Mile per hour",meter / second,0.4470388888888888),
    newUnt("nmi","Nautical Mile",meter,1852.0),
    newUnt("oz","Ounce",kilogram,0.028349523125),
    newUnt("psi","Pound per square inch",newton / (meter ^ 2),6894.75729316836133),
    newUnt("tbsp","Tablespoon",meter ^ 3,0.00001478676478125),
    newUnt("tsp","Teaspoon",meter ^ 3,0.000000492892159375),
    newUnt("yd","Yard",meter,0.9144),
};