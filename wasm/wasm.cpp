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
    string runLine(string call) {
        try {
            return Program::runLine(call).getStr();
        }
        catch(string mess) { return "Error: " + mess; }
        catch(const char* mess) { return "Error:" + string(mess); }
        catch(...) { return "Error: unknown"; }
    }
};

using namespace emscripten;
EMSCRIPTEN_BINDINGS(module) {
    function("evaluate", &wasm::evaluate);
    function("runLine", &wasm::runLine);
}

int main() {
    Program::startup();
    return 0;
}