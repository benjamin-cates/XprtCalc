#include "_header.hpp"
using namespace Help;
string Page::toPlainText() {
    string out(content.length(), ' ');
    int offset = 0;
    for(int i = 0;i < content.length();i++) {
        if(content[i] == '`' || content[i] == '?') offset++;
        else out[i - offset] = content[i];
    }
    out.resize(content.length() - offset);
    return out;
}
string Page::toString() {
    string out;
    out += name;
    if(symbol.length()) out += " - " + symbol;
    if(type.length()) out += " - " + type;
    out += "\n" + toPlainText();
    if(seeMore.length()) out += "\n\nSee more: " + seeMore + "\n";
    return out + "\n";
}
string Page::toJSON() {
    string alias = "[";
    for(auto it = aliases.begin();it != aliases.end();it++) {
        alias += "\"" + String::safeBackspaces(*it) + "\"";
    }
    alias += ']';
    return "{name:\"" + name + "\", symbol: \"" + String::safeBackspaces(symbol) + "\", type: \"" + type + "\", aliases:" + alias + ", content: \"" + String::safeBackspaces(content) + "\"}";
}
Page Page::fromUnit(string n, string message, std::vector<string> aliases, string more) {
    Page out;
    auto it = Unit::listOfUnits.find(n);
    if(it == Unit::listOfUnits.end()) throw "Cannot find " + n;
    out.name = std::get<3>(it->second);
    out.symbol = "[" + n + "]";
    out.type = "unit";
    out.content = message;
    out.content += " " + out.name + " has base units: [" + std::get<0>(it->second).toString() + "]` with coefficient `" + std::to_string(std::get<1>(it->second)) + "`.";
    if(std::get<2>(it->second))
        out.content += " " + out.name + " supports ?metric prefixes?.";
    else out.content += " " + out.name + " does not support ?metric prefixes?.";
    out.seeMore = more;
    out.aliases = aliases;
    return out;
}
Page Page::fromFunction(string n, string sym, string message, std::vector<string> aliases, string more) {
    Page out;
    Function& func = Program::globalFunctions[Program::globalFunctionMap[sym]];
    std::vector<string>& inputs = func.getInputNames();
    out.symbol = sym;
    for(int i = 0;i < inputs.size();i++)
        out.symbol += (i == 0 ? "(" : ",") + inputs[i];
    if(inputs.size() != 0) out.symbol += ")";
    out.name = n;
    out.aliases = aliases;
    out.content = message;
    std::vector<Function::Domain> dom = func.getDomain();
    if(dom.size() == 1 && dom[0].sig == 0) out.content += " `" + sym + "` takes no arguments.";
    else {
        out.content += " Acceptable inputs are: ";
        for(int i = 0;i < dom.size();i++) {
            if(i != 0) out.content += ", ";
            out.content += dom[i].toString();
        }
        out.content += ". See ?types? for more info.";
    }
    out.seeMore = more;
    out.type = "function";
    return out;
}
Page Page::fromLibrary(string n, string message, std::vector<string> aliases, string more) {
    Page out;
    Library::LibFunc& func = Library::functions[n];
    out.name = func.fullName;
    out.content = message;
    out.content += " `" + n + "` compiles from: `" + func.xpr + "`.";
    if(func.dependencies.size() != 0) {
        out.content += "The function is will compile";
        if(func.dependencies.size() == 1) out.content += " ?" + func.dependencies[0] + "? ";
        else {
            for(int i = 0;i < func.dependencies.size() - 1;i++) out.content += string(i == 0 ? ", " : " ") + "?" + func.dependencies[i] + "?";
            out.content += ", and ?" + func.dependencies.back() + "? ";
        }
        out.content += " before it is included.";
    }
    out.aliases = aliases;
    out.symbol = n;
    out.type = "library";
    out.seeMore = more;
    return out;
}

