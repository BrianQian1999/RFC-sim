// Turing SASS trace instruction model
// Qiran Qian, <qiranq@kth.se>

#ifndef TRACE_INST_H
#define TRACE_INST_H

#include <iostream>
#include <vector>

#include "utils.h"
#include "trace_opcode.h"
#include "reg_operand.h"

struct TraceInst_t {	
	// Constructor
	TraceInst_t() {
		this->opcode = OP_VOID; // A fake opcode
	}
    
	TraceInst_t(unsigned pc, utils::Dim3<int> tb_id, unsigned w_id, InstOpcode opcode, std::vector<regOps::RegOperand_t> regPool, const std::bitset<4> & flags)
        : pc(pc), tb_id(tb_id), warp_id(w_id), opcode(opcode), regPool(regPool), reuse_flags(flags) {}

	// TB/warp info
	utils::Dim3<int> tb_id; // Thread Block ID
	unsigned warp_id; // Warp ID (0 - 32 for Turing)

	unsigned pc;
    std::bitset<4> reuse_flags;
    
    // Instruction model 
	InstOpcode	opcode; // enum InstrOpcode opcode			
	std::vector<regOps::RegOperand_t>	regPool; // Registers
};

std::ostream & operator<<(std::ostream & os, const TraceInst_t & traceInst) {
    std::cout << "<Inst>: " << " ";
    std::cout << "[Block ID]:" << traceInst.tb_id << " ";
    std::cout << "[Warp ID]:" << traceInst.warp_id << " ";
	std::cout << "[PC]: " << traceInst.pc << " ";
    std::cout << "[Opcode]:" << MapOpcode2String(traceInst.opcode) << " ";
    if(!traceInst.regPool.empty()) {
		for(auto reg : traceInst.regPool) {
        	std::cout << "[Reg]:" << reg << " "; 
    	}
	}
    return os;
}

#endif
