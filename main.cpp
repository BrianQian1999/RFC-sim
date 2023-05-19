// C++ 14

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


int main(int argc, char ** argv) {
    // Arg Parser
    if(argc < 5 || std::string(argv[1]) != "-trace" || std::string(argv[3]) != "-config") {
        std::cerr << "Usage: " << argv[0] << " -trace <PATH_TO_TRACE> " << "-config <PATH_TO_CONFIG>" << std::endl;
        return 1;
    }
    
    const std::string cfgFile = std::string(argv[4]);
    const std::string traceFile = std::string(argv[2]);
	
	std::cout << "Trace file: " << traceFile << std::endl;
	std::cout << "Config file: " << cfgFile << std::endl;

    // Initialize global config and config parser
    std::shared_ptr<GlobalCfg> globalCfgPtr = std::make_shared<GlobalCfg>();
    std::shared_ptr<CfgParser> cfgParserPtr = std::make_shared<CfgParser>(cfgFile, globalCfgPtr);

	cfgParserPtr->ParseCfg();
	globalCfgPtr->PrintCfg();

    // Initialize trace parser and MRF
    std::shared_ptr<Mrf> mrfPtr = std::make_shared<Mrf>();
    std::unique_ptr<TraceParser> traceParserPtr = std::make_unique<TraceParser>(traceFile);

    /*
     *  Turn off RFC
     */   

    std::cout << "==============================================" << std::endl;
    std::cout << "[MAIN] Turn off RFC..." << std::endl;
    std::cout << "[MAIN] Start Parser..." << std::endl;
    while(!traceParserPtr->IsEOF()) {
        mrfPtr->ProcInst(traceParserPtr->ParseTrace());
    } 
    mrfPtr->PrintStats();
    mrfPtr->Reset();

    std::cout << "[MAIN] Reset Parser..." << std::endl;
    std::cout << "==============================================" << std::endl;
    traceParserPtr->Reset(traceFile);
    
    /*
     *  Turn on RFC
     */
    
    std::cout << std::endl;
    std::cout << "[MAIN] Turn on RFC..." << std::endl;
	std::cout << "[MAIN] RFC configuration:" << std::endl;
    std::cout << "[MAIN] Start parsing..." << std::endl;

	std::unique_ptr<RfcArry> rfcArryPtr = std::make_unique<RfcArry>(globalCfgPtr, mrfPtr);

    while(!traceParserPtr->IsEOF()) {
        auto inst = traceParserPtr->ParseTrace();
		rfcArryPtr->ProcInst(inst);
	}
    mrfPtr->PrintStats();
    return 0;
} 
