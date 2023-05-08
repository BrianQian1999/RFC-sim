// RFC-sim register model
// Qiran Qian, <qiranq@kth.se>

#ifndef REG_OPERAND_H
#define REG_OPERAND_H 

#include <unordered_map>

namespace regOps {
	// register operand type, source/destination
	enum RegOperandType {
		SRC = 0,
		DST
	};

	// register operand
	struct RegOperand {
		RegOperandType __reg_operand_type; // SRC = 0, DST = 1
		unsigned __reg_index;	
	};
}


#endif
