// Turing SASS trace instruction model
// Qiran Qian, <qiranq@kth.se>

#ifndef TRACE_INST_H
#define TRACE_INST_H

#include <iostream>
#include <vector>

#include "utils.h"
#include "trace_opcode.h"
#include "reg_operand.h"

struct TraceInst {	
	// Constructor
	TraceInst() {
		this->opcode = OP_VOID;
	}
    
	TraceInst(const std::string & pc, utils::Dim3<int> tb_id, unsigned w_id, InstOpcode opcode, std::vector<regOps::RegOperand> regs)
        : pc(pc), tb_id(tb_id), warp_id(w_id), opcode(opcode), regs(regs) {} 
	// TB/warp info
	utils::Dim3<int> tb_id; // Thread Block ID
	unsigned warp_id; // Warp ID (0 - 32 for Turing)

	std::string pc;
	// Instruction model 
	InstOpcode	opcode; // enum InstrOpcode opcode			
	std::vector<regOps::RegOperand>	regs; // Registers
};

std::ostream & operator<<(std::ostream & os, const TraceInst & traceInst) {
    std::cout << "<Inst>: " << " ";
    std::cout << "[Block ID]:" << traceInst.tb_id << " ";
    std::cout << "[Warp ID]:" << traceInst.warp_id << " ";
	std::cout << "[PC]: " << traceInst.pc << " ";
    std::cout << "[Opcode]:" << MapOpcode2String(traceInst.opcode) << " ";
    if(!traceInst.regs.empty()) {
		for(auto reg : traceInst.regs) {
        	std::cout << "[Reg]:" << reg << " "; 
    	}
	}
    return os;
}

#endif
