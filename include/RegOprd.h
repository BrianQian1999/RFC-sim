#pragma once

#include <iostream>
#include <unordered_map>
#include <stdexcept>
#include <cstdint>

namespace reg {

    // register operand type
	enum RegOprdT {
		SRC = 0,
		DST,
        ADDR
	};

    inline const std::unordered_map<RegOprdT, std::string> RegOprdT2StrTab {
        std::pair<RegOprdT, std::string>(SRC, "src"), // Read
        std::pair<RegOprdT, std::string>(DST, "dst"), // Write
        std::pair<RegOprdT, std::string>(ADDR, "addr") // Address
    };

    // RegOprdT -> std::string
    inline std::string RegOprdT2Str(RegOprdT regOprdT) {
        auto it = RegOprdT2StrTab.find(regOprdT);
        if(it == RegOprdT2StrTab.end())
            throw std::invalid_argument("[RegOprdT2Str] Invalid register operand type.");
        return it->second;
    }

    inline std::ostream & operator<<(std::ostream & os, const RegOprdT & regOprdT) {
        os << RegOprdT2Str(regOprdT);
        return os;
    }

	struct RegOprd {
        RegOprd() {}
        RegOprd(RegOprdT type, uint32_t pos) : regType(type), regPos(pos) { regIndex = 0; }
        RegOprd(RegOprdT type, uint32_t index, uint32_t pos) : regType(type), regIndex(index), regPos(pos) {}

		RegOprdT regType;
		uint32_t regIndex;
        uint32_t regPos;
	};

    inline std::ostream & operator<<(std::ostream & os, const RegOprd & regOprd) {
        os << "R" << regOprd.regIndex << "." << regOprd.regType << "." << regOprd.regPos;
        return os;
    }

};
