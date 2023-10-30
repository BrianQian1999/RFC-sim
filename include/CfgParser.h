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

    // default arch: sm75 (Turing)
    enum class SmArch {
        sm70, 
        sm75,
        sm80
    };

    // Allocation policy
    enum class AllocPlcy {
        readAlloc=0,
        writeAlloc,
        cplAidedAlloc,
        lookAheadAlloc
    };

	// Replacement policy
    enum class ReplPlcy {
        lru = 0,
        fifo
    };

	// Eviction Policy
	enum class EvictPlcy {
		writeBack = 0,
		writeThrough
	};

    enum class DestMap {
        ln, 
        itl
    };

    // SmArch -> std::string
    inline const std::unordered_map<SmArch, std::string> arch2StrTab {
        std::pair<SmArch, std::string>(SmArch::sm70, "sm70"),
        std::pair<SmArch, std::string>(SmArch::sm75, "sm75"),
        std::pair<SmArch, std::string>(SmArch::sm80, "sm80")
    };

    inline std::ostream & operator<<(std::ostream & os, const SmArch & arch) {
        auto it = arch2StrTab.find(arch);
        if(it == arch2StrTab.end()) 
            throw std::invalid_argument("Invalid GPU architecture.\n");
        os << it->second;
        return os;
    }

    inline const std::unordered_map<AllocPlcy, std::string> alloc2StrTab {
        std::pair<AllocPlcy, std::string>(AllocPlcy::readAlloc, "read-allocate"),
        std::pair<AllocPlcy, std::string>(AllocPlcy::writeAlloc, "write-allocate"),
        std::pair<AllocPlcy, std::string>(AllocPlcy::cplAidedAlloc, "compiler-aided allocate"),
        std::pair<AllocPlcy, std::string>(AllocPlcy::lookAheadAlloc, "compiler-aided allocate with look-ahead")
    };

    inline std::ostream & operator<<(std::ostream & os, const AllocPlcy & plcy) {
        auto it = alloc2StrTab.find(plcy);
        if(it == alloc2StrTab.end()) 
            throw std::invalid_argument("Invalid allocation policy.");
        os << it->second;
        return os;
    }

	inline const std::unordered_map<ReplPlcy, std::string> repl2StrTab {
		std::pair<ReplPlcy, std::string>(ReplPlcy::lru, "LRU"),
		std::pair<ReplPlcy, std::string>(ReplPlcy::fifo, "FIFO")
    };

    inline std::ostream & operator<<(std::ostream & os, const ReplPlcy & plcy) {
		auto it = repl2StrTab.find(plcy);
		if(it == repl2StrTab.end()) 
            throw std::invalid_argument("Invalid replacement policy.\n");
		os << it->second;
        return os;
	}
    
    inline const std::unordered_map<EvictPlcy, std::string> evict2StrTab {
        std::pair<EvictPlcy, std::string>(EvictPlcy::writeBack, "write-back"),
        std::pair<EvictPlcy, std::string>(EvictPlcy::writeThrough, "write-through")
    };
	
    inline std::ostream & operator<<(std::ostream & os, const EvictPlcy & plcy) {
        auto it = evict2StrTab.find(plcy);
        if(it == evict2StrTab.end()) 
            throw std::invalid_argument("Invalid eviction policy.\n");
        os << it->second;
        return os;
    }

    // Energy model
    struct EngyMdl {
        float eRfcRd;
        float eRfcWr;
        float eMrfRd;
        float eMrfWr;
    };

    inline std::ostream & operator<<(std::ostream & os, const EngyMdl & eMdl) {
        std::cout << "(RFC.r, RFC.w, MRF.r, MRF.w) -> (";
        std::cout << eMdl.eRfcRd << ", " 
                  << eMdl.eRfcWr << ", " 
                  << eMdl.eMrfRd << ", " 
                  << eMdl.eMrfWr << ")\n";
        return os;
    } 

    struct GlobalCfg {
        SmArch arch;
        AllocPlcy alloc;
        ReplPlcy repl;
        EvictPlcy ev;
        DestMap dMap;

        
        uint32_t assoc;
        uint32_t nBlk;
        uint32_t nDW;
        uint32_t bw;

        uint32_t wl; // window length

        EngyMdl eMdl;

        GlobalCfg() {}
        GlobalCfg(SmArch, AllocPlcy, ReplPlcy, EvictPlcy);
    };

    inline std::ostream & operator<<(std::ostream & os, const GlobalCfg & cfg) {
        os << "[RFC Config]:\n\t";
        os << "<GPU Arch>:                          " << cfg.arch << "\n\t";
        os << "<Allocation>:                        " << cfg.alloc << "\n\t";
        os << "<Replacement>:                       " << cfg.repl << "\n\t";
        os << "<Eviction>:                          " << cfg.ev << "\n\t";
        os << "<Look-ahead window (optional)>:      " << cfg.wl << "\n\t";
        os << "<Associativity>:                     " << cfg.assoc << "\n\t";
        os << "<# of cache blocks>:                 " << cfg.nBlk << "\n\t";
        os << "<Cache bank bitwidth>:               " << cfg.bw << "\n\t";
        os << "<Energy Model>:                      " << cfg.eMdl << std::endl;

        return os;
    }

    class CfgParser {
    private:
        YAML::Node yamlNode;
	    std::shared_ptr<GlobalCfg> cfg;
    public: 
        explicit CfgParser(const std::string&, const std::shared_ptr<GlobalCfg>&);
	    void parse();
        void print() const;
    }; 

}; // namespace cfg
