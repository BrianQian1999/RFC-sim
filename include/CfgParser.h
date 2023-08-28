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
    enum class Arch {
        sm70, 
        sm75 = 0,
        sm80
    };

    // Allocation policy
    enum class AllocPlcy {
        fullCplAlloc = 0,
        readAlloc,
        writeAlloc,
		customAlloc
    };

	// Replacement policy
    enum class ReplPlcy {
        lru = 0,
        fifo
    };

	// Eviction Policy
	enum class EvictPlcy {
		writeBack = 0, // if dst reg hit, immediately write a copy in MRF
		writeThrough // write data back to MRF only when dirty data is evicted
	};

    // Arch -> std::string
    inline const std::unordered_map<Arch, std::string> arch2StrTab {
        std::pair<Arch, std::string>(Arch::sm70, "sm70 (Volta)"),
        std::pair<Arch, std::string>(Arch::sm75, "sm75 (Turing)"),
        std::pair<Arch, std::string>(Arch::sm80, "sm80 (Ampere)")
    };

    inline std::ostream & operator<<(std::ostream & os, const Arch & arch) {
        auto it = arch2StrTab.find(arch);
        if(it == arch2StrTab.end()) 
            throw std::invalid_argument("Invalid GPU architecture.\n");
        os << it->second;
        return os;
    }

    inline const std::unordered_map<AllocPlcy, std::string> alloc2StrTab {
        std::pair<AllocPlcy, std::string>(AllocPlcy::fullCplAlloc, "fullCplAlloc"),
        std::pair<AllocPlcy, std::string>(AllocPlcy::readAlloc, "readAlloc"),
        std::pair<AllocPlcy, std::string>(AllocPlcy::writeAlloc, "writeAlloc"),
		std::pair<AllocPlcy, std::string>(AllocPlcy::customAlloc, "customAlloc")
    };

    inline std::ostream & operator<<(std::ostream & os, const AllocPlcy & plcy) {
        auto it = alloc2StrTab.find(plcy);
        if(it == alloc2StrTab.end()) 
            throw std::invalid_argument("Invalid allocation policy.");
        os << it->second;
        return os;
    }

	inline const std::unordered_map<ReplPlcy, std::string> repl2StrTab {
		std::pair<ReplPlcy, std::string>(ReplPlcy::lru, "lru"),
		std::pair<ReplPlcy, std::string>(ReplPlcy::fifo, "fifo")
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
        float rfcRdEngy;
        float rfcWrEngy;
        float mrfRdEngy;
        float mrfWrEngy;
    };

    inline std::ostream & operator<<(std::ostream & os, const EngyMdl & engyMdl) {
        std::cout << "(RFC.r | RFC.w | MRF.r | MRF.w) -> (";
        std::cout << engyMdl.rfcRdEngy << " | " << engyMdl.rfcWrEngy << " | " << engyMdl.mrfRdEngy
                  << " | " << engyMdl.mrfWrEngy << ")" << std::endl; 
        return os;
    } 

    struct GlobalCfg {
        Arch arch;
        AllocPlcy allocPlcy;
        ReplPlcy replPlcy;
        EvictPlcy evictPlcy;
        uint32_t assoc;

        EngyMdl engyMdl;

        GlobalCfg() {}
        GlobalCfg(Arch, AllocPlcy, ReplPlcy, EvictPlcy);
    };

    inline std::ostream & operator<<(std::ostream & os, const GlobalCfg & cfg) {
        os << "[Cfg]: (arch, alloc, repl, evict, assoc, energy model) -> ("
           << cfg.arch << ", " << cfg.allocPlcy << ", " << cfg.replPlcy << ", " << cfg.evictPlcy << ", " 
           << cfg.assoc << ", " << cfg.engyMdl << ")\n";
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
