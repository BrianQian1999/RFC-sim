#pragma once

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cctype>

#include "TraceInst.h"
#include "AsmParser.h"

struct KernelInfo {
	KernelInfo() {}
	KernelInfo(std::string n, unsigned id, util::Dim3<int> gridDim, util::Dim3<int> blockDim)
		: kernelSym(n), kernelID(id), gridDim(gridDim), blockDim(blockDim) {}
	
	std::string kernelSym;
	unsigned kernelID;
	util::Dim3<int> gridDim;
	util::Dim3<int> blockDim;
};

inline std::ostream & operator<< (std::ostream & os, const KernelInfo & info) {
	os << "[Kernel Info]: ";
	os << "(Name, id, gridDim, blockDim) -> (" 
       << info.kernelSym << ", "
       << info.gridDim << ", "
       << info.blockDim << ")\n"; 
	return os;
}

class TraceParser {
private:
    std::ifstream traceIfs;
	KernelInfo kernelInfo;
    std::shared_ptr<std::unordered_map<uint32_t, std::bitset<4>>> reuseInfo;
    util::Dim3<int> blockId;
    unsigned wId; 

public:
	explicit TraceParser(const std::string &, const std::shared_ptr<std::unordered_map<uint32_t, std::bitset<4>>>&);
 
    bool eof() const;
    void reset(const std::string&);
    bool isRegOprd(const std::string&) const;
    bool IsAddrOprd(const std::string&) const;
    reg::RegOprd parseReg(const std::string&, reg::RegOprdT, uint32_t) const;
    
    bool isInst(const std::vector<std::string>&) const;
    op::Opcode parseOpcode(const std::string&) const noexcept;

    void extendHmmaRegs(std::vector<reg::RegOprd>&) const;
    void extendImmaRegs(std::vector<reg::RegOprd>&) const;
    TraceInst parseInst(const std::vector<std::string> &);
    TraceInst parse();
};

