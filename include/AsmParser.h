#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <stdexcept>
#include <array>
#include <unordered_map>
#include <vector>
#include <bitset>
#include <regex>
#include <iomanip>

struct AsmParser {
	std::shared_ptr<std::ifstream> asmIfs;
	std::shared_ptr<std::unordered_map<uint32_t, std::bitset<4>>> reuseInfoTab;

	AsmParser();
    AsmParser(const std::string & asmFile, std::shared_ptr<std::unordered_map<uint32_t, std::bitset<4>>> tab);
	
	void parse();
};

