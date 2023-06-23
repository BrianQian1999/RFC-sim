#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>

int main (int argc, char ** argv) {
    std::string s(" /*0000*/                   IMAD.MOV.U32 R1, RZ, RZ, c[0x0][0x28] ;                /* 0x00000a00ff017624 */");

    std::vector<std::string> toks;
    std::stringstream ss(s);
    std::string tok;
    while(std::getline(ss, tok, ' ')) {
        toks.push_back(tok);
    }
    for(auto e : toks) {
        std::cout << "tok: " << e << std::endl; 
    }
    return 0;
}
