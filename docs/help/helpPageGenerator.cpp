#include "../../src/_header.hpp"
#include <fstream>
#include <iostream>
#include <filesystem>
using namespace Help;

int main() {
    Program::startup();
    string out;
    out += "<html lang='en'>\n";
    out += "<head>\n";
    out += "<title>XprtCalc Help</title>\n";
    out += "<meta charset='utf-8'>\n";
    out += "<meta name='description' content='Help pages for XprtCalc.' />\n";
    out += "<link rel='stylesheet' href='../../wasm/style.css'>\n";
    out += "<link rel='stylesheet' href='help.css'>\n";
    out += "<link rel='preconnect' href='https://fonts.googleapis.com'>";
    out += "<link rel='preconnect' href='https://fonts.gstatic.com' crossorigin>";
    out += "<link href='https://fonts.googleapis.com/css2?family=Lato&family=Roboto+Mono:wght@500&display=swap' rel='stylesheet'>\n";
    out += "</head>\n";
    out += "<body>\n";
    for(int i = 0;i < pages.size();i++) {
        out += "<div class='page_help' id='help_" + std::to_string(i) + "'>";
        out += pages[i].toHTML();
        out += "</div>\n";
    }
    out += "</body>\n";
    std::ofstream file;
    std::filesystem::path p = std::filesystem::path("docs") / "help" / "index.html";
    std::cout << "Writing to " << p.c_str() << std::endl;
    file.open(p, std::ofstream::trunc);
    file << out << std::endl;
    file.close();
    std::cout << "Help pages written successfully" << std::endl;
    return 0;
}