std::vector<Page> Help::pages = {
    #pragma region Introduction
    //Info
    Page("Welcome","","guide","Welcome to XprtCalc, use the \"/query\" command to search the help pages, or type \"/help introduction\" to read about the basic functionality, info can be found in the page called ?info?.",{"help"}),
    #pragma endregion
    #pragma region Types
    //Number
    //Arb
    //Vectors
    //Lambdas
    //Strings
    //Maps
    #pragma endregion
    #pragma region Guides
    //List of operators
    //Units guide
    //Base units explanation
    //Metric prefixes
    //Base conversion
    //Commands
    //Comments
    //Applying vectors
    //Lambda capturing
    //Variables
    //Preferences
    //
    #pragma endregion
    #pragma region Functions
    #pragma region Elementary
    Page::fromFunction("Negate","neg","Returns the negative of `x`. It is also aliased by the prefix operator '`-`'.",{"-"}),
    Page::fromFunction("Addition","add","Returns `a + b`. If both are strings, they will be concatenated. It is also aliased by the  operator '`+`'.",{"+","sum","plus"}),
    Page::fromFunction("Subtraction","sub","Returns `a - b`. It is also aliased by the operator '`-`'.",{"-","difference","minus"}),
    Page::fromFunction("Multiply","mult","Returns `a * b`. It is also aliased by the operator '`*`'.",{"*","product","times"}),
    Page::fromFunction("Divide","div","Returns `a / b`. It is also aliased by the operator '`/`'.",{"/","over","quotient"}),
    Page::fromFunction("Power","pow","Returns `a ^ b`. It is also aliased by the operators '`^`' and '`**`'.",{"**","^","exponent","exponentiation"},"https://en.wikipedia.org/wiki/Exponentiation"),
    Page::fromFunction("Modulo","mod","Returns `a % b`, which is the remainder when `a` is divided by `b`. It is also aliased by the operator '`%`'.",{"%","modulus"},"https://en.wikipedia.org/wiki/Modulo_operation"),
    Page::fromFunction("Square Root","sqrt","Returns the square root of `x`, which is equivalent to raising it by `0.5`.",{"power","exponent","root"},"https://en.wikipedia.org/wiki/Square_root"),
    Page::fromFunction("Exponent","exp","Returns the `e ^ x`, which means ?Euler's number? raised to the power of `x`.",{"e","e^x","power","exponent",},"https://en.wikipedia.org/wiki/Exponential_function"),
    Page::fromFunction("Natural log","ln","Returns the natural logarithm of `x`, it is the inverse of `?exp?`. For an arbitrary base, see the `?logb?` function.",{"logarithm"},"https://en.wikipedia.org/wiki/Natural_logarithm"),
    Page::fromFunction("Logarithm","log","Returns the logarithm base 10 of `x`, which means `10^ln(x)` is `x`. For an arbitrary base, see the `?logb?` function.",{"log"},"https://en.wikipedia.org/wiki/Logarithm"),
    Page::fromFunction("Logarithm base b","logb","Returns the logarithm base `b` of `x`, which means `b^logb(x,b)` is `x`. For specialized bases, see the `?exp?` and `?log?` functions.",{"log"},"https://en.wikipedia.org/wiki/Logarithm"),
    Page::fromFunction("Gamma","gamma","Returns the gamma function of `x`. It currently does not support imaginary numbers. Equivalent to `?factorial?(x+1)`.",{},"https://en.wikipedia.org/wiki/Gamma_function"),
    Page::fromFunction("Factorial","factorial","Returns the factorial of `x`. It currently does not support imaginary numbers. It is also aliased by the `!` operator. Example: `4!+2` means `factorial(4)+2`. Factorial is equivalent to `?gamma?(x-1)`.",{},"https://en.wikipedia.org/wiki/Gamma_function"),
    Page::fromFunction("Error function","erf","Returns the error function of `x`. It currently does not support imaginary numbers. It does not have much use in simple mathematics, however support is there for who needs it.",{},"https://en.wikipedia.org/wiki/Error_function"),
    #pragma endregion
    #pragma region Trigonometry
    Page::fromFunction("Sine","sin","Returns the sine of `x`, which is the opposite side divided by hypotenuse of a right triangle with angle `x`",{"trigonometry"},"https://en.wikipedia.org/wiki/Sine_and_cosine"),
    Page::fromFunction("Cosine","cos","Returns the cosine of `x`, which is the adjacent side divided by hypotenuse of a right triangle with angle `x`",{"trigonometry"},"https://en.wikipedia.org/wiki/Sine_and_cosine"),
    Page::fromFunction("Tangent","tan","Returns the tangent of `x`. Equivalent to `sin(x)/cos(x)` or the opposite side divided by the adjacent side of a right triangle with angle `x`.",{"trigonometry"},"https://en.wikipedia.org/wiki/Trigonometric_functions#Right-angled_triangle_definitions"),
    Page::fromFunction("Cosecant","csc","Returns the cosecant of `x`. Equivalent to `1/sin(x)` or the hypotenuse divided by the opposite side of a right triangle with angle `x`.",{"trigonometry"},"https://en.wikipedia.org/wiki/Trigonometric_functions#Right-angled_triangle_definitions"),
    Page::fromFunction("Secant","sec","Returns the secant of `x`. Equivalent to `1/cos(x)` or the hypotenuse divided by the adjacent side of a right triangle with angle `x`.",{"trigonometry"},"https://en.wikipedia.org/wiki/Trigonometric_functions#Right-angled_triangle_definitions"),
    Page::fromFunction("Cotangent","cot","Returns the cotangent of `x`. Equivalent to `1/tan(x)`, `cos(x)/sin(x)` or the adjacent divided by the opposite side of a right triangle with angle `x`.",{"trigonometry"},"https://en.wikipedia.org/wiki/Trigonometric_functions#Right-angled_triangle_definitions"),
    Page::fromFunction("Hyperbolic sine","sinh","Returns the hyperbolic sine of `x`.",{"trigonometry"},"https://en.wikipedia.org/wiki/Hyperbolic_functions"),
    Page::fromFunction("Hyperbolic cosine","cosh","Returns the hyperbolic cosine of `x`.",{"trigonometry"},"https://en.wikipedia.org/wiki/Hyperbolic_functions"),
    Page::fromFunction("Hyperbolic tangent","tanh","Returns the hyperbolic tangent of `x`.",{"trigonometry"},"https://en.wikipedia.org/wiki/Hyperbolic_functions"),
    Page::fromFunction("Inverse sine","asin","Returns the inverse ?sine? of `x`. `asin(sin(x)) = x` for `-pi/2 < x < pi/2`",{"trigonometry","arcsine"},"https://en.wikipedia.org/wiki/Inverse_trigonometric_functions"),
    Page::fromFunction("Inverse cosine","acos","Returns the inverse ?cosine? of `x`. `acos(cos(x)) = x` for `0 < x < pi`",{"trigonometry","arccosine"},"https://en.wikipedia.org/wiki/Inverse_trigonometric_functions"),
    Page::fromFunction("Inverse tangent","atan","Returns the inverse ?tangent? of `x`. `atan(tan(x)) = x` for `-pi/2 < x < pi/2`",{"trigonometry","arctan"},"https://en.wikipedia.org/wiki/Inverse_trigonometric_functions"),
    Page::fromFunction("Inverse hyperbolic sine","asinh","Returns the inverse ?hyperbolic sine? of `x`. `asinh(sinh(x)) = x`.",{"trigonometry","hyperbolic arcsine"},"https://en.wikipedia.org/wiki/Inverse_hyperbolic_functions"),
    Page::fromFunction("Inverse hyperbolic cosine","asinh","Returns the inverse ?hyperbolic cosine? of `x`. `acosh(cosh(x)) = x` for `x >= 1`.",{"trigonometry","hyperbolic arccosine"},"https://en.wikipedia.org/wiki/Inverse_hyperbolic_functions"),
    Page::fromFunction("Inverse hyperbolic tangent","asinh","Returns the inverse ?hyperbolic tangent? of `x`. `atanh(tanh(x)) = x`.",{"trigonometry","hyperbolic arctangent"},"https://en.wikipedia.org/wiki/Inverse_hyperbolic_functions"),
    #pragma endregion
    #pragma region Numeric
    Page::fromFunction("Round","round","Returns the nearest integer to `x`. If the decimal component of `x >= 0.5`, it will round up, else it will round down.",{},"https://en.wikipedia.org/wiki/Rounding#Round_half_up"),
    Page::fromFunction("Floor","floor","Returns the nearest integer that is less than `x`.",{"round"},"https://en.wikipedia.org/wiki/Floor_and_ceiling_functions"),
    Page::fromFunction("Ceiling","ceil","Returns the nearest integer that is greater than `x`.",{"round"},"https://en.wikipedia.org/wiki/Floor_and_ceiling_functions"),
    Page::fromFunction("Real component","getr","Returns the real component of an imaginary number. Example: `getr(2+i) = 2`. Use `geti` to get the imaginary component."),
    Page::fromFunction("Imaginary component","geti","Returns the imaginary component of an imaginary number. Example: `geti(2+i) = 1`. Use `getr` to get the real component."),
    Page::fromFunction("Unit component","getu","Returns the unit component of a number multiplied by 1. Example: `getu(2[m*kg]) = [m*kg]`."),
    Page::fromFunction("Maximum","max","Returns the largest of `a` and `b`. The alternative input is if the first argument is a ?vector?, it will return the largest element."),
    Page::fromFunction("Minimum","min","Returns the smallest of `a` and `b`. The alternative input is if the first argument is a ?vector?, it will return the smallest element."),
    Page::fromFunction("Linear interpolation","lerp","Returns the linear interpolation from `a` to `b` with time `x`. It effectively returns `a + (b-a)*t`.",{"linear interpolate","linear extrapolat"},"https://en.cppreference.com/w/cpp/numeric/lerp"),
    Page::fromFunction("Distance","dist","Returns the distance from `a` to `b`. If both a real, it is equivalent to `abs(a-b)`. For an ?imaginary? number or a ?vector? the Pythagorean theorem is used and they are treated as points.",{"hypotenuse"}),
    Page::fromFunction("Sign","sgn","Returns the sign of the value `x`. If `x` is positive, it returns `1`. If it is negative, it returns `-1`. For ?imaginary? numbers, it returns the unit vector. `sgn` is equivalent to `x/abs(x)`.",{"signum"},"https://en.wikipedia.org/wiki/Sign_function"),
    Page::fromFunction("Absolute value","abs","Returns the distance of `x` from zero. For ?imaginary? numbers it returns `sqrt(r^2 + i^2)`. The notation |x| is not supported.",{"distance"},"https://en.wikipedia.org/wiki/Absolute_value"),
    Page::fromFunction("Argument","arg","Returns the angle between 1+0i and `x`. The function's range is from `-pi` to `pi`. It is equivalent to `atan2(geti(x),getr(x))`.",{"complex argument","angle between"},"https://en.wikipedia.org/wiki/Argument_(complex_analysis)"),
    Page::fromFunction("Arctangent 2","atan2","Returns the angle between the point `(y,x)` and `(0,1)`. It is similar to the `arg` function.",{"inverse tangent"},"https://en.wikipedia.org/wiki/Atan2"),
    #pragma endregion
    #pragma region Comparison
    Page::fromFunction("Equal","equal","Returns `1` if `a` and `b` are equal, else returns `0`. It is aliased by the `==` and `=` operators.",{"equivalent","comparison","=="}),
    Page::fromFunction("Not equal","not_equal","Returns `1` if `a` and `b` are not equal, else returns `0`. It is aliased by the `!=` operator.",{"equivalent","comparison","!="}),
    Page::fromFunction("Less than","lt","Returns `1` if `a` is less than `b`, else returns `0`. It is aliased by the `<` operator. ?Imaginary? components are ignored.",{"comparison","<"}),
    Page::fromFunction("Greater than","gt","Returns `1` if `a` is greater than `b`, else returns `0`. It is aliased by the `>` operator. ?Imaginary? components are ignored",{"comparison",">"}),
    Page::fromFunction("Greater than or equal","gt_equal","Returns `1` if `a` is ?greater than? or ?equal? to `b`, else returns `0`. It is aliased by the `>=` operator. ?Imaginary? components are ignored",{"comparison",">="}),
    Page::fromFunction("Less than or equal","lt_equal","Returns `1` if `a` is ?less than? or ?equal? to `b`, else returns `0`. It is aliased by the `<=` operator. ?Imaginary? components are ignored",{"comparison","<="}),
    #pragma endregion
    #pragma region Constants
    Page::fromFunction("Imaginary number","i","Constant that returns the imaginary number `i`. `i^2` is equal to `-1`, and it is used a lot in higher level math and algebra.",{"constant"},"https://en.wikipedia.org/wiki/Imaginary_number"),
    Page::fromFunction("Pi","pi","Constant that retuns the number pi. Pi is the ratio between the circumference ofa circle and it's diameter. Pi is used a lot in ?trigonometry?. Pi returns a double precision float, see `arb_pi` for a more accurate calculation.",{"constant"},"https://en.wikipedia.org/wiki/Pi"),
    Page::fromFunction("Euler's number","e","Constant that retuns euler's number, which is the base of natural logarithms. `e` is used a lot in math past algebra. `e` returns a double precision float, see `arb_e` for a more accurate calculation.",{"constant"},"https://en.wikipedia.org/wiki/E_(mathematical_constant)"),
    Page::fromFunction("Arbitrary pi","arb_pi","Returns `pi` to `prec` decimal digits as an `arbitrary precision` float. See `pi` for the 15-digit approximation."),
    Page::fromFunction("Arbitrary Euler's number","arb_e","Returns Euler's number to `prec` decimal digits as an `arbitrary precision` float. See `e` for the 15-digit approximation."),
    Page::fromFunction("Random","rand","Returns a unique random number between zero and one every time it is calculated. See `srand` for seeding."),
    Page::fromFunction("Random seed","srand","Takes in a float as a random seed and seeds the random number generator. If the same seed is provided, `rand` will return the exact same sequence of random numbers.",{},"https://en.wikipedia.org/wiki/Random_seed"),
    Page::fromFunction("Not available number","nan","Constant for the not available number floating point exception. This constant is not particularly useful, but included for completeness sake"),
    Page::fromFunction("Infinity","inf","Constant for the floating point infinity. Please do not use this for calculations, only comparison with other infinities."),
    Page::fromFunction("History length","histlen","Constant for the number of elements in the ?history?",{"memory"}),
    Page::fromFunction("Answer","ans","Returns the previously calculated value. If an argument is provided, it returns the `x`th element in the ?history?, or if `x` is negative, `x` values in the past (negative one being the previous value).",{"history","memory"}),
    #pragma endregion
    #pragma region Lambdas
    Page::fromFunction("Run","run","Runs the ?lambda? function `func` with the arguments provided. If not enough arguments are provided, zeroes are filled in. Example: `run((x,y)=>x+y, 5, 4) = 9`. If you have a ?vector? of arguments, use the `apply` function.",{"compute","lambda"}),
    Page::fromFunction("Apply","apply","Runs the ?lambda? function `func` on each element in the ?vector? `args` and returns the resultant vector. If `func` is a ?string?, it will run that global ?function? on each element. Example: `apply(x=>x/2,<4,8,9>) = <2,4,4.5>`.",{"vector","lambda"}),
    Page::fromFunction("Sum","sum","Returns the sum of each ?lambda? `func` run from `begin` to `end` with an optional `step`. If a step is not included, `step` defaults to `1`. Example: `sum(x=>x+1,0,5,1) = 1+2+3+4+5+6 = 21`",{"addition","plus","series","lambda"},"https://en.wikipedia.org/wiki/Summation#Capital-sigma_notation"),
    Page::fromFunction("Product","product","Returns the product of each ?lambda? `func` run from `begin` to `end` with an optional `step`. If a step is not included, `step` defaults to `1`. Example: `sum(x=>x+1,0,5,1) = 1*2*3*4*5*6 = 6! = 720`",{"multiply","multiplication","times","series","lambda"},"https://en.wikipedia.org/wiki/Product_(mathematics)#Product_of_a_sequence"),
    #pragma endregion
    #pragma region Vectors
    Page::fromFunction("Length","length","Returns the number of elements in `obj` or the length of a ?string?. Example: `length(<1,2,4.5>) = 3`."),
    Page::fromFunction("Magnitude","magnitude","Returns the magnitude of a vector, which is the distance from zero to that point.",{"distance"},"https://en.wikipedia.org/wiki/Magnitude_(mathematics)#Euclidean_vector_space"),
    Page::fromFunction("Normalize","normalize","Returns a vector divided by it's magnitude, so the magnitude of the return value is `1`.",{},"https://en.wikipedia.org/wiki/Unit_vector"),
    Page::fromFunction("Get element","get","Returns the element at `key` in `map`. If `map` is a vector, it returns the element with index `key`, with the first element having index `0`. If map is a ?map? type, it returns the value at that key. In either case, if the key is outside of bounds, `get` returns `0`.",{"access"}),
    Page::fromFunction("Fill vector","fill","Returns a ?vector? with `count` elements generated by `func`. The first index is `0`. Example: `fill(x=>x/2,5) = <0,0.5,1,1.5,2>`",{"lambda","vector","generate"}),
    Page::fromFunction("Map vector","map_vector","Returns a new ?vector? which is the result of each element in map being passed through `func`. Example: `map_vector(<4,0,-1>,x=>sqrt(x)) = <2,0,i>`."),
    Page::fromFunction("Concatenate","concat","Retuns the concatenation of the two ?vector? objects `a` and `b`. To concatenate ?string? objects, use the `add` function. Example: `concat(<1,2>,<4,3>) = <1,2,4,3>`.",{"addition","join"}),
    Page::fromFunction("Sort","sort","Returns the sorted form of `vec` using the optional `comp` ?lambda? function. If `comp` is not passed, it defaults to the less than funcition `lt`. Examples: `sort(<4,-5,2.3>) = <-5,2.3,4>`, `sort(<\"abc\",\"b\",\"jh\">,(a,b)=>length(a)<length(b)) = <\"b\",\"jh\",\"abc\">`. Internally, this uses the merge sort algorithm.",{"order"}),
    #pragma endregion
    #pragma region Strings
    Page::fromFunction("Evaluate","eval","Returns the calculation of the ?string? `str` in the global context. Eval is not aware of local variables or arguments, so `x=>eval(\"x\")` will not work. Example: `eval(\"4-2\") = 2`.",{"string","calculation","calculate"}),
    Page::fromFunction("Error","error","Throws the error `str` and ends computation.",{"throw"}),
    Page::fromFunction("Substring","substr","Returns a copy of the ?string? `str` starting from `begin` and continuing for `len` characters. The first character is at index zero, and if a length is not provided, it will copy until the end of the string. Example: `substr(\"Green apple\",2,5) = \"een a\"`.",{"slice"}),
    Page::fromFunction("Lowercase","lowercase","Returns the ?string? `str` with each lowercase letter replaced with it's lowercase equivalent. Example: `lowercase(\"ABed\") = \"abed\"`.",{"uppercase","capital","string","format"}),
    Page::fromFunction("Uppercase","uppercase","Returns the ?string? `str` with each lowercase letter replaced with it's uppercase equivalent. Example: `uppercase(\"ABed\") = \"ABED\"`.",{"lowercase","capital","string","format"}),
    Page::fromFunction("Index of","indexof","Returns the index of `query` within the ?string? `str`. Searching is case-sensitive. The first character is index zero. Example: indexof(\"Red apple\",\"apple\") = 4`.",{"find","query","position","string"}),
    Page::fromFunction("Replace","replace","Returns the ?string? `str` with each instance of `find` replaced with `rep`. Example: `replace(\"heed\",\"e\",\"o\") = \"hood\"`.",{"string","query","find"}),
    #pragma endregion
    #pragma region Conversion
    Page::fromFunction("To Number","tonumber","Returns `val` converted to a 15-digit floating point ?number?.",{"convert"}),
    Page::fromFunction("To Arb","toarb","Returns `val` converted to an ?arbitrary precision? number.",{"convert"}),
    Page::fromFunction("To Vector","tovec","Returns `val` converted to a single element ?vector?.",{"convert"}),
    Page::fromFunction("To Map","tomap","Returns `val` converted to ?map? type.",{"convert"}),
    Page::fromFunction("To String","tostring","Returns `val` converted to ?string? type.",{"convert"}),
    Page::fromFunction("To Lambda","tostring","Returns a constant ?lambda? type that returns `val`.",{"convert"}),
    Page::fromFunction("Typeof","typeof","Returns an integer representing the type of the input `val`. Types are: 1-?number?, 2-?arb?, 3-?vec?, 4-?lambda?, 5-?string?, 6-?map?. If zero is returned, the type of `val` is null.",{"convert"}),
    #pragma endregion
    #pragma endregion
    #pragma region Units
    #pragma region Base units
    Page::fromUnit("m","Meter is a metric base unit representing length",{"length","distance","metre"},"https://en.wikipedia.org/wiki/Metre"),
    Page::fromUnit("kg","Kilogram is a metric base unit representing mass or weight.",{"weight","mass","gram"},"https://en.wikipedia.org/wiki/Kilogram"),
    Page::fromUnit("s","Second is a metric base unit representing time.",{"time","second"},"https://en.wikipedia.org/wiki/Second"),
    Page::fromUnit("A","The Amp is a metric base unit representing electrical current.",{"current","amps"},"https://en.wikipedia.org/wiki/Ampere"),
    Page::fromUnit("mol","The mole is a metric base unit representing amount of substance. It is mainly used in chemistry",{"substance"},"https://en.wikipedia.org/wiki/Mole_unit"),
    Page::fromUnit("K","The kelvin is a metric base unit representing temperature. The size of each degree Kelvin is equivalent to a degree Celsius, however zero Kelvin is set at absolute zero heat. It can be used as an alternative to Celsius.",{"celsius","heat","temperature"},"https://en.wikipedia.org/wiki/Kelvin"),
    Page::fromUnit("$","The dollar is a base unit representing currency. It is independent to any currency system and can be used to represent any currency.",{"currency"}),
    Page::fromUnit("b","The bit is a base unit representing information size.",{"information","byte"},"https://en.wikipedia.org/wiki/Bit"),
    #pragma endregion
    #pragma region Derived units
    Page::fromUnit("N","The Newton is a derived metric unit measuring force.",{"force"},"https://en.wikipedia.org/wiki/Newton_(unit)"),
    Page::fromUnit("J","The Joule is a derived metric unit measuring energy. It is equivalent to `1/3600` ?watt hour? units.",{"energy"},"https://en.wikipedia.org/wiki/Joule"),
    Page::fromUnit("W","The Watt is a derived metric unit measuring energy per second. It is equivalent to one ?Joule? per ?second?.",{"energy"},"https://en.wikipedia.org/wiki/Watt"),
    Page::fromUnit("V","The Volt is a derived metric unit measuring electric potential. It is most commonly represented as one ?watt? per ?Amp?.",{"electric","potential"},"https://en.wikipedia.org/wiki/Volt"),
    Page::fromUnit("Pa","The pascal is a derived metric unit measuring pressure. Pascals are very small, and an ?atmosphere? is over `100000` pascals. It is equivalent to one ?Newton? per square ?meter?.",{"pressure"},"https://en.wikipedia.org/wiki/Pascal_(unit)"),
    Page::fromUnit("bps","Bits per second is a derived metric unit measuring information speed. It is equivalent to one ?bit? per ?second?.",{"information","bytes per second"}),
    Page::fromUnit("Hz","Hertz is a derived metric unit measuring frequency.",{"frequency"},"https://en.wikipedia.org/wiki/Hertz"),
    #pragma endregion
    #pragma region Scaled units
    Page::fromUnit("Wh","The watt hour is the energy required to power one ?watt? for one ?hour?",{"energy"}),
    Page::fromUnit("Ah","The amp hour is the charge required to run one ?amp? of current for one ?hour?",{"charge"}),
    Page::fromUnit("B","The byte is the information in 8 ?bits?.",{"information"}),
    Page::fromUnit("Bps","The bytes per second is the information in 8 ?bits? per second.",{"information"}),
    Page::fromUnit("are","The are is a unit of area equal to `1000` square ?meters?.",{"area"}),
    Page::fromUnit("bar","The bar is a unit of pressure equal to `100000` ?pascal?. It is approximately one ?atmosphere? of pressure.",{"pressure"}),
    Page::fromUnit("min","The minute is a unit of time equal to `60` ?seconds?.",{"time"}),
    Page::fromUnit("hr","The hour is a unit of time equal to `3600` ?seconds?.",{"time"}),
    Page::fromUnit("kph","The kilometers per hour is a unit of velocity for `1000` ?meters? per ?hour?.",{"speed","velocity"}),
    Page::fromUnit("tn","The tonne is a unit of mass equal to `1000` ?kilogram?.",{"mass","weight"}),
    Page::fromUnit("g","The gram is a unit of mass equal to `0.001` ?kilogram?.",{"mass","weight"}),
    #pragma endregion
    #pragma region Universal constants
    Page::fromUnit("c","The speed of light is a constant unit of velocity equal to `299_792_458` ?meters? per ?second?.",{"speed","velocity","constant"},"https://en.wikipedia.org/wiki/Speed_of_light"),
    Page::fromUnit("atm","The atmosphere is a constant unit of perssure equal to the pressure of the Earth's atmosphere at sea level.",{"pressure"},"https://en.wikipedia.org/wiki/Standard_atmosphere_(unit)"),
    Page::fromUnit("eV","The electron volt is a unit of charge equal to the charge of an electron.",{"charge"},"https://en.wikipedia.org/wiki/Electronvolt"),
    Page::fromUnit("mach","The mach is a unit of velocity equal to the speed of sound, approximately 340.3 ?meters? per ?second?.",{"velocity","speed"},"https://en.wikipedia.org/wiki/Mach_number"),
    Page::fromUnit("pc","The parsec is a unit of distance commonly used in atronomical measurements.",{"distance","length"},"https://en.wikipedia.org/wiki/Parsec"),
    #pragma endregion
    #pragma region Metric units
    Page::fromUnit("acre","The acre is a non-metric unit of area.",{"area"},"https://en.wikipedia.org/wiki/Acre"),
    Page::fromUnit("btu","The british thermal unit is a non-metric unit of energy.",{"energy"},"https://en.wikipedia.org/wiki/British_thermal_unit"),
    Page::fromUnit("ct","The carat is a non-metric unit of weight.",{"weight","mass"},"https://en.wikipedia.org/wiki/Carat_(mass)"),
    Page::fromUnit("day","The day is a unit of time.",{"time"}),
    Page::fromUnit("floz","The fluid ounce is a non-metric unit of volume.",{"volume"},"https://en.wikipedia.org/wiki/Fluid_ounce"),
    Page::fromUnit("gallon","The gallon is a non-metric unit of volume.",{"volume"},"https://en.wikipedia.org/wiki/Gallon"),
    Page::fromUnit("in","The inch is a non-metric unit of length.",{"length","distance"},"https://en.wikipedia.org/wiki/Inch"),
    Page::fromUnit("lb","The pound is a non-metric unit of mass or weight.",{"mass","weight"},"https://en.wikipedia.org/wiki/Pound_(mass)"),
    Page::fromUnit("mi","The mile is a non-metric unit of length.",{"length","distance"},"https://en.wikipedia.org/wiki/Mile"),
    Page::fromUnit("mph","The ?mile? per ?hour? is a non-metric unit of speed.",{"speed","velocity"}),
    Page::fromUnit("nmi","The nautical mile is a non-metric unit of distance.",{"length","distance"},"https://en.wikipedia.org/wiki/Nautical_mile"),
    Page::fromUnit("oz","The ounce is a non-metric unit of weight.",{"mass","weight"},"https://en.wikipedia.org/wiki/Ounce"),
    Page::fromUnit("psi","The ?pounds? per square ?inch? is a non-metric unit of pressure.",{"pressure"},"https://en.wikipedia.org/wiki/Pound_per_square_inch"),
    Page::fromUnit("tbsp","The tablespoon is a non-metric unit of volume.",{"volume"},"https://en.wikipedia.org/wiki/Tablespoon"),
    Page::fromUnit("tsp","The teaspoon is a non-metric unit of volume.",{"volume"},"https://en.wikipedia.org/wiki/Teaspoon"),
    Page::fromUnit("yd","The yard is a non-metric unit of length, approximately equal to one ?meter?.",{"length","distance"},"https://en.wikipedia.org/wiki/Yard"),
    #pragma endregion
    #pragma endregion


};
std::map<uint64_t, std::vector<std::pair<int, int>>> Help::queryHash;
void Help::stringToHashList(std::vector<std::pair<uint64_t, int>>& hashOut, const string& str, int basePriority = 1) {
    //X represents the position within a word
    int x = 0;
    uint64_t hash = 0;
    for(int i = 0;i < str.length();i++) {
        //If word separator has been reached
        if(std::strchr(" .,[]()-+`?;:\"", str[i])) {
            if(hash != 0) hashOut.push_back({ hash,basePriority });
            x = 0, hash = 0;
        }
        else {
            uint64_t ch = str[i];
            if(ch >= 'A' && ch <= 'Z') ch += 'a' - 'A';
            if(x < 8) hash |= ch << ((7 - x) * 8);
            x++;
        }
    }
    if(hash != 0) hashOut.push_back({ hash,basePriority });
}
void Help::generateQueryHash() {
    queryHash.clear();
    for(int page = 0;page < pages.size();page++) {
        Page& p = pages[page];
        //Get hashes of words within the page
        std::vector<std::pair<uint64_t, int>> hashes;
        stringToHashList(hashes, p.name, 10);
        stringToHashList(hashes, p.symbol, 7);
        for(auto it = p.aliases.begin();it != p.aliases.end();it++) stringToHashList(hashes, *it, 5);
        stringToHashList(hashes, p.content, 1);
        //Add to queryHash
        for(int i = 0;i < hashes.size();i++)
            queryHash[hashes[i].first].push_back({ page,hashes[i].second });
    }
    //Remove common words
    static const string removedWords = "is the does coefficient are acceptable a b x with to that supports support see returns 1 2 3 4 5 6 7 8 9 0";
    std::vector<std::pair<uint64_t, int>> hashes;
    stringToHashList(hashes, removedWords, 1);
    for(int i = 0;i < hashes.size();i++)
        queryHash[hashes[i].first].clear();
}
std::vector<Page*> Help::search(const string& query, int maxResults) {
    if(queryHash.size() == 0) generateQueryHash();
    //Initialize results vector
    std::vector<std::pair<int, int>> results(pages.size());
    for(int i = 0;i < results.size();i++)
        results[i] = std::make_pair(i, 0);
    //Hash the query
    std::vector<std::pair<uint64_t, int>> hash;
    string newQuery = query;
    for(int i = 0;i < newQuery.length();i++) {
        if(newQuery[i] >= 'A' && newQuery[i] <= 'Z') newQuery[i] = newQuery[i] + 'a' - 'A';
    }
    stringToHashList(hash, query, 1);
    //Add to results the relevance of each page based on the query
    for(int i = 0;i < hash.size();i++) {
        uint64_t hashSize = 0;
        for(int x = 0;x < 8;x++) {
            if(hash[i].first & (uint64_t(0xff) << ((7 - x) * 8))) hashSize |= uint64_t(0xff) << ((7 - x) * 8);
            else break;
        }
        //Upper and lower bound are what each hash starts with
        auto lower = queryHash.lower_bound(hash[i].first);
        auto upper = queryHash.upper_bound(hash[i].first);
        while(upper != queryHash.end() && (upper->first & hashSize) == hash[i].first) upper++;
        //Iterate through upper and lower bound, summing up the priority in the results
        for(;lower != upper;lower++) {
            if((lower->first & hashSize) == hash[i].first)
                for(int x = 0;x < lower->second.size();x++)
                    results[lower->second[x].first].second += lower->second[x].second;
        }
    }
    //Sort by priority
    std::sort(results.begin(), results.end(), [](std::pair<int, int> one, std::pair<int, int> two) {return one.second > two.second;});
    //Return output as a list of page pointers
    std::vector<Page*> out;
    for(int i = 0;i < std::min(size_t(maxResults), results.size());i++) {
        if(results[i].second == 0) break;
        out.push_back(&pages[results[i].first]);
    }
    return out;
}