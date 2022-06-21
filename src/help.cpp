#include "_header.hpp"
using namespace Help;
string Page::toPlainText() {
    return content;
}
string Page::toJSON() {
    string alias = "[";
    for(auto it = aliases.begin();it != aliases.end();it++) {
        alias += "\"" + String::safeBackspaces(*it) + "\"";
    }
    alias += ']';
    return "{name:\"" + name + "\", symbol: \"" + String::safeBackspaces(symbol) + "\", type: \"" + type + "\", aliases:" + alias + ", content: \"" + String::safeBackspaces(content) + "\"}";
}
string Page::getName() { return name; }
string Page::getSymbol() { return symbol; }
string Page::getType() { return type; }
string Page::getContent() { return content; }
std::set<string>& Page::getAliases() { return aliases; }
int Page::calculatePriority(const string& query) {
    if(name == query) return 0;
    return 10000;
}
std::vector<Page> Help::pages = {


};
std::vector<Page*> Help::search(const string& query) {
    //When pushing into the map, it will automatically be sorted
    std::map<int, Page*> sortedMap;
    for(int i = 0;i < pages.size();i++) {
        sortedMap[pages[i].calculatePriority(query)] = &pages[i];
    }
    //Copy over first ten elements of the priority map into the output vector
    int count=0;
    std::vector<Page*> out;
    for(auto it=sortedMap.begin();it!=sortedMap.end();it++) {
        out.push_back(it->second);
        count++;
        if(count>10) break;
    }
    return out;
}