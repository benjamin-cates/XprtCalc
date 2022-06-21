#include "_header.hpp"
Tree::Tree(int opId, ValList&& branchList) {
    op = opId;
    branches = std::forward<ValList>(branchList);
}
Tree::Tree(string opStr, ValList&& branchList) {
    op = Program::globalFunctionMap[opStr];
    branches = std::forward<ValList>(branchList);
}
Tree::Tree(string opStr, ValPtr one, ValPtr two) {
    int opId = Program::globalFunctionMap[opStr];
    if(opId == 0) throw "could not find " + opStr;
    op = opId;
    branches = ValList{ one,two };
}
Tree::Tree(int opId) {
    op = opId;
}
bool Tree::operator==(const Tree& a)const {
    if(op != a.op) return false;
    if(branches.size() != a.branches.size()) return false;
    for(int i = 0;i < branches.size();i++) {
        if(!(*branches[i] == a.branches[i])) return false;
    }
    return true;
}
ValPtr Tree::operator[](int index) {
    return branches[index];
}
ValPtr Tree::derivative() {
    return std::make_shared<Number>(0);
}
string Tree::toWebGL() {
    return "Execute";
}
void Tree::simplify() {

}
void Tree::compSimplify() {

}