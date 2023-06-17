// RFC-sim register model
// Qiran Qian, <qiranq@kth.se>

#ifndef REG_OPERAND_H
#define REG_OPERAND_H 

#include <iostream>
#include <unordered_map>
#include <stdexcept>

namespace regOps {
	// register operand type, source/destination
	enum RegOperandType {
		SRC = 0,
		DST
	};

    // Just implement the pretty printer with a table...
    std::string MapRegOperandType2String(RegOperandType regType) {
        std::unordered_map<RegOperandType, std::string> hashMap = {
            std::pair<RegOperandType, std::string>(SRC, "R"), // Read
            std::pair<RegOperandType, std::string>(DST, "W") // Write
        };
        auto it = hashMap.find(regType);
        if(it == hashMap.end()) {
            throw std::invalid_argument("[MapRegOperandType2String] Invalid input RegOperandType.");
        }
        return it->second;
    }

    std::ostream & operator<<(std::ostream & os, const RegOperandType & regOperandType) {
        std::cout << MapRegOperandType2String(regOperandType);
        return os;
    }

	// register operand
	struct RegOperand {
        RegOperand() {}
        RegOperand(RegOperandType type, unsigned index) : 
            __reg_type(type), __reg_index(index) {}
		RegOperandType __reg_type; // SRC = 0, DST = 1
		unsigned __reg_index;

        RegOperandType RegType() const { return this->__reg_type; }
        unsigned RegIndex() const { return this->__reg_index; }  
	};

    std::ostream & operator<<(std::ostream & os, const RegOperand & regOperand) {
        std::cout << "R" << regOperand.__reg_index << "." << regOperand.__reg_type;
        return os;
    }
}

#endif
