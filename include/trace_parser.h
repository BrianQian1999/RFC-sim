// Turing SASS trace parser, stdin -> struct TraceInstr
// Qiran Qian, <qiranq@kth.se>

#ifndef TRACE_PARSER_H
#define TRACE_PARSER_H

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cctype>

#include "trace_inst.h"
#include "trace_opcode.h"
#include "utils.h"

struct KernelInfo_t {
	KernelInfo_t() {}
	KernelInfo_t(std::string n, unsigned id, utils::Dim3<int> gridDim, utils::Dim3<int> blockDim)
		: __kernel_name(n), __kernel_id(id), __grid_dim(gridDim), __block_dim(blockDim) {}
	
	// Kernel Infos
	std::string __kernel_name;
	unsigned __kernel_id;
	utils::Dim3<int> __grid_dim;
	utils::Dim3<int> __block_dim;

	// Modifiers
	void ModifyKernelName(std::string s) { this->__kernel_name = s; }
	void ModifyKernelID(unsigned id) { this->__kernel_id = id; }
	void ModifyGridDim(utils::Dim3<int> dim) { this->__grid_dim = dim; }
	void ModifyBlockDim(utils::Dim3<int> dim) { this->__block_dim = dim; }
};

std::ostream & operator<< (std::ostream & os, const KernelInfo_t & info) {
	std::cout << "[Kernel Info]: " << std::endl;
	std::cout << "Name: " << info.__kernel_name << std::endl;
	std::cout << "ID: " << info.__kernel_id << std::endl;
	std::cout << "GridDim: " << info.__grid_dim << std::endl;
	std::cout << "BlockDim: " << info.__block_dim << std::endl;
	return os;
}

class TraceParser_t {
public: 
	explicit TraceParser_t(const std::string & s) : __fn(s) {
		std::ifstream ifs(s);
		__ifs = std::move(ifs);
		if(!__ifs.is_open()) {
			throw std::runtime_error("[TraceParser_t] Runtime error: cannot open trace file.");
		}
        __eof = false;
	}
	~TraceParser_t() {
		try {
            __ifs.close();
        } catch (const std::exception & e) {
		    std::cerr << "[~TraceParser_t] " << e.what() << std::endl;
        }
	}

    void Reset(const std::string & s) {
        std::ifstream ifs(s);
        this->__ifs = std::move(ifs);
		if(!__ifs.is_open()) {
			throw std::invalid_argument("[TraceParser_t] Error: Cannot open trace file.");
		}
        __eof = false;
    }
private:
	std::ifstream __ifs; // Input file stream
	std::string __fn; // Trace file name
	KernelInfo_t __kernel_info; // Kernel information

    utils::Dim3<int> __cur_tb; // current TB ID
    unsigned __cur_warp; // current warp ID
    TraceInst __cur_inst; // current instruction

    bool __eof; // EOF

public:
    bool IsEOF() { return this->__eof; }

    // string -> bool
    bool IsRegOperand(const std::string & tok) {
        if (tok.size() < 2) return false;
        if (tok[0] != 'R') return false;
        try {
            std::stoi(tok.substr(1));
            return true;
        } catch (const std::exception &) {
            return false;
        }
    }

    // std::string -> RegOperand
    regOps::RegOperand ParseReg(const std::string & tok, const regOps::RegOperandType regType) {
        if(tok.size() < 2) throw std::invalid_argument("[ParseReg] Invalid input string length.");
        if(tok[0] != 'R') throw std::invalid_argument("[ParseReg] Invalid register format.");
        unsigned index;
        try {
            index = std::stoi(tok.substr(1));
        } catch (const std::invalid_argument & e) {
            throw std::invalid_argument("[ParseReg] Invalid register index.");
        }
        regOps::RegOperand reg(regType, index);
        return reg;
    } 
    
    // vector<string> -> bool
    bool IsTraceInst(const std::vector<std::string> & toks) {
        if(toks.size() < 4) return false;
        std::string strPC = toks[0];
        if(strPC.length() != 4) return false;
        for(auto c : strPC) {
            if (!std::isxdigit(c)) return false;
        }

        // TODO: Make it more strict    
        return true;
    }

    // string -> InstOpcode
    InstOpcode ParseOpcode(const std::string & tok) {
        size_t dotIndex = tok.find('.');
        if (dotIndex != std::string::npos)
            return MapString2Opcode(tok.substr(0, dotIndex));
        else 
            return MapString2Opcode(tok); 
    }

    // vector<RegOperand> -> void
    void ExtendMMARegList(std::vector<regOps::RegOperand> & regs) {
        if(regs.size() < 4) throw std::invalid_argument("[ExtendMMARegList] Invalid argument.");
        auto reg_dst = regs[0];
        auto reg_src_a = regs[1];
        auto reg_src_b = regs[2];
        auto reg_src_c = regs[3];
        
        regs.clear();
        regs.push_back(reg_dst);
        regs.push_back(regOps::RegOperand(reg_dst.__reg_type, reg_dst.__reg_index + 1));
        regs.push_back(regOps::RegOperand(reg_dst.__reg_type, reg_dst.__reg_index + 2));
        regs.push_back(regOps::RegOperand(reg_dst.__reg_type, reg_dst.__reg_index + 3));

        regs.push_back(reg_src_a);
        regs.push_back(regOps::RegOperand(reg_src_a.__reg_type, reg_src_a.__reg_index + 1));
        regs.push_back(regOps::RegOperand(reg_src_a.__reg_type, reg_src_a.__reg_index + 2));
        regs.push_back(regOps::RegOperand(reg_src_a.__reg_type, reg_src_a.__reg_index + 3));

        regs.push_back(reg_src_b);
        regs.push_back(regOps::RegOperand(reg_src_b.__reg_type, reg_src_b.__reg_index + 1));
        
        regs.push_back(reg_src_c);
        regs.push_back(regOps::RegOperand(reg_src_c.__reg_type, reg_src_c.__reg_index + 1));
        regs.push_back(regOps::RegOperand(reg_src_c.__reg_type, reg_src_c.__reg_index + 2));
        regs.push_back(regOps::RegOperand(reg_src_c.__reg_type, reg_src_c.__reg_index + 3));
    }

