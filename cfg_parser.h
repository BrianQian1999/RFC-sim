// Global Configuration Parser
// Qiran QIAN, <qiranq@kth.se>

#ifndef CFG_PARSER_H
#define CFG_PARSER_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <stdexcept>
#include <unordered_map>

// namespace cfg
namespace cfg {
    
    // enum UARCH
    // Currently, only Turing uArch is considered
    enum ARCH {
        TURING = 0,
        VOLTA,
        AMPERE
    };

    // enum EVICT_PLCY
    enum EVICT_PLCY {
        LRU = 0,
        FIFO
    };

    // Pretty Printer
    std::string ARCH2Str(const ARCH & arch) {
        std::unordered_map<ARCH, std::string> hashMap = {
            std::pair<ARCH, std::string>(VOLTA, "VOLTA"),
            std::pair<ARCH, std::string>(TURING, "TURING"),
            std::pair<ARCH, std::string>(AMPERE, "AMPERE")
        };
            
        auto it = hashMap.find(arch);
        if(it == hashMap.end()) throw std::invalid_argument("[ARCH2Str] Invalid input arch.");
        return it->second;
    } 

    std::string EVICT_PLCY2Str(const EVICT_PLCY & plcy) {
        std::unordered_map<EVICT_PLCY, std::string> hashMap = {
            std::pair<EVICT_PLCY, std::string>(LRU, "LRU"),
            std::pair<EVICT_PLCY, std::string>(FIFO, "FIFO")
        };

        auto it = hashMap.find(plcy);
        if(it == hashMap.end()) throw std::invalid_argument("[EVICT_PLCY2Str] Invalid input policy.");
        return it->second;
    }
};

// struct GlobalCfg
struct GlobalCfg {
    cfg::ARCH archCfg;
    cfg::EVICT_PLCY evictPlcyCfg;
    unsigned numEntryCfg;
    
    GlobalCfg() {}

    void PrintCfg() {
        std::cout << "[GlobalCfg.PrintCfg] Arch = " << cfg::ARCH2Str(this->archCfg) << "\t" << "Eviction policy = "
            << cfg::EVICT_PLCY2Str(this->evictPlcyCfg) << "\t" << "Number of entries per warp = " << this->numEntryCfg << std::endl;
    }
};

class CfgParser {
private:
    std::ifstream __ifs; // Input file stream
    std::shared_ptr<GlobalCfg> __cfg_ptr; // RFC configuration pointer

public: 
    explicit CfgParser(const std::string & cfg_file, const std::shared_ptr<GlobalCfg> & cfg_ptr) : __cfg_ptr(cfg_ptr) {
        std::ifstream ifs(cfg_file);
		this->__ifs = std::move(ifs);
        if(!__ifs.is_open()) {
            throw std::runtime_error("[CfgParser] Runtime error: cannot open config file.");
        }
    }
    ~CfgParser() {
        try {
            __ifs.close();
        } catch (const std::exception & e) {
            std::cerr << "[~CfgParser] " << e.what() << std::endl;
        }
    }

    void PrintCfg() {
        this->__cfg_ptr->PrintCfg();
    }

    void ParseCfg() {
        std::string lineStr;
        while(std::getline(__ifs, lineStr)) {
            this->ParseLine(lineStr);
        }
    }

    void ParseLine(const std::string & lineStr) {
        std::stringstream ss(lineStr);
        std::string tokStr;
        std::vector<std::string> tokStrList; // Sub-strings
        while(std::getline(ss, tokStr, ' ')) {
            tokStrList.push_back(tokStr);
        } 

        if(tokStrList.size() >= 2 && tokStrList[0] == "-arch") {
            if(tokStrList[1] == "turing")
                this->__cfg_ptr->archCfg = cfg::TURING;
            else {
                std::cout << "[CfgParser] Unsupported arch detected. Use TURING as default." << std::endl;
            }
        }
        else if(tokStrList.size() >= 2 && tokStrList[0] == "-eviction_policy") {
            if(tokStrList[1] == "lru") 
                this->__cfg_ptr->evictPlcyCfg = cfg::LRU;
            else if(tokStrList[1] == "fifo")
                this->__cfg_ptr->evictPlcyCfg = cfg::FIFO;
            else
                std::cout << "[CfgParser] Unsupported eviction policy. Use LRU as default." << std::endl;
        }
        else if(tokStrList.size() >= 2 && tokStrList[0] == "-n_entry_per_warp") {
            try {
                this->__cfg_ptr->numEntryCfg = std::stoi(tokStrList[1]);
            } catch (const std::exception & e) {
                std::cerr << "[CfgParser] Invalid -n_entry_per_warp" << e.what() << std::endl;                
            }
        }
    }

};

#endif
