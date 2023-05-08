// Turing SASS trace instruction model
// Qiran Qian, <qiranq@kth.se>

#ifndef TRACE_INSTR_H
#define TRACE_INSTR_H

#include <vector>

#include "utils.h"
#include "trace_opcode.h"
#include "reg_operand.h"

struct TraceInstr {	
	// Constructor
	TraceInstr() {}

	// TB/warp info
	utils::Dim3<int> tb_id; // Thread Block ID
	unsigned warp_id; // Warp ID (0 - 32 for Turing)

	// Instruction model 
	InstrOpcode	opcode; // enum InstrOpcode opcode			
	std::vector<regOps::RegOperand>	regs; // Registers
};

#endif
