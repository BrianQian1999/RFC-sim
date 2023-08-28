#include "TraceParser.h"

TraceParser::TraceParser(
    const std::string & traceFile,
    const std::shared_ptr<std::unordered_map<uint32_t, std::bitset<4>>> & reuseInfo
) : traceIfs(std::ifstream(traceFile)), reuseInfo(reuseInfo) {
    if (!traceIfs.is_open()) {
        throw std::runtime_error("Runtime error: failed to open trace file.\n");
    }
}

bool TraceParser::eof() const {
    return traceIfs.eof();
}

void TraceParser::reset(const std::string & s) {
    traceIfs = std::ifstream(s);
}

bool TraceParser::isRegOprd(const std::string & tok) const {
    if (tok.size() < 2 || tok[0] != 'R') 
        return false;
        
    try {
        std::stoi(tok.substr(1));
        return true;
    } catch (const std::exception & e) {
        std::cerr << e.what() << std::endl;
    }
    return false;
}

bool TraceParser::IsAddrOprd(const std::string & tok) const {
    return (tok.size() > 3 && tok.substr(0, 2) == "0x"); 
}

reg::RegOprd TraceParser::parseReg(const std::string & tok, reg::RegOprdT type, uint32_t pos) const {
    if(tok.size() < 2) 
        throw std::invalid_argument("Invalid input: tok.size() < 2.\n");
    if(tok[0] != 'R' && type != reg::RegOprdT::ADDR) 
        throw std::invalid_argument("Invalid input: register format.\n");
       
    if(type == reg::RegOprdT::ADDR) 
        return reg::RegOprd(reg::RegOprdT::ADDR, pos);

    uint32_t index; 
    try {
        index = std::stoi(tok.substr(1));
    } catch (const std::invalid_argument & e) {
        throw std::invalid_argument("Invalid input: register index.");
    }
    reg::RegOprd reg(type, index, pos);
    return reg;
}

bool TraceParser::isInst(const std::vector<std::string> & toks) const {
    if(toks.size() < 4) 
        return false;
    std::string strPC(toks[0]);
    if(strPC.length() != 4) 
        return false;
    for(auto c : strPC) {
        if (!std::isxdigit(c)) 
            return false;
    } 
    return true;
}

op::Opcode TraceParser::parseOpcode(const std::string & tok) const noexcept {
    size_t dotIndex = tok.find('.');
    if (dotIndex != std::string::npos)
        return op::str2op(tok.substr(0, dotIndex));
    else 
        return op::str2op(tok);
}

void TraceParser::extendHmmaRegs(std::vector<reg::RegOprd> & regs) const {
    if(regs.size() < 4) 
        throw std::invalid_argument("Invalid input.\n");
    
    auto matAReg = regs.at(0);
    auto matBReg = regs.at(1);
    auto matCReg = regs.at(2);
    auto matDReg = regs.at(3); 
    regs.clear();
    
    matAReg.regPos = 0;
    regs.push_back(matAReg);
    regs.push_back(reg::RegOprd(reg::RegOprdT::SRC, matAReg.regIndex + 1, 1));

    matBReg.regPos = 2;
    regs.push_back(matBReg);
    
    matCReg.regPos = 3;
    regs.push_back(matCReg);
    regs.push_back(reg::RegOprd(reg::RegOprdT::SRC, matCReg.regIndex + 1, 4));
    regs.push_back(reg::RegOprd(reg::RegOprdT::SRC, matCReg.regIndex + 2, 5));
    regs.push_back(reg::RegOprd(reg::RegOprdT::SRC, matCReg.regIndex + 3, 6));
    
    regs.push_back(matDReg);
    regs.push_back(reg::RegOprd(reg::RegOprdT::DST, matDReg.regIndex + 1, 0));
    regs.push_back(reg::RegOprd(reg::RegOprdT::DST, matDReg.regIndex + 2, 0));
    regs.push_back(reg::RegOprd(reg::RegOprdT::DST, matDReg.regIndex + 3, 0));
}

void TraceParser::extendImmaRegs(std::vector<reg::RegOprd> & regs) const {
    if(regs.size() < 4) 
        throw std::invalid_argument("Invalid input.\n");
    
    auto matAReg = regs.at(0);
    auto matBReg = regs.at(1);
    auto matCReg = regs.at(2);
    auto matDReg = regs.at(3); 
    regs.clear();
    
    matAReg.regPos = 0;
    regs.push_back(matAReg);
    regs.push_back(reg::RegOprd(reg::RegOprdT::SRC, matAReg.regIndex + 1, 1));

    matBReg.regPos = 2;
    regs.push_back(matBReg);
    
    matCReg.regPos = 3;
    regs.push_back(matCReg);
    regs.push_back(reg::RegOprd(reg::RegOprdT::SRC, matCReg.regIndex + 1, 4));
    regs.push_back(reg::RegOprd(reg::RegOprdT::SRC, matCReg.regIndex + 2, 5));
    regs.push_back(reg::RegOprd(reg::RegOprdT::SRC, matCReg.regIndex + 3, 6));
    
    regs.push_back(matDReg);
    regs.push_back(reg::RegOprd(reg::RegOprdT::DST, matDReg.regIndex + 1, 0));
    regs.push_back(reg::RegOprd(reg::RegOprdT::DST, matDReg.regIndex + 2, 0));
    regs.push_back(reg::RegOprd(reg::RegOprdT::DST, matDReg.regIndex + 3, 0));
}

