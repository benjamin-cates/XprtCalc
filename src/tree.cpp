#include "_header.hpp"
Tree::Tree(int opId, ValList&& branchList) {
    op = opId;
    branches = std::forward<ValList>(branchList);
}
Tree::Tree(string opStr, ValList&& branchList) {
    auto it = Program::globalFunctionMap.find(opStr);
    if(it == Program::globalFunctionMap.end())
        throw "Function " + opStr + " not found";
    op = it->second;
    branches = std::forward<ValList>(branchList);
}
Tree::Tree(string opStr, Value one, Value two) {
    *this = Tree(opStr, ValList{ one,two });
}
Tree::Tree(int opId) {
    op = opId;
}
bool Tree::operator==(const Tree& a)const {
    if(op != a.op) return false;
    if(branches.size() != a.branches.size()) return false;
    for(int i = 0;i < branches.size();i++) {
        if(branches[i] != a.branches[i]) return false;
    }
    return true;
}
Value Tree::operator[](int index) {
    return branches[index];
}
Value Tree::derivative() {
    return std::make_shared<Number>(0);
}
string Tree::toWebGL() {
    return "Execute";
}
void Tree::simplify() {

}
void Tree::compSimplify() {

}