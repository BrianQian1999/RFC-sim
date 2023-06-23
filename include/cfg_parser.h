/**
 * Global Configuration Parser
 * author@Qiran QIAN, <qiranq@kth.se>
 **/

#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <stdexcept>
#include <unordered_map>

#include "yaml-cpp/yaml.h"

// namespace cfg
namespace cfg {
    // Currently, only Turing GPU is considered
    enum ARCH {
        TURING = 0,
        VOLTA,
        AMPERE
    };

    // Allocation policy
    enum ALLOC_PLCY {
        ALLOC_BASE = 0,  // allocate for both SRC/DST registers
        ALLOC_DST,        // allocate only for DST registers
		ALLOC_CUSTOM     // custom allocator, (for MMA, allocate for DST + ACC SRC)
    };

	// Replacement policy
    enum REP_PLCY {
        LRU = 0,
        FIFO
    };

	// Eviction Policy
	enum EV_PLCY {
		WR_BACK = 0,
		WR_THR
	};

    // Pretty Printer
    std::string PPrintArch(const ARCH & arch) {
        std::unordered_map<ARCH, std::string> hashMap = {
            std::pair<ARCH, std::string>(VOLTA, "VOLTA"),
            std::pair<ARCH, std::string>(TURING, "TURING"),
            std::pair<ARCH, std::string>(AMPERE, "AMPERE")
        };            
        auto it = hashMap.find(arch);
        if(it == hashMap.end()) throw std::invalid_argument("[PPrintArch] Invalid GPU architecture.");
        return it->second;
    } 

    std::string PPrintAllocPlcy(const ALLOC_PLCY & plcy) {
        std::unordered_map<ALLOC_PLCY, std::string> hashMap = {
            std::pair<ALLOC_PLCY, std::string>(ALLOC_BASE, "ALLOC_BASE"),
            std::pair<ALLOC_PLCY, std::string>(ALLOC_DST, "ALLOC_DST"),
			std::pair<ALLOC_PLCY, std::string>(ALLOC_CUSTOM, "ALLOC_CUSTOM")
        };

        auto it = hashMap.find(plcy);
        if(it == hashMap.end()) throw std::invalid_argument("[PPrintAllocPlcy] Invalid allocation policy.");
        return it->second;
    }

	std::string PPrintRePlcy(const REP_PLCY & plcy) {
		std::unordered_map<REP_PLCY, std::string> hashMap = {
			std::pair<REP_PLCY, std::string>(LRU, "LRU"),
			std::pair<REP_PLCY, std::string>(FIFO, "FIFO")
		};
		auto it = hashMap.find(plcy);
		if(it == hashMap.end()) throw std::invalid_argument("[PPrintRePlcy] Invalid replacement policy.");
		return it->second;
	}
    
	std::string PPrintEvPlcy(const EV_PLCY & plcy) {
        std::unordered_map<EV_PLCY, std::string> hashMap = {
            std::pair<EV_PLCY, std::string>(WR_BACK, "WR_BACK"),
            std::pair<EV_PLCY, std::string>(WR_THR, "WR_THR")
        };
        auto it = hashMap.find(plcy);
        if(it == hashMap.end()) throw std::invalid_argument("[PPrintEvPlcy] Invalid eviction policy.");
        return it->second;
    }
};

// struct GlobalCfg_t
struct GlobalCfg_t {
    cfg::ARCH archCfg;
    cfg::ALLOC_PLCY allocPlcyCfg;
	cfg::REP_PLCY rePlcyCfg;
    cfg::EV_PLCY evPlcyCfg;
    unsigned numEntryCfg;
    
    GlobalCfg_t() {}

    void PPrintGlobalCfg_t() {
        std::cout << "[GlobalCfg_t.PrintCfg] \n<Architecture>: " << cfg::PPrintArch(this->archCfg) << "\n"
            << "<Allocation policy>: " << cfg::PPrintAllocPlcy(this->allocPlcyCfg) << "\n"
			<< "<Replacement policy>: " << cfg::PPrintRePlcy(this->rePlcyCfg) << "\n"
            << "<Eviction policy>: " << cfg::PPrintEvPlcy(this->evPlcyCfg) << "\n" 
            << "<# of entries per warp>: " << this->numEntryCfg << std::endl;
    }
};

/*
 *    @class CfgParser_t
 *    Read from a stdin file, fill a GlobalCfg_t struct	
 */
class CfgParser_t {
private:
    YAML::Node __yaml_node;
	std::shared_ptr<GlobalCfg_t> __cfg_ptr;

public: 
    explicit CfgParser_t(const std::string & file_path, const std::shared_ptr<GlobalCfg_t> & cfg_ptr) : __cfg_ptr(cfg_ptr) {
    	try { 
	   		this->__yaml_node = YAML::LoadFile(file_path);
		} catch (YAML::ParserException & e) {
			throw std::runtime_error("[CfgParser_t.Constructor] Runtime error.");
		}
    }

    virtual ~CfgParser_t() {}
	
	void ParseCfg() {
		if(this->__yaml_node) {
			this->__cfg_ptr->archCfg = static_cast<cfg::ARCH>(this->__yaml_node["arch"].as<int>());
			this->__cfg_ptr->numEntryCfg = this->__yaml_node["n_entry"].as<unsigned>();
			this->__cfg_ptr->allocPlcyCfg = static_cast<cfg::ALLOC_PLCY>(this->__yaml_node["policy"][0]["alloc"].as<int>());
			this->__cfg_ptr->rePlcyCfg = static_cast<cfg::REP_PLCY>(this->__yaml_node["policy"][1]["replace"].as<int>());
			this->__cfg_ptr->evPlcyCfg = static_cast<cfg::EV_PLCY>(this->__yaml_node["policy"][2]["evict"].as<int>());
		}
		else {
			throw std::runtime_error("[CfgParser_t.ParseCfg] Runtime error.");	
		}
	}

    void PPrintCfg() {
        this->__cfg_ptr->PPrintGlobalCfg_t();
    }
}; 
//@namespace cfg

