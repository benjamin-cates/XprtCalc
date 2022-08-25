#include "../src/_header.hpp"
#include <emscripten.h>
#include <emscripten/bind.h>

namespace wasm {
    string evaluate(string xpr) {
        try {
            Value tr = Tree::parseTree(xpr, Program::parseCtx);
            return tr->compute(Program::computeCtx)->toString();
        }
        catch(string mess) { return "Error: " + mess; }
        catch(const char* mess) { return "Error:" + string(mess); }
        catch(...) { return "Error: unknown"; }
    }
    string highlight(string str, string colors) {
        if(str == "") return "";
        return ColoredString(str, colors).toHTML();
    }
    string highlightLine(string str) {
        string colors = Expression::colorLine(str, Program::parseCtx);
        return highlight(str, colors);
    }
    string highlightExpression(string str) {
        string colors(str.length(), Expression::hl_error);
        Expression::color(str, colors.begin(), Program::parseCtx);
        return highlight(str, colors);
    }
    string runLine(string call) {
        return Program::runLine(call).getStr();
    }
    string runLineWithColor(string call) {
        ColoredString res = Program::runLine(call);
        return highlight(res.getStr(), res.getColor());
    }
    string query(string search) {
        Help::init();
        std::vector<Help::Page*> pages = Help::search(search);
        string out = "[";
        for(int i = 0;i < pages.size();i++) {
            if(i != 0) out += ",";
            out += "{\"name\": \"";
            out += pages[i]->name;
            if(pages[i]->symbol != "") out += " - " + pages[i]->symbol;
            out += "\",\"id\": ";
            out += std::to_string(pages[i] - Help::pages.data());
            out += "}";
        }
        out += "]";
        return out;
    }
    string helpPage(int id) {
        Help::init();
        return Help::pages[id].toHTML();
    }
};
ColoredString command_query(std::vector<string>& args, const string& self) {
    Help::init();
    string inp = Command::combineArgs(args);
    emscripten_run_script(("panelPage('help');helpSearch({'key':'Enter'},{'value':\"" + String::safeBackspaces(inp) + "\"})").c_str());
    return ColoredString("");
}
ColoredString command_help(std::vector<string>& args, const string& self) {
    Help::init();
    string inp = Command::combineArgs(args);
    emscripten_run_script(("openHelp(\"" + String::safeBackspaces(inp) + "\")").c_str());
    return ColoredString("");
}

using namespace emscripten;
EMSCRIPTEN_BINDINGS(module) {
    function("evaluate", &wasm::evaluate);
    function("highlight", &wasm::highlight);
    function("highlightLine", &wasm::highlightLine);
    function("highlightExpression", &wasm::highlightExpression);
    function("runLine", &wasm::runLine);
    function("runLineWithColor", &wasm::runLineWithColor);
    function("query", &wasm::query);
    function("helpPage",&wasm::helpPage);
}

void startup() {
    Program::commandList.insert(std::pair<string, Command>{"query", { &command_query }});
    Program::commandList.insert(std::pair<string, Command>{"help", { &command_help }});

}

int main() {
    Program::implementationStartup = &startup;
    Program::startup();
    EM_ASM(onProgramInitialized());
    return 0;
}