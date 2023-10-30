/*** RFC-sim is a analytic model for the Register File Cache used in NVIDIA GPUs
 *   For more information, please check the paper:
 *   Energy-efficient Mechanisms for Manstep Thread Context in Throughput Processors (ISCA 2011)
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

	std::cout << "[RFC-sim] Parsing input arguments..." << std::endl;
	std::cout << "[RFC-sim] Trace file directory: " << traceListFile << std::endl;
	std::cout << "[RFC-sim] Config file: " << cfgFile << std::endl;
    std::cout << "[RFC-sim] Assembly file: " << asmFile << std::endl;
	
	// detect all kernels
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

	/*
	############################################################################################################
	*/
	// Parse config
	std::shared_ptr<cfg::GlobalCfg> cfg = std::make_shared<cfg::GlobalCfg>(); 
    std::unique_ptr<cfg::CfgParser> cfgParser = std::make_unique<cfg::CfgParser>(cfgFile, cfg);
	cfgParser->parse();
	std::cout << "\n-----------------------------------------------------------------------------------------\n";
	cfgParser->print();
	std::cout << "\n-----------------------------------------------------------------------------------------\n\n";

	using mapT = std::unordered_map<uint32_t, std::bitset<4>>;
	std::unique_ptr<AsmParser> asmParser = std::make_unique<AsmParser>(
		asmFile, 
		std::make_shared<std::vector<mapT>>(),
		std::make_shared<std::unordered_map<std::string, size_t>>()
	);
	asmParser->parse();

	std::unique_ptr<TraceParser> traceParser = std::make_unique<TraceParser>(
		traceList.at(0), 
		asmParser->tab,
		asmParser->map
	);

	// statistics
	auto eMdl = cfg->eMdl;
	std::shared_ptr<stat::Stat> scoreboardBase = std::make_shared<stat::Stat>(eMdl);
	std::shared_ptr<stat::Stat> scoreboard = std::make_shared<stat::Stat>(eMdl);

	// RFC
    std::cout << "[RFC-sim] Simulating >>> " << std::endl;
	std::vector<Rfc> rfcArry;

	// Initialize RFC instance for each warp on SM core (e.g., for TU102, 4 sub-core * 8 warps = 32)
	for (auto i = 0; i < 32; i++)
		rfcArry.push_back(Rfc(cfg, scoreboardBase, scoreboard));
	
	// Traverse GPU Kernels
	for(auto & traceFile : traceList) {
		traceParser->reset(traceFile);

		bool eof = false;
		while(!eof) {
			auto inst = traceParser->parse();

#ifndef NDEBUG
			while(true) {
				char ch = std::cin.get();
				if(ch == '\n') 
					break;
			}
#endif

			eof = rfcArry.at(inst.wId % 32).exec(inst);
			
#ifndef NDEBUG
			std::cout << "[RFC] " << rfcArry.at(inst.wId % 32) << std::endl;
			std::cout << "'[Stat] " << *scoreboard << std::endl;
#endif
		}
	}
    
	std::cout << "[RFC-sim] <<< Simulation End" << std::endl;

	std::cout << "--------------------------------------------------------------------------------\n";
	std::cout << "[RFC-sim] Statistics " << std::endl;
	std::cout << *scoreboardBase << std::endl;
	std::cout << *scoreboard << std::endl;
	stat::Stat::printCmp(*scoreboardBase, *scoreboard);
	std::cout << std::endl;
	std::cout << "--------------------------------------------------------------------------------\n";

	// Logging
	std::ofstream of(logFile, std::ios::app);
	if (of.is_open()) {
		Logger::logging(of, *cfg, *scoreboardBase, *scoreboard);
		of.close();
	}
    std::cout << "[RFC-sim] End.\n\n";
	return 0;
}
