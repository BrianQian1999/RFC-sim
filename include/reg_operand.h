// RFC-sim register model
// Qiran Qian, <qiranq@kth.se>

#ifndef REG_OPERAND_H
#define REG_OPERAND_H 

#include <iostream>
#include <unordered_map>
#include <stdexcept>

namespace regOps {
	// register operand type, source/destination
	enum RegOperandType_e {
		SRC = 0,
		DST,
        ADDR // Logically we consider an address as an SRC reg.
	};

    // Just implement the pretty printer with a table...
    std::string RegOperandTypeStr(RegOperandType_e regType) {
        std::unordered_map<RegOperandType_e, std::string> hashMap = {
            std::pair<RegOperandType_e, std::string>(SRC, "R"), // Read
            std::pair<RegOperandType_e, std::string>(DST, "W"), // Write
            std::pair<RegOperandType_e, std::string>(ADDR, "A") // Address
        };
        auto it = hashMap.find(regType);
        if(it == hashMap.end()) {
            throw std::invalid_argument("[RegOperandTypeStr] Invalid input RegOperandType_e.");
        }
        return it->second;
    }

    std::ostream & operator<<(std::ostream & os, const RegOperandType_e & regOperandType) {
        std::cout << RegOperandTypeStr(regOperandType);
        return os;
    }

	// register operand
	struct RegOperand_t {
        RegOperand_t() {}
        RegOperand_t(RegOperandType_e type) : regType(type) { regIndex = 0; } // Use this constructor only for ADDR type
        RegOperand_t(RegOperandType_e type, unsigned index) : regType(type), regIndex(index) {}

		RegOperandType_e regType; // SRC = 0, DST = 1, ADDR = 2
		unsigned regIndex;

        RegOperandType_e RegType() const { return this->regType; }
        unsigned RegIndex() const { return this->regIndex; }  
	};

    std::ostream & operator<<(std::ostream & os, const RegOperand_t & regOperand) {
        std::cout << "R" << regOperand.regIndex << "." << regOperand.regType;
        return os;
    }

}; // namespace regOps

#endif