    // vector<string> -> TraceInst
    TraceInst ParseInst(const std::vector<std::string> & toks) {
        if(toks.size() < 6) 
            throw std::invalid_argument("[ParseInst] Invalid input length."); 
        
        // Start to parse the instruction
        InstOpcode opcode; // Instruction Opcode
        std::vector<regOps::RegOperand> regs;
        
        // There are basically 2 types of instructions, [dst_num] = 0 / 1 
        if(toks[2] == "1") {
            regs.push_back(ParseReg(toks[3], regOps::DST)); // Push the DST reg to the list
            opcode = this->ParseOpcode(toks[4]);
            for(int i = 5; i < toks.size(); i++) {
                if(IsRegOperand(toks[i]))
                    regs.push_back(this->ParseReg(toks[i], regOps::SRC));
                else 
                    continue;
            }
            if(opcode == OP_HMMA || opcode == OP_BMMA || opcode == OP_IMMA) 
                this->ExtendMMARegList(regs);
            TraceInst traceInst(toks[0], this->__cur_tb, this->__cur_warp, opcode, regs);
            return traceInst;
        }
        else if(toks[2] == "0") {
            opcode = this->ParseOpcode(toks[3]);
            for(int i = 4; i < toks.size(); i++) {
                if(IsRegOperand(toks[i]))
                    regs.push_back(this->ParseReg(toks[i], regOps::SRC));
                else
                    continue;
            }
            TraceInst traceInst(toks[0], this->__cur_tb, this->__cur_warp, opcode, regs);
            return traceInst;
        }
        else {
            throw std::invalid_argument("[ParseInst] Invalid input tokens.");
        }
    }

	// Parse Trace
	TraceInst ParseTrace() {
        if(this->IsEOF()) {
            TraceInst voidInst;
            return voidInst;
        }

		std::string instStr;
		std::getline(__ifs, instStr); // Get a line from ifstream
        if(__ifs.eof()) {
            // std::cout << "[ParseTrace] EOF (SUCCESS)." << std::endl;
            this->__eof = true;
            TraceInst voidInst;
            return voidInst;
        }

        // Parse the line, break into tokens
		std::stringstream ss(instStr);
		std::string tokStr;
		std::vector<std::string> tokStrs; // SubStrings
		while(std::getline(ss, tokStr, ' ')) {
			tokStrs.push_back(tokStr);
		}

		// Parse the tokens
		if(tokStrs.empty()) 
            return this->ParseTrace();
		else if(tokStrs[0] == "-kernel" && tokStrs[1] == "name" && tokStrs.size() >= 4) {
			this->__kernel_info.ModifyKernelName(tokStrs[3]);
            return this->ParseTrace();
		}
		else if(tokStrs[0] == "-kernel" && tokStrs[1] == "id" && tokStrs.size() >= 4) {
            try {
			    this->__kernel_info.ModifyKernelID(std::stoi(tokStrs[3]));
            } catch (const std::invalid_argument & e) {
                throw std::runtime_error("[ParseLine] Invalid kernel ID.");
            }
            return this->ParseTrace();
		}
        else if(tokStrs[0] == "thread" && tokStrs[1] == "block" && tokStrs.size() == 4) {
            // Update thread block ID
            int tb_x, tb_y, tb_z;
            try {
                size_t pos_y = tokStrs[3].find(',', 0);
                size_t pos_z = tokStrs[3].find(',', pos_y + 1);
                
                tb_x = std::stoi(tokStrs[3].substr(0, pos_y)); // Get position X
                tb_y = std::stoi(tokStrs[3].substr(pos_y + 1, pos_z - pos_y - 1)); // Get position Y
                tb_z = std::stoi(tokStrs[3].substr(pos_z + 1, tokStrs[3].size() - pos_z - 1)); // Get position Z
                // utils::Dim3<int> dim_tb(pos_x, pos_y, pos_z); 
            } catch (const std::exception & e) { 
                throw std::runtime_error("[ParseLine] Invalid TB ID.");
            }
            this->__cur_tb.x = tb_x;
            this->__cur_tb.y = tb_y;
            this->__cur_tb.z = tb_z; 
            return this->ParseTrace();
        } 
        else if(tokStrs[0] == "warp" && tokStrs.size() == 3) {
            // Update warp ID
            int w_id_int;
            try {
                w_id_int = std::stoi(tokStrs[2]);
            } catch (const std::exception & e) {
                throw std::runtime_error("[ParseLine] Invalid warp ID.");
            }
            if(w_id_int < 0) throw std::runtime_error("[ParseLine] Invalid warp ID (negative).");
            this->__cur_warp = static_cast<unsigned>(w_id_int);
            return this->ParseTrace(); 
        }
        else if(IsTraceInst(tokStrs)) {
            this->__cur_inst = this->ParseInst(tokStrs);
            return this->__cur_inst;
        }
        else 
            return this->ParseTrace();
	}

    void PrintCurInst() {
        std::cout << this->__cur_inst << std::endl;
    }
};

#endif
