#pragma once

#include <iostream>
#include <vector>
#include <bitset>
#include <ios>
#include <cstdint>
#include "Util.h"
#include "TraceOpcode.h"
#include "RegOprd.h"

struct TraceInst {	
	TraceInst()=default;
	TraceInst(
		uint32_t pc, 
		std::bitset<32> mask,
		util::Dim3<int> tbId,
		uint32_t wId,
		op::Opcode opcode,
		std::vector<reg::RegOprd> regPool,
		std::bitset<4> reuseFlag
	) : pc(pc), mask(mask), tbId(tbId), wId(wId), opcode(opcode), regPool(regPool), reuseFlag(reuseFlag) {}

	uint32_t pc;
	std::bitset<32> mask;
	util::Dim3<int> tbId;
	uint32_t wId; // warp id

	op::Opcode opcode;	
	std::vector<reg::RegOprd> regPool;
    std::bitset<4> reuseFlag;
};

inline std::ostream & operator<<(std::ostream & os, const TraceInst & traceInst) {
	auto & ins = traceInst;
	// os << "[Inst]: " << "(bId, wId, pc, mask, reuseFlag, opcode, regs) -> (";
	os << ins.tbId << ", " << ins.wId << ", " << ins.pc << ", " << ins.mask << ", "
			  << ins.reuseFlag << ", " << ins.opcode << ", <";
	for (const auto & reg : ins.regPool) {
		os << reg << " ";
	}
	os << ">";
    return os;
}
