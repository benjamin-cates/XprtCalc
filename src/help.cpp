#include "_header.hpp"
using namespace Help;
#pragma region Exporter functions
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
string Page::contentToHTML(string content) {
    string out;
    for(int i = 0;i < content.length();i++) {
        if(content[i] == '?') {
            int end = i + 1;
            while(end != content.length() && content[end] != '?') end++;
            string link = content.substr(i + 1, end - i - 1);
            out += "<ln onclick='openHelp(this)'>" + link + "</ln>";
            i = end;
        }
        else if(content[i] == '`') {
            int end = i + 1;
            while(end != content.length() && content[end] != '`') end++;
            if(end == -1) throw "Cannot find end of `";
            out += "<mono>";
            //Get colored string
            string cont = content.substr(i + 1, end - i - 1);
            ColoredString str;
            if(cont[0] == '/') str = ColoredString(cont, Expression::colorLine(cont, Program::parseCtx));
            else str = ColoredString::fromXpr(cont);
            //Replace error with argument type
            string col = str.getColor();
            for(int i = 0;i < col.length();i++)
                if(col[i] == Expression::hl_error) col[i] = Expression::hl_argument;
            str.setColor(std::move(col));
            out += str.toHTML();
            out += "</mono>";
            i = end;
        }
        else if(content[i] == '"') out += "&quot;";
        else if(content[i] == '&') out += "&amp;";
        else if(content[i] == '<') out += "&lt;";
        else if(content[i] == '>') out += "&gt;";
        else if(content[i] == '\n') out += "<br>\n";
        else out += content[i];
    }
    return out;
}
string Page::toHTML() {
    string out = "<hTitle>" + name + "</hTitle>";
    if(symbol.length() != 0)
        out += "<hSym>" + symbol + "</hSym>";
    if(type.length() != 0)
        out += "<hType>" + type + "</hType>";
    if(aliases.size() != 0) {
        out += "<hTags>";
        for(int i = 0;i < aliases.size();i++) out += (i == 0 ? "" : ", ") + aliases[i];
        out += "</hTags>";
    }
    out += "\n<hCont>";
    out += Page::contentToHTML(content);
    out += "</hCont>\n";
    if(examples.size() != 0) {
        out += "<hExList>";
        for(int i = 0;i < examples.size();i++)
            out += "<hEx>" + Page::contentToHTML("`" + examples[i] + "`") + "</hEx>";
        out += "</hExList>\n";
    }
    if(seeMore.length() != 0) {
        string humanReadable = seeMore;
        if(humanReadable.substr(0, 8) == "https://") humanReadable = humanReadable.substr(8);
        if(humanReadable.find("/") != string::npos) humanReadable = humanReadable.substr(0, humanReadable.find("/"));
        out += "<hSeeMore>See more: <a href='" + seeMore + "' target='_blank'>" + humanReadable + "</a></hSeeMore>";
    }
    return out;
}
ColoredString Page::contentToColoredString(string content) {
    ColoredString out;
    int lastSection = 0;
    for(int i = 0;i < content.length();i++) {
        //Colored part
        if(content[i] == '`') {
            //Add string before colored
            out += content.substr(lastSection, i - lastSection);
            //Add colored part
            int end = Expression::findNext(content, i + 1, '`');
            ColoredString str = ColoredString::fromXpr(content.substr(i + 1, end - i - 1));
            //Replace unfound variable errors with an argument
            string col = str.getColor();
            for(int i = 0;i < col.length();i++)
                if(col[i] == Expression::hl_error) col[i] = Expression::hl_argument;
            str.setColor(std::move(col));
            out += str;
            //Iterate to end
            lastSection = end + 1;
            i = end;
        }
        //Help page link (ignore because links are not supported in ColoredString)
        else if(content[i] == '?') {
            out += content.substr(lastSection, i - lastSection);
            lastSection = i + 1;
        }
    }
    return out;
}
ColoredString Page::toColoredString() {
    ColoredString out;
    out += Page::contentToColoredString(content);
    out += "\n";
    if(examples.size() != 0) {
        out += "Examples: \n";
        for(int i = 0;i < examples.size();i++) {
            out += Page::contentToColoredString("`" + examples[i] + "`");
            out += "\n";
        }
    }
    if(seeMore.length() != 0) out += "\nSee more: " + seeMore + "\n";
    return out;
}
#pragma endregion
#pragma region Page generator functions
void Help::addPageData() {
    for(int i = 0;i < pages.size();i++) {
        if(pages[i].type == "function") pages[i].addFunctionData();
        if(pages[i].type == "unit") pages[i].addUnitData();
        if(pages[i].type == "type") pages[i].addTypeData();
        if(pages[i].type == "library") pages[i].addLibraryData();
        if(pages[i].type == "list") pages[i].addListData();
    }
}
void Page::addUnitData() {
    Page out;
    auto it = Unit::listOfUnits.find(symbol);
    name = std::get<3>(it->second);
    symbol = "[" + symbol + "]";
    content += " " + name + " has base units: `[" + std::get<0>(it->second).toString() + "]` with coefficient `" + std::to_string(std::get<1>(it->second)) + "`.";
    if(std::get<2>(it->second))
        content += " " + name + " supports ?metric prefixes?.";
    else content += " " + name + " does not support ?metric prefixes?.";
}
void Page::addFunctionData() {
    Function& func = Program::globalFunctions[Program::globalFunctionMap[symbol]];
    //Add domain to content
    std::vector<Function::Domain> dom = func.getDomain();
    if(dom.size() == 1 && dom[0].sig == 0) content += " `" + symbol + "` takes no arguments.";
    else {
        content += " Acceptable inputs are: ";
        for(int i = 0;i < dom.size();i++) {
            if(i != 0) content += ", ";
            content += dom[i].toString();
        }
        content += ". See ?types? for more info.";
    }
    //Append arguments to symbol
    std::vector<string>& inputs = func.getInputNames();
    for(int i = 0;i < inputs.size();i++)
        symbol += (i == 0 ? "(" : ",") + inputs[i];
    if(inputs.size() != 0) symbol += ")";
}
void Page::addLibraryData() {
    Library::LibFunc& func = Library::functions[name];
    name = func.fullName;
    symbol = func.name;
    content += " `" + name + "` compiles from: `" + func.xpr + "`.";
    if(func.dependencies.size() != 0) {
        content += "The function is will compile";
        if(func.dependencies.size() == 1) content += " ?" + func.dependencies[0] + "? ";
        else {
            for(int i = 0;i < func.dependencies.size() - 1;i++) content += string(i == 0 ? ", " : " ") + "?" + func.dependencies[i] + "?";
            content += ", and ?" + func.dependencies.back() + "? ";
        }
        content += " before it is included.";
    }
}
void Page::addTypeData() {
    string type = content.substr(0, 1);
    content = content.substr(1);
    content += " The ?typeof? function returns `" + type + "` for this type.";
}
void Page::addListData() {
    if(name == "List of functions") {
        string prevType = "";
        for(int i = 0;i < Help::pages.size();i++) {
            if(Help::pages[i].type == "function") {
                //Add function category header if needed
                if(prevType != Help::pages[i].aliases[0]) {
                    prevType = Help::pages[i].aliases[0];
                    content += "`" + prevType + "`\n";
                }
                content += "- `" + Help::pages[i].symbol + "` - ?" + Help::pages[i].name + "?\n";
            }
        }
    }
    else if(name == "List of units") {
        for(int i = 0;i < Help::pages.size();i++) {
            if(Help::pages[i].type == "unit") {
                content += "- `" + Help::pages[i].symbol + "` ?" + Help::pages[i].name + "? (" + Help::pages[i].aliases[0] + ")\n";
            }
        }
    }
    else if(name == "List of pages") {
        string prevType;
        for(int i = 0;i < Help::pages.size();i++) {
            if(prevType != Help::pages[i].type) {
                content += "`" + Help::pages[i].type + "`\n";
                prevType = Help::pages[i].type;
            }
            content += "- `";
            if(Help::pages[i].symbol != "")
                content += Help::pages[i].symbol;
            content += "` ?";
            content += Help::pages[i].name;
            content += "?\n";
        }
    }
    else if(name == "List of commands") {
        for(int i = 0;i < Help::pages.size();i++) {
            if(Help::pages[i].type == "command") {
                content += "- `";
                content += Help::pages[i].symbol;
                content += "` - ?";
                content += Help::pages[i].name;
                content += "?\n";
            }
        }
    }
    else if(name == "List of includes") {
        content += "Each of these functions can be included by running `/include {symbol}` on them. You can also batch include categories by running include on the category names shown in this document.\n";
        std::map<string, std::vector<string>> categoryMap;
        for(const std::pair<string, Library::LibFunc>& x : Library::functions) {
            int colon1 = x.second.fullName.find(':');
            int colon2 = x.second.fullName.find(':', colon1 + 1);
            const string& symbol = x.first + x.second.inputs;
            const string& name = x.second.fullName.substr(0, colon1);
            const string& category = x.second.fullName.substr(colon1 + 1, colon2 - colon1);
            const string& description = x.second.fullName.substr(colon2 + 1);
            if(categoryMap.find(category) == categoryMap.end()) categoryMap.insert(std::pair<string, std::vector<string>>{category, {}});
            categoryMap[category].push_back("`" + symbol + "` - " + name + ". " + description + "\nCompiles to: `" + x.first + "=" + x.second.inputs + "=>" + x.second.xpr + "`");
        }
        //Print out sorted by category
        for(const std::pair<string, std::vector<string>>& x : categoryMap) {
            content += "\n*** " + x.first + "\n\n";
            for(int i = 0;i < x.second.size();i++)
                content += x.second[i] + "\n\n";
        }
    }
}
#pragma endregion
std::vector<Page> Help::pages = {
    #pragma region Introduction
    //Info
    Page("Welcome","","guide","Welcome to XprtCalc, use the \"/query\" command to search the help pages, or type \"/help introduction\" to read about the basic functionality, info can be found in the page called ?info?.",{"help"}),
    #pragma region Guides
    Page("List of operators","","guide","Operators are small character sequences that imply functions like add, subtract, etc. They can be binary (take two arguments) or unary (apply to one number). Binary operators are used like `3*4`, which is casted by the program into `mul(3,4)`, for example. The binary operators are: \n`+ - add` (Addition) \n`- - sub` (Subtraction) \n`* - mul` (Multiplication) \n`/`` - div` (Division) \n`^ - pow` (Exponentiation) \n`** - pow` (Exponentiation) \n`= - equal` \n`== - equal` \n`!= - not_equal` \n`> - gt` (Greater than) \n`>= - gt_equal` (Greater than or equal) \n`< - lt` (Less than) \n`<= - lt_equal` (Less than or equal) \nBinary operators have an order of precedence, which is exponents first, then multiplication and division, then addition and subtraction, then the comparison operators. Example: `4*5+9` becomes `add(mul(4,5),9)` because multiplication has higher precedence. The prefix unary operators (goes before expression) are:\n`- - neg` (Negative)\nThe suffix unary operators (goes after expression) are: \n`! - factorial`.",{},{"4*5!+2 = add(mul(4,factorial(5)),2)"}),
    Page("Units","","guide","Units store types of units of measurement. They must be referenced inside the `[]` square brackets (do not confuse them with the square brackets in ?accessor notation?). An example of a unit is meters, which stores length. Six metric base units are supported, and the `dollar` and `bit` are additionally added. They are stored as a list of base units with their powers. For example area is stored as `[m^2]`. All units are stored in terms of metric base units, and some have support for ?metric prefixes?. Units interact with certain operators, like with multiplication, their powers are added, with division the powers are subtracted, etc. However, with most functions, the units are left untouched. The base units are: `m` for Meter, `kg` for kilogram, `s` for second, `A` for Amp, `K` for Kelvin, `mol` for mole, `b` for bits, and `$` for dollars. If the power ever goes above `127` or below `-127`, an overflow error will occur, and the powers can only be integers. See the ?list of units? in the help pages.",{"measurement"},{"[m]/[s] = [m/s]","[J/s] = [W]"},"https://www.mathsisfun.com/measure/unit.html"),
    Page("Metric prefix","","guide","Metric prefixes are single character ?unit? modifiers that represent a change in magnitude. For example `[km]` translates to `1000[m]`, the k means 1000, and the m means meters. Prefixes are supported on most metric units, but check each unit's help page to be sure.\nList of prefixes: \n n - nano `(10^-9)` \n u - micro `(10^-6)` \nm - milli `(0.001)`\nc - centi `(0.01)`\nk - kilo `(1000)`\nM - Mega `(1_000_000)`\nG - Giga `(10^9)`\nT - Tera `(10^12)`\nOther less common prefixes: \ny - yocto `(10^-24)`\nz - zempto `(10^-21)`\na - atto `(10^-18)`\nf - fempto `(10^-15)`\np - pico `(10^-12)`\nP - Peta `(10^15)`\nZ - Zetta `(10^18)`\nY - Yotta `(10^21)`\n",{},{"[km] = 1000[m]","[cm] = 0.01[m]","[g] = 0.001[kg]"},"https://www.nist.gov/pml/owm/metric-si-prefixes"),
    Page("Base conversion","","guide","There are multiple ways that different bases can be parsed, however there is currently no way to print in a different base. The first is a prefix signal. Prefix signals start with a zero (so it is not parsed as a variable), followed by a character, then the number. The supported prefixes are: `0b` binary (2), `0t` ternary (3), `0o` octal (8), `0d` decimal (10), and `0x` hexadecimal (16). Note that the exponent character is also parsed in that base, so `0b11e101` is `3 * 2^5`. The other way to parse bases is using the square bracket underscore notation. This looks something like `[0A1]_16`. The base is the number after the underscore, which can be computed, but it must be constant. Note that, like in this example, all numbers must start with a number from 0-9, so hexadecimals must start with `0`. The maximum base is `36` and uses the characters `0-9` and then `A-Z` for `10-36` (characters must be uppercase). The minimum base is base 2. ",{},{"ob101 = 5","0b11e101 = 3 * w^5","0xA1 = 0xA*16 + 0x1 = 161","0xA1 = [0A1]_16","[15]_15 = 20"},"https://en.wikipedia.org/wiki/Positional_notation#Base_of_the_numeral_system"),
    Page("Accessor notation","","guide","Accessor notation is a quick alias to the function `get`. It is written as square brackets following a parenthesis block, ?string?, ?vector?, ?map? or function call. For vectors, it returns the value at that index, with the first value having index `0`. For a map, it returns the value that is paired with that key. For a string, it returns the character code for the character at that index, with the first character having index `0`. If you would instead like to use square brackets for naming ?units?, precede it with an operator (usually ?multiplication?) so it is not treated as an accessor.",{"get","index"},{"<1,2,3>[0] = 1","<1,2,3>[1] = 2","{4:3,\"4\":2}[4] = 3","\"Hello World\"[1] = \"e\""}),
    Page("Assignment operator","","guide","The assignment operator allows the user to assign and change the values of ?variables?. It is similar to a command in that it must be the start of a line (cannot be within parenthesis). Any expression can be placed after the equals sign. It also allows ?accessor notation? in assignment to allow the changing of a single memeber in a ?vector?, ?map?, or ?string?, accessors can be nested in this form. The final available feature of the assignment operator is function syntax, which is a syntactic sugar for ?lambdas? in variables.. ",{"variables"},{"b[0][1]=2","a=10","f(x)=x^2"}),
    Page("Command","","guide","Commands are functions that can access special parts of the program. Commands are prefixed by the '/' character. Commands interact with top-level features that can't, or wouldn't make sense to, be in a function. A ?list of commands? is available in the help pages."),
    Page("History","","guide","The history is a record of all previous calculations. It can be accessed in several ways: first, the `ans` variable will always return the previous answer when presented with no arguments. Second, the `ans` function will return that element in the history, e.g. `ans(5)` will return the fifth element, and `ans(-2)` will return the one that is two before the previous calculation. `ans` and `ans(-1)` are equivalent (the previous element). Finally, you can also use the `$1` number that is presented whenever the answer was given.",{"answer","memory","previous"},{"ans = ans(-1)","$3 = ans(3)"}),
    Page("Variables","","guide","Variables are global objects that store a value under a string. They can be created with the ?assignment operator?, and can be accessed with their name. Example: `a=10` will set the variable a to `10`, whenever a is referenced from now on, it will evaluate to `10`. Variables can also be used as custom functions, for example `g(a,b)=a*b` will create a function `g` that returns the first argument multiplied by the second argument. This is internally stored as a lambda function, which is essentially an expression with arguments encapsulated in a value.",{"values","custom"}),
    //Comments
    //Preferences
    #pragma endregion
    #pragma endregion
    #pragma region Types
    Page("Types","","guide","Each value in the program can be stored in one of 6 different types: \nnum - ?number? (a complex float)\narb - ?arb? (an arbitrary precision float)\nvec - ?vector? (stores a list of values)\nlmb - ?lambda? (encapsulates expressions into mini functions)\nstr - ?string? (stores text)\nmap - ?map? (stores a list of key-value pairs like a Python dictionary)\nThese types allow for many different features and are widely supported in many of the different functions."),
    Page("Number","num","type","0The number class has three components: a real decimal, and ?imaginary? decimal, and a ?unit?. These can be treated as a single package. Each of the two decimals is stored to `15` digits of precision. More precision can be stored in the ?arb? type.",{"floating","double"},{"2","-8i","2.5+4i*[m]","2[kg]","inf"},"https://en.wikipedia.org/wiki/Floating-point_arithmetic"),
    Page("Arb","arb","type","1The arb type is very similar to the ?number? type, except that the decimals are stored with arbitrary precision, and it does not have imaginary numbers. To cast from arb to a number, use the ?tonumber? function. To cast from number to arb, use the ?toarb? function, and precision can be specified in the second argument. They can also be created with the 'p' symbol in a decimal literal. If the number of digits is more than 15 it will also store it with extra precision as in the previous example. When two arb types (or an arb and a number) are put together in a binary operation, the return value will have the highest precision of both. The 'p' symbol must come after the 'e' symbol for exponents. On the terminal interface version, arb may not be supported if it is compiled without it.",{"p","precision","arbitrary","float"},{"1.5p20 * 2p5 = 3.0p20","1.5p20 = 1.5000000000000000000","1.5e2p50 = 150p50"},"https://github.com/bluescarni/mppp"),
    Page("Vector","vec","type","2The vector type stores an arbitrarily long list of values. The elements can be of any type. They are created with the angle brackets `< and >` as is commonly used in linear algebra. There are many different built in functions that handle vectors, and the complete list can be seen in the ?list of functions? help page. Some common ones are ?get?, ?fill?, ?map_vector?, ?length?, ?magnitude?, ?normalize?, and ?sort?. The most common function is `get`, which is also aliased by ?accessor notation?. Vectors can be used to store Euclidian point positions, return multiple values from a function, or store a list. Vectors apply to many common functions like `sqrt`, `exp`, and every other elementary operation. See the pages for each function to see if they apply to vectors.",{"list","array","iterator","index"},{"<1,2,3>","<1.5p20,x=>x+1,<12,3>,{1:2}>","2+<4,8> = <6,10>","sqrt(<4,9>) = <2,3>","sin(<pi,pi/2>) = <0,1>","<10,4,2>[1] = 4","fill(x=>x,5) = <0,1,2,3,4>","length(<5,3,9>) = 3"}),
    Page("Lambda","lmb","type","3The lambda type stores a nameless function that can have any number of arguments, and a single output. They use arrow notation similar to JavaScript. Example: `x=>x+1` should be read as \"x goes to x plus one\". For zero argument lambdas, use an underscore character `_=>rand`, for more than one, use a comma separated list enclosed in parenthesis `(x,t)=>x*t`. As long as they follow the ?variable? naming conventions. Lambdas can also be nested and are dynamic types. There are many different functions that take advantage of lambdas, common ones are: ?run?, ?apply?, ?sum?, ?product?, ?fill?, ?map?, and ?sort?. Variables that store anonymous functions can be treated as regular functions with that name like `a(x)=x^2`.",{"anonymous function","=>"},{"run(x=>y=>x+y,2) = y=>2+y","simplify(x=>x*x^2) = x=>x^3","sum(x=>x+1,0,4) = 1+2+3+4+5 = 15","apply(<16,9>,x=>sqrt(x)) = <4,3>"}),
    Page("String","str","type","4The string type stores text (as a list of characters). Strings can be used to print information, create dynamic evaluations, or throw errors. They are enclosed by the double quotes \" (no other wrapping is available). There are multiple different string methods, like: ?eval?, ?substr?, ?lowercase?, ?uppercase?, ?error?, ?print?, ?indexof?, and ?replace?. In order to prevent character conflict, double quotes within strings must be preceded by the \\ backslash character, backslashes must be escaped by another backslash. Other escaped characters are: `\"\\n\"` for a new line, and `\"\\t\"` for the tab character.",{"character","text","words"},{"\"Hello\"","\"\\\\\" -> \\","\"\\n\" ->` new line `","\"\\t\" ->` tab character`","uppercase(\"foo bar\") = \"FOO BAR\""}),
    Page("Map","map","type","5The map type stores key-value mappings, it is represented using the `{}` curly bracket syntax. Each pair is separeted by commas and the elements in the pair are separeted by the colon. Example: `{1:4,\"yes\":8}` means `1` maps to `4`, and `\"yes\"` maps to 8. Any value (even ?lambdas?) can be either a key or value. Maps are not used much in builtin functions, but they are extremely useful in storing data in a logical way, or to create an object oriented design. The map is internally stored as a binary search tree, however the different value types can obviously not be compared, so the ordering is more approximate. Functions that support the map are: ?get?, ?concat?, and ?length?. ?get? is aliased by ?accessor notation? similar to JavaScript.",{"relation","dictionary","object","key"},{"{1:3,\"yes\":8}[\"yes\"] = 8","{\"size\":\"large\",\"name\":\"Peacock\",\"height\":1.2[m]}","length({1:2,3:-1}) = 2","concat({1:2},{3:-1}) = {1:2,3:-1}"}),
    #pragma endregion
    #pragma region Functions
    #pragma region Elementary
    Page("Negate","neg","function","Returns the negative of `x`. It is also aliased by the prefix operator '`-`'.",{"elementary","-"},{"-4","-sqrt(2)","4*-3 = -12"}),
    Page("Addition","add","function","Returns `a + b`. If both are strings, they will be concatenated. It is also aliased by the  operator '`+`'.",{"elementary","+","sum","plus"}),
    Page("Subtraction","sub","function","Returns `a - b`. It is also aliased by the operator '`-`'.",{"elementary","-","difference","minus"}),
    Page("Multiply","mul","function","Returns `a * b`. It is also aliased by the operator '`*`'.",{"elementary","*","product","times"}),
    Page("Divide","div","function","Returns `a / b`. It is also aliased by the operator '`/`'.",{"elementary","/","over","quotient"}),
    Page("Power","pow","function","Returns `a ^ b`. It is also aliased by the operators '`^`' and '`**`'.",{"elementary","**","^","exponent","exponentiation"},{"3^2 = 3*3 = 9","sqrt(16^2) = 16"},"https://en.wikipedia.org/wiki/Exponentiation"),
    Page("Modulo","mod","function","Returns `a % b`, which is the remainder when `a` is divided by `b`. It is also aliased by the operator '`%`'.",{"elementary","%","modulus"},{"20 % 5 = 0","20 % 4 = 0","20 % 3 = 2","<1,2,3,4,5,6,7> % 3 = <1,2,0,1,2,0,1>","6 % 2.5 = 1"},"https://en.wikipedia.org/wiki/Modulo_operation"),
    Page("Square Root","sqrt","function","Returns the square root of `x`, which is equivalent to raising it to the power of `0.5`.",{"elementary","power","exponent","root"},{"sqrt(16) = 4","sqrt(x^2) = x","sqrt(-1) = i"},"https://en.wikipedia.org/wiki/Square_root"),
    Page("Exponent","exp","function","Returns the `e ^ x`, which means ?Euler's number? raised to the power of `x`. the `exp` function is special in mathematics because it acts as a starting base for exponential growth, and has the special property that it is the derivative of itself. The inverse function of `exp` is `ln`.",{"elementary","e","e^x","power","exponent",},{"exp(0) = e^0 = 1","exp(-1) = 1/e","exp(4) = e^4","exp(ln(x)) = x"},"https://en.wikipedia.org/wiki/Exponential_function"),
    Page("Natural log","ln","function","Returns the natural logarithm of `x`, it is the inverse of `exp`. For an arbitrary base, see the `logb` function.",{"elementary","logarithm"},{"ln(e^3) = 3","ln(exp(x)) = x"},"https://en.wikipedia.org/wiki/Natural_logarithm"),
    Page("Logarithm","log","function","Returns the logarithm base 10 of `x`, which means `10^log(x)` is `x`. For an arbitrary base, see the `logb` function.",{"elementary","log"},{"log(1000) = log(10^3) = 3"},"https://en.wikipedia.org/wiki/Logarithm"),
    Page("Logarithm base b","logb","function","Returns the logarithm base `b` of `x`, which means `b^logb(x,b)` is `x`. For specialized bases, see the `ln` (base `e`) and `log` (base `10`) functions.",{"elementary","log"},{"logb(5^3,5) = 3","logb(x,b) = ln(x)/ln(b)"},"https://en.wikipedia.org/wiki/Logarithm"),
    Page("Gamma","gamma","function","Returns the gamma function of `x`. It currently does not support imaginary numbers. Equivalent to `factorial(x+1)`.",{"elementary"},{},"https://en.wikipedia.org/wiki/Gamma_function"),
    Page("Factorial","factorial","function","Returns the factorial of `x`, which is equavialent to all of the positive integers smaller than x multiplied together. However, factorial accepts non-integer inputs, but currently does not support ?imaginary? numbers. Factorial is also aliased by the `!` operator. Factorial is equivalent to `gamma(x-1)`.",{"elementary"},{"5! = 5*4*3*2*1 = 120","4!+2 = factorial(4)+2 = 24+2 = 26"},"https://en.wikipedia.org/wiki/Gamma_function"),
    Page("Error function","erf","function","Returns the error function of `x`. It currently does not support imaginary numbers. It does not have much use in simple mathematics, however support is there for who needs it.",{"elementary"},{},"https://en.wikipedia.org/wiki/Error_function"),
    #pragma endregion
    #pragma region Trigonometry
    Page("Sine","sin","function","Returns the sine of `x`, which is the opposite side divided by hypotenuse of a right triangle with angle `x`",{"trigonometry"},{},"https://en.wikipedia.org/wiki/Sine_and_cosine"),
    Page("Cosine","cos","function","Returns the cosine of `x`, which is the adjacent side divided by hypotenuse of a right triangle with angle `x`",{"trigonometry"},{},"https://en.wikipedia.org/wiki/Sine_and_cosine"),
    Page("Tangent","tan","function","Returns the tangent of `x`. Equivalent to `sin(x)/cos(x)` or the opposite side divided by the adjacent side of a right triangle with angle `x`.",{"trigonometry"},{},"https://en.wikipedia.org/wiki/Trigonometric_functions#Right-angled_triangle_definitions"),
    Page("Cosecant","csc","function","Returns the cosecant of `x`. Equivalent to `1/sin(x)` or the hypotenuse divided by the opposite side of a right triangle with angle `x`.",{"trigonometry"},{},"https://en.wikipedia.org/wiki/Trigonometric_functions#Right-angled_triangle_definitions"),
    Page("Secant","sec","function","Returns the secant of `x`. Equivalent to `1/cos(x)` or the hypotenuse divided by the adjacent side of a right triangle with angle `x`.",{"trigonometry"},{},"https://en.wikipedia.org/wiki/Trigonometric_functions#Right-angled_triangle_definitions"),
    Page("Cotangent","cot","function","Returns the cotangent of `x`. Equivalent to `1/tan(x)`, `cos(x)/sin(x)` or the adjacent divided by the opposite side of a right triangle with angle `x`.",{"trigonometry"},{},"https://en.wikipedia.org/wiki/Trigonometric_functions#Right-angled_triangle_definitions"),
    Page("Hyperbolic sine","sinh","function","Returns the hyperbolic sine of `x`.",{"trigonometry"},{},"https://en.wikipedia.org/wiki/Hyperbolic_functions"),
    Page("Hyperbolic cosine","cosh","function","Returns the hyperbolic cosine of `x`.",{"trigonometry"},{},"https://en.wikipedia.org/wiki/Hyperbolic_functions"),
    Page("Hyperbolic tangent","tanh","function","Returns the hyperbolic tangent of `x`.",{"trigonometry"},{},"https://en.wikipedia.org/wiki/Hyperbolic_functions"),
    Page("Inverse sine","asin","function","Returns the inverse ?sine? of `x`. `asin(sin(x)) = x` for `-pi/2 < x < pi/2`",{"trigonometry","arcsine"},{},"https://en.wikipedia.org/wiki/Inverse_trigonometric_functions"),
    Page("Inverse cosine","acos","function","Returns the inverse ?cosine? of `x`. `acos(cos(x)) = x` for `0 < x < pi`",{"trigonometry","arccosine"},{},"https://en.wikipedia.org/wiki/Inverse_trigonometric_functions"),
    Page("Inverse tangent","atan","function","Returns the inverse ?tangent? of `x`. `atan(tan(x)) = x` for `-pi/2 < x < pi/2`",{"trigonometry","arctan"},{},"https://en.wikipedia.org/wiki/Inverse_trigonometric_functions"),
    Page("Arctangent 2","atan2","function","Returns the angle between the point `(y,x)` and `(0,1)`. It is similar to the `arg` function.",{"trigonometry","inverse tangent"},{"atan2(1,0) = pi/2","atan2(i) = pi/2"},"https://en.wikipedia.org/wiki/Atan2"),
    Page("Inverse hyperbolic sine","asinh","function","Returns the inverse ?hyperbolic sine? of `x`. `asinh(sinh(x)) = x`.",{"trigonometry","hyperbolic arcsine"},{},"https://en.wikipedia.org/wiki/Inverse_hyperbolic_functions"),
    Page("Inverse hyperbolic cosine","acosh","function","Returns the inverse ?hyperbolic cosine? of `x`. `acosh(cosh(x)) = x` for `x >= 1`.",{"trigonometry","hyperbolic arccosine"},{},"https://en.wikipedia.org/wiki/Inverse_hyperbolic_functions"),
    Page("Inverse hyperbolic tangent","atanh","function","Returns the inverse ?hyperbolic tangent? of `x`. `atanh(tanh(x)) = x`.",{"trigonometry","hyperbolic arctangent"},{},"https://en.wikipedia.org/wiki/Inverse_hyperbolic_functions"),
    #pragma endregion
    #pragma region Numeric
    Page("Round","round","function","Returns the nearest integer to `x`. If the decimal component of `x >= 0.5`, it will round up, else it will round down. Round also applies to ?imaginary? numbers in a euclidian way.",{"numeric"},{"round(4.3) = 4","round(4.7) = 5","round(4.5) = 5","round(-1.5) = -2"},"https://en.wikipedia.org/wiki/Rounding#Round_half_up"),
    Page("Floor","floor","function","Returns the nearest integer that is less than `x`. Floor also applies to ?imaginary? numbers in a euclidian way.",{"numeric","round"},{"floor(4.3) = 4","floor(4.99) = 4","floor(-1.5) = -2"},"https://en.wikipedia.org/wiki/Floor_and_ceiling_functions"),
    Page("Ceiling","ceil","function","Returns the nearest integer that is greater than `x`. Ceiling also applies to ?imaginary? numbers in a euclidian way.",{"numeric","round"},{"ceil(4.3) = 4","ceil(4.01) = 5","ceil(4) = 4","ceil(-1.5) = -1"},"https://en.wikipedia.org/wiki/Floor_and_ceiling_functions"),
    Page("Real component","getr","function","Returns the real component of an complex number, ignoring the ?imaginary? part. Use `geti` to get the imaginary component.",{"numeric","complex"},{"getr(2+i) = 2","getr(3i) = 0"}),
    Page("Imaginary component","geti","function","Returns the ?imaginary? component of an imaginary number. Use `getr` to get the real component.",{"numeric","complex"},{"geti(2+i) = 1","geti(40) = 0","geti(4i) = 4"}),
    Page("Unit component","getu","function","Returns the unit component of a number multiplied by 1.",{"numeric"},{"getu(2[m*kg]) = [m*kg]","getu(0[m]) = [m]"}),
    Page("Maximum","max","function","Returns the largest of `a` and `b`. The alternative input is if the first argument is a ?vector?, it will return the largest element (largest is determined only by the real component if it has an ?imaginary? component)",{"numeric"},{"max(10,4) = 10","max(40i,1) = 1","max(<40,1,100,0.5>) = 100"}),
    Page("Minimum","min","function","Returns the smallest of `a` and `b`. The alternative input is if the first argument is a ?vector?, it will return the smallest element (smallest is determined only by the real component if it has an ?imaginary? component)",{"numeric"},{"min(10,4) = 4","min(i,40) = i","min(<40,1,100,0.5>) = 0.5"}),
    Page("Linear interpolation","lerp","function","Returns the linear interpolation from `a` to `b` with time `x`. It effectively returns `a + (b-a)*x`.",{"numeric","linear interpolate","linear extrapolat"},{"lerp(a,b,0) = a","lerp(a,b,1) = b","lerp(a,b,0.5) = (a+b)/2"},"https://en.cppreference.com/w/cpp/numeric/lerp"),
    Page("Distance","dist","function","Returns the distance from `a` to `b`. If both are real numbers, it is equivalent to `abs(a-b)`. For an ?imaginary? number or a ?vector? the Pythagorean theorem is used and they are treated as euclidian points.",{"numeric","hypotenuse"},{"dist(-1,2) = 3","dist(1,i) = sqrt(2)"}),
    Page("Sign","sgn","function","Returns the sign of the value `x`. If `x` is positive, it returns `1`. If it is negative, it returns `-1`, and finally if `x` is zero, it returns `0`. For ?imaginary? numbers, it returns the unit vector of the two scalars. `sgn` is equivalent to `x/abs(x)` with a special case for zero.",{"numeric","signum"},{"sgn(-20) = -1","sgn(40) = 1","sgn(0) = 0","sgn(1+i) = 0.707+0.707*i","sgn(x) = x/abs(x)"},"https://en.wikipedia.org/wiki/Sign_function"),
    Page("Absolute value","abs","function","Returns the ?distance? of `x` from zero. For ?imaginary? numbers it returns `sqrt(r^2 + i^2)` from the pythagorean theorem. Usually in mathematics it is denoted as |x|, however this is not supported in XprtCalc.",{"numeric","distance","|","|x|"},{"abs(10) = 10","abs(-10) = 10","abs(1+i) = sqrt(2)","abs(-1+i) = sqrt(2)"},"https://en.wikipedia.org/wiki/Absolute_value"),
    Page("Argument","arg","function","Returns the angle between 1+0i and the complex number `z`. The function's return range is from `-pi` to `pi`.",{"numeric","complex argument","angle between"},{"arg(z) = atan2(geti(z),getr(z))","arg(1+i) = pi/4","arg(1-i) = -pi/4"},"https://en.wikipedia.org/wiki/Argument_(complex_analysis)"),
    Page("Greatest common divisor","gcd","function","Returns the greatest common divisor of `a` and `b` (not to be confused with ?least common multiple?). The greatest common divisor is the largest integer that both numbers can be divided evenly by. If a fractional value is passed in, it will round down to the nearest integer. The ?arb? type is not supported, however it will be cast up to a 15 digit integer and can be cast back to arb.",{"numeric"},{"gcd(5,15) = 5` because `5/5` and `5/15` are integers`","gcd(x,1) = 1","gcd(135,213) = 3"}),
    Page("Least common multiple","lcm","function","Returns the least common multiple of `a` and `b` (not to be confused with ?greatest common divisor?). The least common multiple is the smallest number that both numbers divide evenly into. If a fractional value is passed in, it will round down to the nearest integer. The ?arb? type is not supported, however it will be cast to a number and can be cast back to arb.",{"numeric"},{"lcm(8,12) = 24` because `24/8` and `24/12` are integers`","lcm(15,10) = 30"}),
    Page("Prime factors","factors","function","Returns a list of prime factors of `x`. This can be used to test for primality because it will return a ?vector? of ?length? one if `x` is prime.",{"numeric"},{"factors(60) = <2,2,3,5>` because `2*2*3*5 = 60","factors(32) = <2,2,2,2,2>","factors(15) = 3*5"}),
    #pragma endregion
    #pragma region Binary Functions
    Page("Equal","equal","function","Returns `1` if `a` and `b` are equal, else returns `0`. It is aliased by the `==` and `=` operators. Beware that if the values are of different ?types?, it will return zero always.",{"comparison","equivalent","=="},{"5=5 = 1","5=5p20 = 0","5=2 = 0"}),
    Page("Not equal","not_equal","function","Returns `1` if `a` and `b` are not equal, else returns `0`. It is aliased by the `!=` operator. Be careful that if the values are of different ?types?, it will return `1` always.",{"comparison","equivalent","!="},{"5!=5 = 0","5!=5p20 = 1","5!=2 = 1"}),
    Page("Less than","lt","function","Returns `1` if `a` is less than `b`, else returns `0`. It is aliased by the `<` operator. ?Imaginary? components are ignored. Opposite of this is ?greater than or equal?.",{"comparison","<"},{"5<2 = 0","2<5 = 1","5<5"}),
    Page("Greater than","gt","function","Returns `1` if `a` is greater than `b`, else returns `0`. It is aliased by the `>` operator. ?Imaginary? components are ignored. Opposite of this is ?less than or equal?.",{"comparison",">"},{"5>2 = 1","2>5 = 0","5>5 = 0"}),
    Page("Greater than or equal","gt_equal","function","Returns `1` if `a` is ?greater than? or ?equal? to `b`, else returns `0`. It is aliased by the `>=` operator. ?Imaginary? components are ignored. Beware that ?equal? will return false if they are of different types.",{"comparison",">="},{"5>=5 = 1","2>=5 = 0"}),
    Page("Less than or equal","lt_equal","function","Returns `1` if `a` is ?less than? or ?equal? to `b`, else returns `0`. It is aliased by the `<=` operator. ?Imaginary? components are ignored. Beware that ?equal? will return false if they are of different types.",{"comparison","<="},{"5<=5 = 1","2<=5 = 1","5<=2 = 0"}),
    Page("Not","not","function","Returns the boolean opposite of `x`. If `x` is a zero, it will return one. If `x` is any value other than zero, it will return zero.",{"bitwise","opposite"},{"not(0) = 1","not(1) = 0","not(2) = 0"}),
    Page("And","and","function","Returns the bitwise and of `a` and `b`. If `a` and `b` are constrained to one or zero, it works like a boolean and. Both arguments are rounded to integers before calculation.",{"bitwise"},{"and(1,1) = 1","and(0,1) = 0","and(2,3) = and(0b10,0b11) = 0b10 = 2"},"https://en.wikipedia.org/wiki/Bitwise_operation#AND"),
    Page("Or","or","function","Returns the bitwise or of `a` and `b`. If `a` and `b` are constrained to one or zero, it works like a boolean or. Both arguments are rounded to integers before calculation.",{"bitwise"},{"or(0,1) = 1","or(0,0) = 0","or(2,1) = or(0b10,0b01) = 0b11 = 3"},"https://en.wikipedia.org/wiki/Bitwise_operation#OR"),
    Page("Xor","xor","function","Returns the bitwise xor of `a` and `b`. If `a` and `b` are either one or zero, it works like a boolean operator. Both arguments are rounded to integers before calculation",{"bitwise","exclusive or"},{"xor(0,0) = 0","xor(0,1) = 1","xor(1,1) = 0","xor(1,3) = xor(0b01,0b11) = 0b10 = 2"},"https://en.wikipedia.org/wiki/Bitwise_operation#XOR"),
    Page("Left shift","ls","function","Returns the bitwise left shift of `a` shifted by `b`. It is essentially equivalent to `a * 2^b`, however both arguments are rounded to integers before calculation. Left shift is circular, so if it overflows to the left, it will wrap around. The inverse is the right shift function `rs`.",{"bitwise","bit shift"},{"ls(1,2) = 1 * 2^2 = 4","ls(3,1) = 3 * 2^1 = 6"},"https://en.wikipedia.org/wiki/Circular_shift#Example"),
    Page("Right shift","rs","function","Returns the bitwise right shift of `a` shifted by `b`. It is essentially equivalent to `a / 2^b`, however both arguments are rounded to integers before and after calculation. Right shift is an arithmetic shift, so if it overflows to the right, it will be cut off. The inverse is the left shift function `ls`.",{"bitwise","bit shift"},{"rs(5,1) = 5 / 2^1 = 2","rs(4,1) = 4 / 2^1 = 2"},"https://en.wikipedia.org/wiki/Arithmetic_shift"),
    #pragma endregion
    #pragma region Constants
    Page("True","true","function","Constant that returns 1. It is used for boolean logic because comparison operators return `1` on success. The `false` constant is the opposite of `true`.",{"constant","binary","boolean"}),
    Page("False","false","function","Constant that returns `0`. It is used for boolean logic because comparison operators return `0` on failure. The `true` constant is the opposite of `false`.",{"constant","binary","boolean"}),
    Page("Imaginary number","i","function","Constant that returns the imaginary number `i`. `i^2` is equal to `-1`, and it is used a lot in higher level math and algebra.",{"constant","imaginary"},{"i*i = -1","2+i + 3-2i = 5-i","(1+i) * (2-i) = 2+i-2i-i^2 = 2-i-(-1) = 3-i"},"https://en.wikipedia.org/wiki/Imaginary_number"),
    Page("Pi","pi","function","Constant that retuns the number pi. Pi is the ratio between the circumference ofa circle and it's diameter. Pi is used a lot in ?trigonometry?. Pi returns a 15-digit float, see ?arb_pi? for a more accurate calculation.",{"constant"},{"pi = 3.141592653"},"https://en.wikipedia.org/wiki/Pi"),
    Page("Euler's number","e","function","Constant that retuns euler's number, which is the base of natural logarithms. `e` is used a lot in math past algebra. `e` returns a 15-digit float, see ?arb_e? for a more accurate calculation.",{"constant"},{"e = 2.71828183"},"https://en.wikipedia.org/wiki/E_(mathematical_constant)"),
    Page("Arbitrary pi","arb_pi","function","Returns `pi` to `prec` decimal digits as an ?arbitrary precision? float. See ?pi? for the 15-digit approximation.",{"constant"},{"arb_pi(20) = 3.1415926535897932385p20"}),
    Page("Arbitrary Euler's number","arb_e","function","Returns Euler's number to `prec` decimal digits as an ?arbitrary precision? float. See ?e? for the 15-digit approximation.",{"constant"},{"arb_e(20) = 2.7182818284590452354p20"}),
    Page("Arbitrary random number","arb_rand","function","Returns an ?arbitrary precision? number that is randomly between 0 and 1, with `prec` digits of precision",{"constant"},{"arb_rand(10) = 0.4503537494p10"}),
    Page("Random","rand","function","Returns a unique random number between zero and one every time it is calculated. See `srand` for seeding.",{"constant"},{"max(fill(x=>rand,1000)) = 0.999999","min(fill(x=>rand,1000)) = 0"}),
    Page("Random seed","srand","function","Takes in a float as a random seed and seeds the random number generator. If the same seed is provided, `rand` will return the exact same sequence of random numbers.",{"constant"},{},"https://en.wikipedia.org/wiki/Random_seed"),
    Page("Undefined","undefined","function","Constant for a non-definable floating point exception. This constant is not meant for regular use but is returned by some functions in the case of an error.",{"constant","nan"}),
    Page("Infinity","inf","function","Constant for the floating point infinity. Please do not use this for calculations, only comparison with other infinities.",{"constant"}),
    Page("History length","histlen","function","Constant for the number of elements in the ?history?",{"constant","memory"}),
    Page("Answer","ans","function","Returns the previously calculated value. If an argument is provided, it returns the `x` index in the ?history? (starting at zero), or if `x` is negative, `x` values in the past (negative one being the previous value).",{"constant","history","memory"},{"ans(0) =` first calculation`","ans(3) =` fourth calculation`","ans(-1) =` previous calculation","ans(-2) =` one before previous calculation`"}),
    #pragma endregion
    #pragma region Lambdas
    Page("Run","run","function","Runs the ?lambda? function `func` with the arguments provided. If not enough arguments are provided, zeroes are filled in. If you have a ?vector? of arguments, use the `apply` function.",{"lambda","compute"},{"run(x=>x+2,4) = 6","run(_=>4) = 4","run((x,y)=>x+y, 4, 6) = 10"}),
    Page("Apply","apply","function","Runs the ?lambda? function `func` on each element in the ?vector? `args` and returns the resultant vector. If `func` is a ?string?, it will run that global ?function? on each element.",{"lambda","vector"},{"apply((x,y)=>x+y,<2,3>) = 5"},"apply(\"add\",<4,5>) = 9"),
    Page("Sum","sum","function","Returns the sum of each ?lambda? `func` run from `begin` up to and including `end` with an optional `step`. If a step is not included, `step` defaults to `1`. Another definition takes a single ?vector? and finds the sum of every element.",{"lambda","addition","plus","series"},{"sum(x=>x+1,0,5) = 1+2+3+4+5+6 = 21","sum(x=>x^2,0,4,2) = 0^2 + 2^2 + 4^2 = 20","sum(<1,2,3>) = 1+2+3 = 6"},"https://en.wikipedia.org/wiki/Summation#Capital-sigma_notation"),
    Page("Product","product","function","Returns the product of each ?lambda? `func` run from `begin` up to and including `end` with an optional `step`. If a step is not included, `step` defaults to `1`. Another definition takes a single ?vector? and finds the product of every element.",{"lambda","multiply","multiplication","times","series"},{"product(x=>x,1,4) = 1*2*3*4 = 24` (you should use ?factorial? here instead)`","product(<5,2,8>) = 5*2*8 = 80","product(x=>x^2,1,4) = 1^2 * 2^2 * 3^2 * 4^2 = 576"},"https://en.wikipedia.org/wiki/Product_(mathematics)#Product_of_a_sequence"),
    Page("Derivative","derivative","function","Returns the derivative of the ?lambda? function that is passed in. It uses an algorithm to find the derivative of any expression (however some functions are not yet supported), and then passes through ?simplify?. The second argument in the call to derivative is which argument it will take the derivative with respect to, it defaults to zero and is optional.",{"lambda","calculus"},{"derivative(x=>x^2) = x=>2x","derivative(x=>sin(x)) = x=>cos(x)","derivative((x,y)=>x*y,1) = (x,y)=>x `(with respect to y)`","`Given `a(y,z)=y^2+z^3`, `derivative(x=>a(3x,x-2)) = x=>3*(x-2)^2+18x","derivative(x=>infinite_sum(n=>x^n/n!)) = x=>infinite_sum(n=>n*x^n/x/n!)"},"https://en.wikipedia.org/wiki/Derivative"),
    Page("Simplify","simplify","function","Returns the simplification of the lambda function that is passed to it. Currently it only supprts simplification of types with elementary functions like addition, multiplication, and exponent type functions. Any other functions like logarithms or trig functions will be left untouched. It also computes functions that returns integers.",{"lambda","reduce"},{"simplify(_=>sqrt(16)) = _=>4","simplify(_=>2/6) = _=>1/3","simplify(x=>(x+3)(x-2)/(x+3)) = x=>x-2","simplify(x=>x*x*x) = x^3","simplify(x=>x+x+x) = x=>3x"}),
    Page("Infinite series","infinite_sum","function","Returns the sum of the lambda as the first argument goes to infinity. If a starting index is required, it can be the second argument. This can be used to execute the taylor series or test for convergence. The function will, however, return whatever result it got after `100000` iterations. Otherwise, the function will stop when the desired precision is reached, whether it is an ?arb? number or a regular one.",{"lambda","calculus","sum"},{"infinite_sum(n=>1/n!) = 1/0! + 1/1! + 1/2!... = 2.718... = e","infinite_sum(n=>1/n!,1) = 1/1! + 1/2! ... = e-1","infinite_sum(n=>1/toarb(n,30)!) = arb_e(30)"},"https://en.wikipedia.org/wiki/Series_(mathematics)#Examples_of_numerical_series"),
    Page("Definite integral","dint","function","Returns the definite integral of `func` from `a` to `b`. The definite integral is the area under the curve. This function uses Simpson's method to approximate the area, and successively increases the number of intervals until a desired accuracy is reached. If `a` and `b` are swapped, it will be the same answer but negative.",{"lambda","calculus"},{"dint(x=>x^2,0,1) = 0.33333","dint(x=>sin(x),0,pi/2) = 1","dint(x=>sqrt(x)+3x,1,4) = 27.16666667"},"https://math.libretexts.org/Courses/Mount_Royal_University/MATH_2200%3A_Calculus_for_Scientists_II/2%3A_Techniques_of_Integration/2.5%3A_Numerical_Integration_-_Midpoint%2C_Trapezoid%2C_Simpson's_rule"),
    #pragma endregion
    #pragma region Vectors
    Page("Length","length","function","Returns the number of elements in `obj`, whether it is a ?vector?, ?map?, or a ?string?.",{"vector","map","string"},{"length(<1,2,4.5>) = 3","length(\"Hello\") = 5"}),
    Page("Magnitude","magnitude","function","Returns the magnitude of a vector, which is the distance from zero to that point.",{"vector","distance"},{"magnitude(<1,1>) = sqrt(2)"},"https://en.wikipedia.org/wiki/Magnitude_(mathematics)#Euclidean_vector_space"),
    Page("Normalize","normalize","function","Returns a vector divided by it's magnitude, so the magnitude of the return value is `1`.",{"vector"},{"normalize(vec) = vec/magnitude(vec)","normalize(<1,1>) = <0.707,0.707>"},"https://en.wikipedia.org/wiki/Unit_vector"),
    Page("Get element","get","function","Returns the element at `key` in `map`. If `map` is a vector, it returns the element with index `key`, with the first element having index `0`. If map is a ?map? type, it returns the value at that key. In either case, if the key is outside of bounds, `get` returns `0`.",{"vector","access"},{"get(<1,2>,0) = 1","get(<1,2,3>,14) = 0","get({1:2,5:4},5) = 4","get({\"yes\":1,\"no\":0},\"yes\") = 1"}),
    Page("Fill vector","fill","function","Returns a ?vector? with `count` elements generated by the ?lambda? function `func`. The first index is `0`.",{"vector","lambda","generate"},{"fill(x=>x,5) = <0,1,2,3,4>","fill(x=>x^2,5) = <0,1,4,9,16>","fill(x=>x/2,3) = <0,0.5,1>"}),
    Page("Map vector","map_vector","function","Returns a new ?vector? which is the result of each element in the first argument being passed through ?lambda? function `func`. The lambda can also take a second argument, which is the index of the element.",{"vector","lambda"},{"map_vector(<4,0,-1>,x=>sqrt(x)) = <2,0,i>","map_vector(<0,1,2>,x=>x+1) = <1,2,3>","map_vector(<4,2,5>,(x,index)=>x+index) = <4,3,7>"}),
    Page("Every","every","function","Returns `1` or `0` depending on whether all elements in the vector return true when passed through `func`.",{"vector","all","lambda"},{"every(<1,2,3>,x=>(x>0) = 1","every(<0,1,2>,x=>(x>0)) = 0"}),
    Page("Concatenate","concat","function","Retuns the concatenation of the two ?vector? or ?map? objects `a` and `b`. To concatenate ?string? objects, use the ?add? function.",{"vector","addition","join"},{"concat(<1,2>,<3,4>) = <1,2,3,4>","concat({1:-5,5:8},{9:3}) = {1:-5,5:8,9:3}"}),
    Page("Sort","sort","function","Returns the sorted form of `vec` using the optional `comp` ?lambda? function. If `comp` is not passed, it defaults to the ?less than? function `lt`. For the second argument, two elements will be swapped if the function returns `0`. Internally, this uses the merge sort algorithm.",{"vector","order"},{"sort(<4,-5,2.3>) = <-5,4,2.3>","sort(<\"abc\",\"b\",\"jh\">,(a,b)=>length(a)<length(b)) = <\"b\",\"jh\",\"abc\">"}),
    #pragma endregion
    #pragma region Strings
    Page("Evaluate","eval","function","Returns the calculation of the ?string? `str` in the global context. Eval is not aware of local variables or arguments, so `x=>eval(\"x\")` will not work.",{"string","calculation","calculate"},{"eval(\"4-2\") = 2"}),
    Page("Error","error","function","Throws the error `str` and ends computation.",{"string","throw"}),
    Page("Substring","substr","function","Returns a copy of the ?string? `str` starting from `begin` and continuing for `len` characters. The first character is at index zero, and if a length is not provided, it will copy until the end of the string.",{"string","slice"},{"substr(\"Green apple\",2,5) = \"een a\""}),
    Page("Lowercase","lowercase","function","Returns the ?string? `str` with each lowercase letter replaced with it's lowercase equivalent.",{"string","uppercase","capital","format"},{"lowercase(\"ABed\") = \"abed\""}),
    Page("Uppercase","uppercase","function","Returns the ?string? `str` with each lowercase letter replaced with it's uppercase equivalent.",{"string","lowercase","capital","format"},{"uppercase(\"ABed\") = \"ABED\""}),
    Page("Index of","indexof","function","Returns the index of `query` within the ?string? `str`. Searching is case-sensitive. The first character is index zero.",{"string","find","query","position"},{"indexof(\"Red apple\",\"apple\") = 4"}),
    Page("Replace","replace","function","Returns the ?string? `str` with each instance of `find` replaced with `rep`.",{"string","query","find"},{"replace(\"heed\",\"e\",\"o\") = \"hood\""}),
    #pragma endregion
    #pragma region Conversion
    Page("To Number","tonumber","function","Returns `val` converted to a 15-digit floating point ?number?. If the input is a ?vector? or ?map?, it will return the element at index zero. Conversions from ?lambda? are not supported.",{"convert"},{"tonumber(0.512p20) = 0.512","tonumber(<4,3,1>) = 4","tonumber({4:5,0:2}) = 2","tonumber(\"51.2451\") = 51.2451"}),
    Page("To Arb","toarb","function","Returns `val` converted to an ?arbitrary precision? number, the second argument specifies the number of decimals of precision (optional, but defaults to 15). If the input is a ?vector? or ?map?, it will return the zeroeth index. Conversions from ?lambda? are not supported.",{"convert"},{"toarb(5.12,50) = 5.12p50","toarb(<1.3,2>,20) = 1.3p20","toarb(4.2151) = 4.2151p15","toarb(\"514.2p20\") = 514.2p20"}),
    Page("To Vector","tovec","function","Returns `val` converted to a single element ?vector?. If the current type is a ?map?, it will look for keys that are indicies from 0 to 1000 and fill them in the vector, any other type of key is ignored. If the current type is any other, it will be converted to a vector of size one with the value at the zeroeth index.",{"convert"},{"tovector(4) = <4>","tovector(\"hello\") = <\"hello\">","tovector(x=>rand+x) = <x=>rand+x>","tovector({0:3,0.5:2,1:4,6:3}) = <3,4,0,0,0,0,3>"}),
    Page("To Map","tomap","function","Returns `val` converted to ?map? type. If the current type is a vector, it will pair each value with it's index. Any other type will be converted to `{0:x}`.",{"convert"},{"tomap(16) = {0:16}","tomap(<4,12,5>) = {0:4,1:12,2:5}"}),
    Page("To String","tostring","function","Returns `val` converted to ?string? type. This is the same as if it were printed.",{"convert"},{"tostring(5.4) = \"5.4\"","tostring(x=>x+1) = \"x=>x+1\""}),
    Page("To Lambda","tostring","function","Returns a constant ?lambda? type that returns `val`. For any input `x`, the return value is `_=>x`.",{"convert"},{"tolambda(45) = _=>45"}),
    Page("Typeof","typeof","function","Returns an integer representing the type of the input `val`. Types are: 0-?number?, 1-?arb?, 2-?vec?, 3-?lambda?, 4-?string?, 5-?map?. If -1 is returned, the type of `val` is null.",{"convert"},{"typeof(1+i) = 0","typeof(0.513p20) = 1","typeof(<1,2>) = 2","typeof(x=>x+1) = 3","typeof(\"h\") = 4","typeof({1:2}) = 5"}),
    #pragma endregion
    #pragma endregion
    #pragma region Units
    #pragma region Base units
    Page("","m","unit","Meter is a metric base unit representing length",{"length","distance","metre"},{},"https://en.wikipedia.org/wiki/Metre"),
    Page("","kg","unit","Kilogram is a metric base unit representing mass or weight.",{"mass","weight","gram"},{},"https://en.wikipedia.org/wiki/Kilogram"),
    Page("","s","unit","Second is a metric base unit representing time.",{"time","second"},{},"https://en.wikipedia.org/wiki/Second"),
    Page("","A","unit","The Amp is a metric base unit representing electrical current.",{"current","amps"},{},"https://en.wikipedia.org/wiki/Ampere"),
    Page("","mol","unit","The mole is a metric base unit representing amount of substance. It is mainly used in chemistry",{"substance"},{},"https://en.wikipedia.org/wiki/Mole_unit"),
    Page("","K","unit","The kelvin is a metric base unit representing temperature. The size of each degree Kelvin is equivalent to a degree Celsius, however zero Kelvin is set at absolute zero heat. It can be used as an alternative to Celsius.",{"temperature","heat","celsius"},{},"https://en.wikipedia.org/wiki/Kelvin"),
    Page("","$","unit","The dollar is a base unit representing currency. It is independent to any currency system and can be used to represent any currency.",{"currency"}),
    Page("","b","unit","The bit is a base unit representing information size.",{"information","byte"},{},"https://en.wikipedia.org/wiki/Bit"),
    Page("","N","unit","The Newton is a derived metric unit measuring force.",{"force"},{},"https://en.wikipedia.org/wiki/Newton_(unit)"),
    Page("","J","unit","The Joule is a derived metric unit measuring energy. It is equivalent to `1/3600` ?watt hour? units.",{"energy"},{},"https://en.wikipedia.org/wiki/Joule"),
    Page("","W","unit","The Watt is a derived metric unit measuring energy per second. It is equivalent to one ?Joule? per ?second?.",{"energy"},{},"https://en.wikipedia.org/wiki/Watt"),
    Page("","V","unit","The Volt is a derived metric unit measuring electric potential. It is most commonly represented as one ?watt? per ?Amp?.",{"electric potential",},{},"https://en.wikipedia.org/wiki/Volt"),
    Page("","Pa","unit","The pascal is a derived metric unit measuring pressure. Pascals are very small, and an ?atmosphere? is over `100000` pascals. It is equivalent to one ?Newton? per square ?meter?.",{"pressure"},{},"https://en.wikipedia.org/wiki/Pascal_(unit)"),
    Page("","bps","unit","Bits per second is a derived metric unit measuring information speed. It is equivalent to one ?bit? per ?second?.",{"information rate","bytes per second"}),
    Page("","Hz","unit","Hertz is a derived metric unit measuring frequency.",{"frequency"},{},"https://en.wikipedia.org/wiki/Hertz"),
    Page("","Wh","unit","The watt hour is the energy required to power one ?watt? for one ?hour?",{"energy"}),
    Page("","Ah","unit","The amp hour is the charge required to run one ?amp? of current for one ?hour?",{"charge"}),
    Page("","B","unit","The byte is the information in 8 ?bits?.",{"information"}),
    Page("","Bps","unit","The bytes per second is the information in 8 ?bits? per second.",{"information"}),
    Page("","are","unit","The are is a unit of area equal to `1000` square ?meters?.",{"area"}),
    Page("","bar","unit","The bar is a unit of pressure equal to `100000` ?pascal?. It is approximately one ?atmosphere? of pressure.",{"pressure"}),
    Page("","min","unit","The minute is a unit of time equal to `60` ?seconds?.",{"time"}),
    Page("","hr","unit","The hour is a unit of time equal to `3600` ?seconds?.",{"time"}),
    Page("","kph","unit","The kilometers per hour is a unit of velocity for `1000` ?meters? per ?hour?.",{"speed","velocity"}),
    Page("","tn","unit","The tonne is a unit of mass equal to `1000` ?kilogram?.",{"mass","weight"}),
    Page("","g","unit","The gram is a unit of mass equal to `0.001` ?kilogram?.",{"mass","weight"}),
    Page("","c","unit","The speed of light is a constant unit of velocity equal to `299_792_458` ?meters? per ?second?.",{"velocity","speed","constant"},{},"https://en.wikipedia.org/wiki/Speed_of_light"),
    Page("","atm","unit","The atmosphere is a constant unit of perssure equal to the pressure of the Earth's atmosphere at sea level.",{"pressure"},{},"https://en.wikipedia.org/wiki/Standard_atmosphere_(unit)"),
    Page("","eV","unit","The electron volt is a unit of charge equal to the charge of an electron.",{"electric charge"},{},"https://en.wikipedia.org/wiki/Electronvolt"),
    Page("","mach","unit","The mach is a unit of velocity equal to the speed of sound, approximately 340.3 ?meters? per ?second?.",{"velocity","speed","sound","speed of sound"},{},"https://en.wikipedia.org/wiki/Mach_number"),
    Page("","pc","unit","The parsec is a unit of distance commonly used in atronomical measurements.",{"length","distance"},{},"https://en.wikipedia.org/wiki/Parsec"),
    Page("","acre","unit","The acre is a non-metric unit of area.",{"area"},{},"https://en.wikipedia.org/wiki/Acre"),
    Page("","btu","unit","The british thermal unit is a non-metric unit of energy.",{"energy"},{},"https://en.wikipedia.org/wiki/British_thermal_unit"),
    Page("","ct","unit","The carat is a non-metric unit of weight.",{"mass","weight"},{},"https://en.wikipedia.org/wiki/Carat_(mass)"),
    Page("","day","unit","The day is a unit of time.",{"time"}),
    Page("","floz","unit","The fluid ounce is a non-metric unit of volume.",{"volume"},{},"https://en.wikipedia.org/wiki/Fluid_ounce"),
    Page("","gallon","unit","The gallon is a non-metric unit of volume.",{"volume"},{},"https://en.wikipedia.org/wiki/Gallon"),
    Page("","in","unit","The inch is a non-metric unit of length.",{"length","distance"},{},"https://en.wikipedia.org/wiki/Inch"),
    Page("","lb","unit","The pound is a non-metric unit of mass or weight.",{"mass","weight"},{},"https://en.wikipedia.org/wiki/Pound_(mass)"),
    Page("","mi","unit","The mile is a non-metric unit of length.",{"length","distance"},{},"https://en.wikipedia.org/wiki/Mile"),
    Page("","mph","unit","The ?mile? per ?hour? is a non-metric unit of speed.",{"velocity","speed"}),
    Page("","nmi","unit","The nautical mile is a non-metric unit of distance.",{"length","distance"},{},"https://en.wikipedia.org/wiki/Nautical_mile"),
    Page("","oz","unit","The ounce is a non-metric unit of weight.",{"mass","weight"},{},"https://en.wikipedia.org/wiki/Ounce"),
    Page("","psi","unit","The ?pounds? per square ?inch? is a non-metric unit of pressure.",{"pressure"},{},"https://en.wikipedia.org/wiki/Pound_per_square_inch"),
    Page("","tbsp","unit","The tablespoon is a non-metric unit of volume.",{"volume"},{},"https://en.wikipedia.org/wiki/Tablespoon"),
    Page("","tsp","unit","The teaspoon is a non-metric unit of volume.",{"volume"},{},"https://en.wikipedia.org/wiki/Teaspoon"),
    Page("","yd","unit","The yard is a non-metric unit of length, approximately equal to one ?meter?.",{"length","distance"},{},"https://en.wikipedia.org/wiki/Yard"),
    #pragma endregion
    #pragma endregion
    #pragma region Commands
    Page("Include","/include","command","The include command includes functions from a predetermined list. Several names can be given separated by spaces. Search for ?List of includes? to see them all.",{},{"/include solvequad","/include stddev mean"}),
    Page("Parse","/parse","command","The parse command parses the expression to a tree and then returns it without computing.",{},{"/parse add(1,2)` -> `1+2"}),
    Page("Metadata","/meta","command","The meta command returns a list of metadata for the program like author name and version."),
    Page("List","/ls","command","The ls command lists every ?variable? in the global scope. Variables can be defined with the ?def? command."),
    Page("Help","/help","command","The help command finds the most relevant help page to the search query and displays it.",{},{"/help help command","/help vector"}),
    Page("Query","/query","command","The query command creates a list of help pages from the query sorted by relevance. It can be rerun with the index to display.",{},{"/query log","`Open with: `/query log 0"}),
    #pragma endregion
    #pragma region Lists
    //Automatically generated in Page::addListData
    Page("List of functions","","list","",{"builtin"}),
    Page("List of units","","list",""),
    Page("List of commands","","list",""),
    Page("List of includes","","list",""),
    Page("List of pages","","list",""),

    #pragma endregion
};
#pragma region Searching
std::map<uint64_t, std::vector<std::pair<int, int>>> Help::queryHash;
void Help::stringToHashList(std::vector<std::pair<uint64_t, int>>& hashOut, const string& str, int basePriority = 1) {
    //X represents the position within a word
    int i = 0, x = 0;
    uint64_t hash = 0;
    for(i = 0;i < str.length();i++) {
        //If word separator has been reached
        if(std::strchr(" .,[]()-+`?;:\"", str[i])) {
            if(hash != 0) {
                //If starts with, add extra priority
                if(x == i) hashOut.push_back({ hash,basePriority + 2 });
                else hashOut.push_back({ hash,basePriority });
            }
            x = 0, hash = 0;
        }
        else {
            uint64_t ch = str[i];
            if(ch >= 'A' && ch <= 'Z') ch += 'a' - 'A';
            if(x < 8) hash |= ch << ((7 - x) * 8);
            x++;
        }
    }
    if(hash != 0) {
        //If exact match, add extra priority
        if(x == i) hashOut.push_back({ hash,basePriority + 7 });
        else hashOut.push_back({ hash,basePriority });
    }
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
    static const string removedWords = "is the does coefficient are acceptable a b x with to that supports support see returns 1 2 3 4 5 6 7 8 9 0 of ";
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
                for(int x = 0;x < lower->second.size();x++) {
                    int priority = lower->second[x].second;
                    if(priority > 10 && i != 0) priority = 9;
                    if(strcasecmp(query.c_str(), Help::pages[lower->second[x].first].name.c_str()) == 0) priority += 10;
                    results[lower->second[x].first].second += priority;
                }
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
#pragma endregion