#include "../src/_header.hpp"
#include <emscripten/bind.h>

namespace wasm {
    string evaluate(string xpr) {
        Value tr = Tree::parseTree(xpr, Program::parseCtx);
        return tr->compute(Program::computeCtx)->toString();
    }
};

using namespace emscripten;
EMSCRIPTEN_BINDINGS(module) {
    function("evaluate", &wasm::evaluate);
}

int main() {
    Program::startup();
    return 0;
}