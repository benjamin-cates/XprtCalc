#include "../src/_header.hpp"
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
        return ColoredString(string, color).toHTML();
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
};

using namespace emscripten;
EMSCRIPTEN_BINDINGS(module) {
    function("evaluate", &wasm::evaluate);
    function("highlight", &wasm::highlight);
    function("highlightLine", &wasm::highlightLine);
    function("highlightExpression", &wasm::highlightExpression);
    function("runLine", &wasm::runLine);
    function("runLineWithColor", &wasm::runLineWithColor);
}

int main() {
    Program::startup();
    return 0;
}