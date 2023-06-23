/**
 * Turing SASS trace parser, stdin -> struct TraceInst_tr
 * author@ Qiran Qian, <qiranq@kth.se>
 **/

#pragma once

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cctype>

#include "trace_inst.h"
#include "trace_opcode.h"
#include "utils.h"
#include "reuse_info.h"

struct KernelInfo_t {
	KernelInfo_t() {}
	KernelInfo_t(std::string n, unsigned id, utils::Dim3<int> gridDim, utils::Dim3<int> blockDim)
		: kernelSym(n), kernelID(id), gridDim(gridDim), blockDim(blockDim) {}
	
	// Kernel Infos
	std::string kernelSym;
	unsigned kernelID;
	utils::Dim3<int> gridDim;
	utils::Dim3<int> blockDim;

	// Modifiers
	void ModifyKernelName(std::string s) { this->kernelSym = s; }
	void ModifyKernelID(unsigned id) { this->kernelID = id; }
	void ModifyGridDim(utils::Dim3<int> dim) { this->gridDim = dim; }
	void ModifyBlockDim(utils::Dim3<int> dim) { this->blockDim = dim; }
};

std::ostream & operator<< (std::ostream & os, const KernelInfo_t & info) {
	std::cout << "[Kernel Info]: " << std::endl;
	std::cout << "Name: " << info.kernelSym << std::endl;
	std::cout << "ID: " << info.kernelID << std::endl;
	std::cout << "GridDim: " << info.gridDim << std::endl;
	std::cout << "BlockDim: " << info.blockDim << std::endl;
	return os;
}

/* 
 * @class TraceParser_t
 * Top-level of the trace parser
 */
class TraceParser_t {
public: 
	explicit TraceParser_t(const std::string & s, const std::string & sass_s) : _fn(s) {
		std::ifstream ifs(s);
		_ifs = std::move(ifs);
		if(!_ifs.is_open()) {
			throw std::runtime_error("[TraceParser_t] Runtime error: cannot open trace file.");
		}
        _eof = false;

        // Initialize SASS trace
		try {
        	_reuse_info_p = std::make_shared<ReuseInfo_t>(sass_s);
        	_reuse_info_p->ParseSASS();
		} catch (std::exception & e) {
			std::cerr << "[TraceParser_t]: " << e.what() << std::endl;
		}
	}
	~TraceParser_t() {
		try {
            _ifs.close();
        } catch (const std::exception & e) {
		    std::cerr << "[~TraceParser_t] " << e.what() << std::endl;
        }
	}

    void Reset(const std::string & s) {
        std::ifstream ifs(s);
        this->_ifs = std::move(ifs);
		if(!_ifs.is_open()) {
			throw std::invalid_argument("[TraceParser_t] Error: Cannot open trace file.");
		}
        _eof = false;
    }

private:
	std::ifstream _ifs; // Input file stream
	std::string _fn; // Trace file name
	KernelInfo_t _kernel_info; // Kernel information

    std::shared_ptr<ReuseInfo_t> _reuse_info_p;

    utils::Dim3<int> _cur_tb; // current TB ID
    unsigned _cur_warp; // current warp ID
    TraceInst_t _cur_inst; // current instruction

    bool _eof; // EOF

public:
    bool IsEOF() { return this->_eof; }

    // string -> bool
    bool IsRegOperand(const std::string & tok) {
        if (tok.size() < 2 || tok[0] != 'R') 
            return false;
        
        try {
            std::stoi(tok.substr(1));
            return true;
        } catch (const std::exception & e) {
            return false;
        }
    }

    /**
     * Is address operand
     * @param tok
     **/
    bool IsAddrOperand(const std::string & tok) {
        return (tok.size() > 3 && tok.substr(0, 2) == "0x"); 
    }

    /**
     * Parse register
     * @param tok
     * @param regType
     **/
    regOps::RegOperand_t ParseReg(const std::string & tok, const regOps::RegOperandType_e regType) {
        if(tok.size() < 2) 
            throw std::invalid_argument("[ParseReg] Invalid input string length.");
        if(tok[0] != 'R') 
            throw std::invalid_argument("[ParseReg] Invalid register format.");
       
        // Address operand
        if(regType == regOps::ADDR) 
            return regOps::RegOperand_t(regOps::ADDR);

        // Register operand
        unsigned index; 
        try {
            index = std::stoi(tok.substr(1));
        } catch (const std::invalid_argument & e) {
            throw std::invalid_argument("[ParseReg] Invalid register index.");
        }
        regOps::RegOperand_t reg(regType, index);
        return reg;
    } 
    
    // vector<string> -> bool
    bool IsTraceInst_t(const std::vector<std::string> & toks) {
        if(toks.size() < 4) return false;
        std::string strPC = toks[0];
        if(strPC.length() != 4) return false;
        for(auto c : strPC) {
            if (!std::isxdigit(c)) return false;
        }

        // TODO: Make it more strict    
        return true;
    }

    /*
     * Parse opcode
     * @param tok
     */
    InstOpcode ParseOpcode(const std::string & tok) {
        size_t dotIndex = tok.find('.');
        if (dotIndex != std::string::npos)
            return MapString2Opcode(tok.substr(0, dotIndex));
        else 
            return MapString2Opcode(tok); 
    }