TraceInst TraceParser::parseInst(const std::vector<std::string>& toks) {
    if (toks.size() < 6) 
        throw std::invalid_argument("Invalid input: toks.size()<6.\n");

    uint32_t pc;
    auto & pcStr = toks.at(0);
    std::stringstream pcStrStream(pcStr);
    pcStrStream >> std::hex >> pc;

    std::bitset<32> mask;
    auto & maskStr = toks.at(1);
    std::stringstream maskStrStream(maskStr);
    unsigned long mask_ui;
    maskStrStream >> std::hex >> mask_ui;
    mask = std::bitset<32> (mask_ui);

    op::Opcode opcode;
    std::bitset<4> flags = static_cast<std::bitset<4>>("0000");  
    auto it = reuseInfo->find(pc);
    if (it != reuseInfo->end())
        flags = it->second;

    std::vector<reg::RegOprd> regs;

    if (toks[2] == "1") {
        opcode = this->parseOpcode(toks.at(4));

        uint32_t curPos = 0;
        for (auto i = 5; i < toks.size(); i++) {
            auto & s = toks.at(i);
            if (isRegOprd(s)) {
                regs.push_back(parseReg(s, reg::RegOprdT::SRC, curPos));
                curPos++;
            }
            else if (IsAddrOprd(s)) {
                regs.push_back(this->parseReg(s, reg::RegOprdT::ADDR, curPos));
                curPos++;
            }
        }
        regs.push_back(parseReg(toks.at(3), reg::RegOprdT::DST, 0));
        if (opcode == op::OP_HMMA) extendHmmaRegs(regs);
        if (opcode == op::OP_IMMA) extendImmaRegs(regs);
        return TraceInst(pc, mask, blockId, wId, opcode, regs, flags);
    }
    else if (toks.at(2) == "0") {
        opcode = this->parseOpcode(toks.at(3));
        uint32_t curPos = 0;
        for (auto i = 5; i < toks.size(); i++) {
            auto & s = toks.at(i);
            if (isRegOprd(s)) {
                regs.push_back(parseReg(s, reg::RegOprdT::SRC, curPos));
                curPos++;
            }
            else if (IsAddrOprd(s)) {
                regs.push_back(this->parseReg(s, reg::RegOprdT::ADDR, curPos));
                curPos++;
            }
        }
        return TraceInst(pc, mask, blockId, wId, opcode, regs, flags);
    }
    else 
        return TraceInst(); 
}

TraceInst TraceParser::parse() {
    if (eof()) return TraceInst();

    std::string instStr;
    std::getline(traceIfs, instStr);

    std::stringstream ss(instStr);
	std::string tokStr;
	std::vector<std::string> tokStrs;
    while(std::getline(ss, tokStr, ' ')) {
		tokStrs.push_back(tokStr);
	}

    if (tokStrs.empty())
        return this->parse();
    else if(tokStrs.at(0) == "thread" && tokStrs.at(1) == "block" && tokStrs.size() == 4) {
        int tbX, tbY, tbZ;
        size_t posY = tokStrs.at(3).find(',', 0);
        size_t posZ = tokStrs.at(3).find(',', posY + 1);
            
        tbX = std::stoi(tokStrs.at(3).substr(0, posY));
        tbY = std::stoi(tokStrs.at(3).substr(posY + 1, posZ - posY - 1));
        tbZ = std::stoi(tokStrs.at(3).substr(posZ + 1, tokStrs[3].size() - posZ - 1));
        
        blockId.x = tbX;
        blockId.y = tbY;
        blockId.z = tbZ; 
       
        return parse();
    }
    else if(tokStrs.at(0) == "warp" && tokStrs.size() == 3) {
        int wIdSigned;
        wIdSigned = std::stoi(tokStrs.at(2));
        if(wIdSigned < 0) 
            throw std::runtime_error("Runtime error: negative warp ID.\n");
        wId = static_cast<unsigned>(wIdSigned);
        return parse();
    } 
    else if (isInst(tokStrs))
        return parseInst(tokStrs);
    else 
        return parse();
}
