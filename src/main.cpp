/*** RFC-sim is a analytic model for the Register File Cache used in NVIDIA GPUs
 *   For more information, please check the paper:
 *   Energy-efficient Mechanisms for Managing Thread Context in Throughput Processors (ISCA 2011)
 *   Qiran Qian, <qiranq@kth.se>
 ***/


#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <stdexcept>
#include <memory>

#include "TraceParser.h"
#include "Rfc.h"
#include "Logger.h"

#define NDEBUG

int main(int argc, char ** argv) {
    
	if(argc < 7 || std::string(argv[1]) != "-t" || std::string(argv[3]) != "-c" || std::string(argv[5]) != "-d") {
        std::cerr << "Usage: " << argv[0] << " -t <path_to_trace_dir> " 
                  << "-c <path_to_config_file> " 
				  << "-d <path_to_asm_file>" 
				  << "-o <path_to_log_file>\n";
        return 1;
    }
   
    const std::string traceDir = std::string(argv[2]);
    const std::string cfgFile = std::string(argv[4]);
    const std::string asmFile = std::string(argv[6]);
	const std::string logFile = std::string(argv[8]);

	const std::string traceListFile = traceDir + "/kernelslist.g";

	std::cout << "[RFC-sim] >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> START >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>" << std::endl;
	std::cout << "[RFC-sim] trace file dir: " << traceListFile << std::endl;
	std::cout << "[RFC-sim] Config file: " << cfgFile << std::endl;
    std::cout << "[RFC-sim] asm file: " << asmFile << std::endl;

	// list all kernels
	std::ifstream traceListIf(traceListFile);
	std::string s;
	std::vector<std::string> traceList;
	while(std::getline(traceListIf, s)) {
		if(s.substr(0, 6) == "kernel")
			traceList.push_back(traceDir + "/" + s);
	}
	if(traceList.empty()) {
		std::cerr << "[RFC-sim] Empty kernel list." << std::endl;
		return 1;
	}

	// Parsing
	std::shared_ptr<cfg::GlobalCfg> cfg = std::make_shared<cfg::GlobalCfg>(); 
	
    std::unique_ptr<cfg::CfgParser> cfgParser = std::make_unique<cfg::CfgParser>(cfgFile, cfg);
	cfgParser->parse();
	cfgParser->print();

	using mapT = std::unordered_map<uint32_t, std::bitset<4>>;
	std::shared_ptr<mapT> tab = std::make_shared<mapT>();
	std::unique_ptr<AsmParser> asmParser = std::make_unique<AsmParser>(asmFile, tab);
	asmParser->parse();
    std::unique_ptr<TraceParser> traceParser = std::make_unique<TraceParser>(traceList[0], tab);

	// statistics
	auto engyMdl = cfg->engyMdl;
	std::shared_ptr<stat::RfcStat> stat = std::make_shared<stat::RfcStat>(engyMdl);
	std::shared_ptr<stat::InstStat> iStat = std::make_shared<stat::InstStat>();	

	// mrf
    auto mrf = std::make_shared<Mrf>(stat, iStat);

	// Run
    std::cout << "[RFC-sim] MRF only..." << std::endl;
    std::cout << "[RFC-sim] Start parsing..." << std::endl;
    
	for(auto & traceFile : traceList) {
		traceParser->reset(traceFile);
		// std::cout << "[RFC-sim] parsing: " << traceFile << std::endl;
		while(!traceParser->eof()) {
			auto inst = traceParser->parse();
			if (inst.opcode == op::OP_VOID) break;
			mrf->exec(inst);
		}
	}

	auto iStatSwp = *iStat;
	auto statSwp = *stat;

	iStat->clear();
	stat->clear();

	// RFC
	std::cout << std::endl;
    std::cout << "[RFC-sim] Turn on RFC..." << std::endl;
    std::cout << "[RFC-sim] Start parsing..." << std::endl;

	std::vector<Rfc> rfcArry;
	for (auto i = 0; i < 32; i++)
		rfcArry.push_back(Rfc(cfg, stat, mrf));

	for(auto traceFile : traceList) {
		traceParser->reset(traceFile);
		// std::cout << "[RFC-sim] parsing: " << traceFile << std::endl;
    	
		while(!traceParser->eof()) {
			auto inst = traceParser->parse();
          
			#ifndef NDEBUG
			std::cout << "[RFC-sim] " << inst << std::endl;
			while(true) {
				char ch = std::cin.get();
				if(ch == '\n') 
					break;
			}
			#endif

			try {
				rfcArry[inst.wId % 32].exec(inst);
				// std::cout << rfcArry.at(inst.wId % 32) << std::endl;
			} catch (std::exception & e) {
				std::cerr << e.what() << std::endl;
			}
		}
	}

	std::cout << std::endl << std::endl;
	std::cout << "| ================================ Statistics ======================================" << std::endl;
    std::cout << "Workload: " << traceDir << std::endl;
    std::cout << "Config: " << *cfg << std::endl;
    std::cout << std::endl;
	std::cout << " >----------------------------- Workload Stats ----------------------------<" << std::endl;
	std::cout << iStatSwp << std::endl;
	std::cout << " >----------------------------- MRF only ----------------------------<" << std::endl;
	std::cout << statSwp << std::endl;
	std::cout << " >----------------------------- RFC ---------------------------------<" << std::endl;
	std::cout << *stat << std::endl;
	std::cout << " >----------------------------- Comparison --------------------------<" << std::endl;
	stat::RfcStat::printCmp(statSwp, *stat);

	std::ofstream of(logFile);
	Logger::logging(of, *cfg, *iStat, statSwp, *stat);
	of.close();

	return 0;
}
