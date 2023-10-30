#include "CfgParser.h"

namespace cfg {

    GlobalCfg::GlobalCfg(SmArch arch, AllocPlcy alloc, ReplPlcy repl, EvictPlcy evict) 
        : arch(arch), alloc(alloc), repl(repl), ev(evict) {
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
            cfg->arch = static_cast<SmArch>(yamlNode["arch"].as<int>());
            cfg->alloc = static_cast<AllocPlcy>(yamlNode["policy"][0]["alloc"].as<int>());
            cfg->wl = cfg->alloc != AllocPlcy::lookAheadAlloc ? 0 : yamlNode["window_len"].as<int>();
            cfg->repl = static_cast<ReplPlcy>(yamlNode["policy"][1]["repl"].as<int>());
            cfg->ev = static_cast<EvictPlcy>(yamlNode["policy"][2]["evict"].as<int>());
            cfg->dMap = static_cast<DestMap>(yamlNode["policy"][3]["dest_map"].as<int>());

            cfg->nBlk = yamlNode["n_block"].as<int>();
            cfg->assoc = yamlNode["assoc"].as<int>();
            cfg->assoc = cfg->assoc == 0 ? cfg->nBlk : cfg->assoc;
            
            cfg->nDW = yamlNode["n_dw"].as<int>();
            cfg->bw = yamlNode["bitwidth"].as<int>();


            cfg->eMdl.eRfcRd = yamlNode["energy_model"][0]["rfc.r"].as<float>();
            cfg->eMdl.eRfcWr = yamlNode["energy_model"][1]["rfc.w"].as<float>();
            cfg->eMdl.eMrfRd = yamlNode["energy_model"][2]["mrf.r"].as<float>();
            cfg->eMdl.eMrfWr = yamlNode["energy_model"][3]["mrf.w"].as<float>();
        } catch (std::exception & e) {
            std::cerr << e.what() << "yaml parsing error: " << std::endl;
        }

		// Config parameters checking
        if (cfg->bw % 32 != 0) 
            throw std::invalid_argument("Invalid input: bitwidth % 32 must be 0.\n");
    } 

    void CfgParser::print() const {
        std::cout << *cfg;
    } 
};
