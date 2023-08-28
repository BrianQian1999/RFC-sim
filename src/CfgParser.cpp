#include "CfgParser.h"

namespace cfg {

    GlobalCfg::GlobalCfg(Arch arch, AllocPlcy alloc, ReplPlcy repl, EvictPlcy evict) 
        : arch(arch), allocPlcy(alloc), replPlcy(repl), evictPlcy(evict) {
    }

    CfgParser::CfgParser(const std::string & filePath, const std::shared_ptr<GlobalCfg> & cfg) : cfg(cfg) {
    	try { 
	   		this->yamlNode = YAML::LoadFile(filePath);
		} catch (YAML::ParserException & e) {
			throw std::runtime_error("Runtime error: failed to open yaml file.");
		}
    } 

    void CfgParser::parse() {
        try {
            cfg->arch = static_cast<Arch>(yamlNode["arch"].as<int>());
            cfg->allocPlcy = static_cast<AllocPlcy>(yamlNode["policy"][0]["alloc"].as<int>());
            cfg->replPlcy = static_cast<ReplPlcy>(yamlNode["policy"][1]["repl"].as<int>());
            cfg->evictPlcy = static_cast<EvictPlcy>(yamlNode["policy"][2]["evict"].as<int>());

            cfg->engyMdl.rfcRdEngy = yamlNode["energy_model"][0]["rfc.r"].as<float>();
            cfg->engyMdl.rfcWrEngy = yamlNode["energy_model"][1]["rfc.w"].as<float>();
            cfg->engyMdl.mrfRdEngy = yamlNode["energy_model"][2]["mrf.r"].as<float>();
            cfg->engyMdl.mrfWrEngy = yamlNode["energy_model"][3]["mrf.w"].as<float>();
        } catch (std::exception & e) {
            std::cerr << "yaml parsing error: " << e.what() << std::endl;
        }
        switch(cfg->allocPlcy) {
            case(AllocPlcy::fullCplAlloc):
                cfg->assoc = 2;
                break;
            case(AllocPlcy::readAlloc):
                cfg->assoc = 8;
                break;
            case(AllocPlcy::writeAlloc):
                cfg->assoc = 8;
                break;
            default:
                cfg->assoc = 8;
                break;
        }
    } 

    void CfgParser::print() const {
        std::cout << *cfg;
    } 
};
