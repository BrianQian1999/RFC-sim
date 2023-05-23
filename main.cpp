// RFC-sim is a analytic model for the Register File Cache used in NVIDIA GPUs
// For more information, please check the paper:
// Energy-efficient Mechanisms for Managing Thread Context in Throughput Processors (ISCA 2011)
// Qiran Qian, <qiranq@kth.se>

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <stdexcept>
#include <memory>

#include "utils.h"
#include "trace_parser.h"
#include "cfg_parser.h"
#include "mrf.h"
#include "rfc.h"

// #define NDEBUG

int main(int argc, char ** argv) {
    // Arg Parser
    if(argc < 5 || std::string(argv[1]) != "-t" || std::string(argv[3]) != "-c") {
        std::cerr << "Usage: " << argv[0] << " -t <path_to_trace_dir> " << "-c <path_to_config_file>" << std::endl;
        return 1;
    }
    
    const std::string cfgFile = std::string(argv[4]);
    const std::string traceDir = std::string(argv[2]);
	
	const std::string traceListFile = traceDir + "/kernelslist.g";
	std::cout << "[main] Kernel list file: " << traceListFile << std::endl;
	std::cout << "[main] Configuration file: " << cfgFile << std::endl;

	// Get traces of all kernels
	std::ifstream traceListIf(traceListFile);
	std::string s;
	std::vector<std::string> traceList;
	while(std::getline(traceListIf, s)) {
		if(s.substr(0, 6) == "kernel")
			traceList.push_back(traceDir + "/" + s);
	}
	if(traceList.empty()) {
		std::cerr << "[main] Empty kernel list." << std::endl;
		return 1;
	}

    // Initialize global config and config parser
    std::shared_ptr<GlobalCfg> globalCfgPtr = std::make_shared<GlobalCfg>();
    std::shared_ptr<CfgParser> cfgParserPtr = std::make_shared<CfgParser>(cfgFile, globalCfgPtr);

	cfgParserPtr->ParseCfg();
	globalCfgPtr->PrintCfg();

    // Initialize trace parser and MRF
    std::shared_ptr<Mrf> mrfPtr = std::make_shared<Mrf>();
    std::unique_ptr<TraceParser> traceParserPtr = std::make_unique<TraceParser>(traceList[0]);

    /*
     *  Turn off RFC
     */   

	std::cout << std::endl;
    std::cout << "===========================================================================================" << std::endl;
    std::cout << "[main] Turn off RFC..." << std::endl;
    std::cout << "[main] Start parsing..." << std::endl;
    
	for(auto traceStr : traceList) {
		traceParserPtr->Reset(traceStr); // Reset the trace kernel
		std::cout << "[main] Processing " << traceStr << std::endl;
		while(!traceParserPtr->IsEOF()) {
        	mrfPtr->ProcInst(traceParserPtr->ParseTrace());
    	} 
	}

	// Reset MRF
	std::cout << "[main] Reset..." << std::endl;
    mrfPtr->PrintStats();
    mrfPtr->Reset();
    std::cout << "===========================================================================================" << std::endl;
    
    /*
     *  Turn on RFC
     */
    
    std::cout << std::endl;
    std::cout << "[main] Turn on RFC..." << std::endl;
	std::cout << "[main] RFC configuration:" << std::endl;
    std::cout << "[main] Start parsing..." << std::endl;

	// Initialize RFC array
	std::unique_ptr<RfcArry> rfcArryPtr = std::make_unique<RfcArry>(globalCfgPtr, mrfPtr);

	for(auto traceStr : traceList) {
		traceParserPtr->Reset(traceStr);
		std::cout << "[main] Processing " << traceStr << std::endl;
    	while(!traceParserPtr->IsEOF()) {
        	auto inst = traceParserPtr->ParseTrace();
			if(inst.opcode == OP_VOID) 
				break;
#ifndef NDEBUG
			std::cout << "[main] Processing: " << inst << std::endl;
#endif
			rfcArryPtr->ProcInst(inst);
		}
	}
    mrfPtr->PrintStats();
    return 0;
} 
