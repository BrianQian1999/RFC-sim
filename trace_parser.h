// Turing SASS trace parser, stdin -> struct TraceInstr
// Qiran Qian, <qiranq@kth.se>

#ifndef TRACE_PARSER_H
#define TRACE_PARSER_H

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include "trace_instr.h"
#include "utils.h"

struct KernelInfo {
	KernelInfo() {}
	KernelInfo(std::string n, unsigned id, utils::Dim3<int> gridDim, utils::Dim3<int> blockDim)
		: __kernel_name(n), __kernel_id(id), __grid_dim(gridDim), __block_dim(blockDim) {}
	
	// Kernel Infos
	std::string __kernel_name;
	unsigned __kernel_id;
	utils::Dim3<int> __grid_dim;
	utils::Dim3<int> __block_dim;

	// Modifiers
	void ModifyKernelName(std::string s) { this->__kernel_name = s; }
	void ModifyKernelID(unsigned id) { this->__kernel_id = id; }
	void ModifyGridDim(utils::Dim3<int> dim) { this->__grid_dim = dim; }
	void ModifyBlockDim(utils::Dim3<int> dim) { this->__block_dim = dim; }
};

std::ostream & operator<< (std::ostream & os, const KernelInfo & info) {
	std::cout << "[Kernel Info]: " << std::endl;
	std::cout << "Name: " << info.__kernel_name << std::endl;
	std::cout << "ID: " << info.__kernel_id << std::endl;
	std::cout << "GridDim: " << info.__grid_dim << std::endl;
	std::cout << "BlockDim: " << info.__block_dim << std::endl;
	return os;
}

class TraceParser {
private: 
	explicit TraceParser(std::string s) : __fn(s) {
		std::ifstream ifs(s);
		__ifs = std::move(ifs);
		if(!__ifs.is_open()) {
			std::cerr << "[TraceParser] Error: Cannot open trace file." << std::endl;
			exit(EXIT_FAILURE);
		}
	}
	~TraceParser() {
		__ifs.close();
		if(__ifs.is_open()) {
			std::cerr << "[TraceParser] Error: Cannot close ifstream." << std::endl;
			exit(EXIT_FAILURE);
		}
	}
private:
	std::ifstream __ifs; // Input file stream
	std::string __fn; // Trace file name
	KernelInfo __kernel_info; // Kernel information

public:
	std::string GetLine() {
		std::string str;
		std::getline(__ifs, str);
		return str;
	}
	
	// Basically, parse a line and map to TraceInstr
	TraceInstr ParseInstr() {
		std::string instStr;
		std::getline(__ifs, instStr);
#ifndef NDEBUG
		std::cout << "[DEBUG][TraceParser] string read from line: " << instStr << std::endl;
#endif		

		std::stringstream ss(instStr);
		std::string tokStr;
		std::vector<std::string> tokStrs; // SubStrings
		while(std::getline(ss, tokStr, ' ')) {
			tokStrs.push_back(tokStr);
		}
#ifndef NDEBUG
		std::cout << "[DEBUG][TraceParser] toks got from string: ";
		for (auto tok : tokStrs) {
			std::cout << tok << " ";
		}
		std::cout << std::endl;
#endif
		
		// Parse the tokens
		if(tokStrs.empty()) return ParseInstr(); // If empty line, call the parser recursively
		else if(tokStrs[0] == "-kernel" && tokStrs[1] == "name" && tokStrs.size() >= 4) {
			this->__kernel_info.ModifyKernelName(tokStrs[3]);
			return ParseInstr();
		}
		else if(tokStrs[0] == "-kernel" && tokStrs[1] == "id" && tokStrs.size() >= 4 {
			this->__kernel_info.ModifyKernelID( 
		}
		
		TraceInstr traceInstr;
		return traceInstr;
	}


};

#endif
