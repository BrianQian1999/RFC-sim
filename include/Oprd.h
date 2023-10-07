#pragma once

#include <iostream>
#include <unordered_map>
#include <stdexcept>
#include <cstdint>

namespace reg {

    // register operand type
	enum OprdT {
		src = 0,
		dst,
        addr
	};

    inline const std::unordered_map<OprdT, std::string> OprdT2StrTab {
        std::pair<OprdT, std::string>(src, "src"), // Read
        std::pair<OprdT, std::string>(dst, "dst"), // Write
        std::pair<OprdT, std::string>(addr, "addr") // Address
    };

    // OprdT -> std::string
    inline std::string OprdT2Str(OprdT OprdT) {
        auto it = OprdT2StrTab.find(OprdT);
        if(it == OprdT2StrTab.end())
            throw std::invalid_argument("[OprdT2Str] Invalid register operand type.");
        return it->second;
    }

    inline std::ostream & operator<<(std::ostream & os, const OprdT & OprdT) {
        os << OprdT2Str(OprdT);
        return os;
    }

	struct Oprd {
        Oprd() {}
        Oprd(OprdT type, uint32_t pos) : type(type), pos(pos) { index = 256; set = pos; }
        Oprd(OprdT type, uint32_t index, uint32_t pos) : type(type), index(index), pos(pos) { set = pos;}
        Oprd(OprdT type, uint32_t index, uint32_t pos, uint32_t set) : type(type), index(index), pos(pos), set(set) {}

		OprdT type;
		uint32_t index;
        uint32_t pos;
        uint32_t set;
	};

    inline std::ostream & operator<<(std::ostream & os, const Oprd & Oprd) {
        os << "R" << Oprd.index << "." << Oprd.type << "." << Oprd.pos;
        return os;
    }

};