    /* For MMA instructions, the registers to be accessed are implicitly expressed by the instruction
     * Extend the register list for MMA instructions 
     * @param regs
     */
    void ExtendMMARegList(std::vector<regOps::RegOperand_t> & regs) {
        if(regs.size() < 4) throw std::invalid_argument("[ExtendMMARegList] Invalid argument.");
        auto reg_dst = regs[0];
        auto reg_src_a = regs[1];
        auto reg_src_b = regs[2];
        auto reg_src_c = regs[3];
        
        regs.clear();
        regs.push_back(reg_dst);
        regs.push_back(regOps::RegOperand_t(reg_dst.regType, reg_dst.regIndex + 1));
        regs.push_back(regOps::RegOperand_t(reg_dst.regType, reg_dst.regIndex + 2));
        regs.push_back(regOps::RegOperand_t(reg_dst.regType, reg_dst.regIndex + 3));

        // A is a half-precision Matrix
        regs.push_back(reg_src_a);
        regs.push_back(regOps::RegOperand_t(reg_src_a.regType, reg_src_a.regIndex + 1));

        // B is a half-precision Matrix
        regs.push_back(reg_src_b);
        
        regs.push_back(reg_src_c);
        regs.push_back(regOps::RegOperand_t(reg_src_c.regType, reg_src_c.regIndex + 1));
        regs.push_back(regOps::RegOperand_t(reg_src_c.regType, reg_src_c.regIndex + 2));
        regs.push_back(regOps::RegOperand_t(reg_src_c.regType, reg_src_c.regIndex + 3));
    }

    /*
     * Parse instruction 
     * @param toks
     */
    TraceInst_t ParseInst(const std::vector<std::string> & toks) {
        if(toks.size() < 6) 
            throw std::invalid_argument("[ParseInst] Invalid input length."); 
        
        unsigned pc;
        auto & pc_s = toks[0];
        std::stringstream pc_ss(pc_s);
        pc_ss >> std::hex >> pc; 

        InstOpcode opcode;
        std::vector<regOps::RegOperand_t> regs;
       
        // Generate register list 
        if(toks[2] == "1") {
            regs.push_back(ParseReg(toks[3], regOps::DST));
            opcode = this->ParseOpcode(toks[4]); // Parse opcode
            for(int i = 5; i < toks.size(); i++) {
                if(IsRegOperand(toks[i]))
                    regs.push_back(this->ParseReg(toks[i], regOps::SRC));
                else if(IsAddrOperand(toks[i]))
                    regs.push_back(this->ParseReg(toks[i], regOps::ADDR));
                else
                    continue;
            }

            // Take special care to MMA instructions
            if(opcode == OP_HMMA || opcode == OP_BMMA || opcode == OP_IMMA) {
                this->ExtendMMARegList(regs);
            }

            // Look up reuse flags
            auto flags = this->_reuse_info_p->reuse_info_tab.at(pc);

            // Construct TraceInst_t
            return TraceInst_t(pc, this->_cur_tb, this->_cur_warp, opcode, regs, flags);
        }
        else if(toks[2] == "0") {
            opcode = this->ParseOpcode(toks[3]); // Parse Opcode
            for(int i = 4; i < toks.size(); i++) {
                if(IsRegOperand(toks[i]))
                    regs.push_back(this->ParseReg(toks[i], regOps::SRC));
                else if(IsAddrOperand(toks[i])) 
                    regs.push_back(this->ParseReg(toks[i], regOps::ADDR));
                else
                    continue;
            }
            // Look up reuse flags
            auto flags = this->_reuse_info_p->reuse_info_tab.at(pc);

            // Construct TraceInst_t
            return TraceInst_t(pc, this->_cur_tb, this->_cur_warp, opcode, regs, flags);
        }
        else
            throw std::invalid_argument("[ParseInst] Invalid input tokens.");
    }

	/* 
     * Top-level function of TraceParser
     * Parse the STDIN and return the next instruction
     */
	TraceInst_t ParseTrace() {
        if(this->IsEOF()) {
            TraceInst_t voidInst;
            return voidInst;
        }
		
        std::string instStr;
		std::getline(_ifs, instStr);

        if(_ifs.eof()) {
            this->_eof = true;
            return TraceInst_t();
        }

        // Generate string tokens
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
			this->_kernel_info.ModifyKernelName(tokStrs[3]);
            return this->ParseTrace();
		}
		else if(tokStrs[0] == "-kernel" && tokStrs[1] == "id" && tokStrs.size() >= 4) {
            try {
			    this->_kernel_info.ModifyKernelID(std::stoi(tokStrs[3]));
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
                throw std::runtime_error("[ParseLine] Invalid thread block ID.");
            }
            this->_cur_tb.x = tb_x;
            this->_cur_tb.y = tb_y;
            this->_cur_tb.z = tb_z; 
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
            this->_cur_warp = static_cast<unsigned>(w_id_int);
            return this->ParseTrace(); 
        }
        else if(IsTraceInst_t(tokStrs)) {
            this->_cur_inst = this->ParseInst(tokStrs);
            return this->_cur_inst;
        }
        else 
            return this->ParseTrace();
	}

    void PrintCurInst() {
        std::cout << this->_cur_inst << std::endl;
    }
};

