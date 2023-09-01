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
	using mapT = std::unordered_map<uint32_t, std::bitset<4>>;
	std::shared_ptr<std::ifstream> asmIfs;
	std::shared_ptr<std::vector<mapT>> tab;

	AsmParser();
    AsmParser(const std::string&, const std::shared_ptr<std::vector<mapT>>&);
	
	void parse();
};

