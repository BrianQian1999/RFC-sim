// C++ 14

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <stdexcept>
#include <memory>

#include "utils.h"
#include "trace_parser.h"
#include "config_parser.h"
#include "mrf.h"
#include "rfc.h"


int main(int argc, char ** argv) {
    // Arg Parser
    if(argc < 5 || std::string(argv[1]) != "-trace" || std::string(argv[3]) != "-config") {
        std::cerr << "Usage: " << argv[0] << " -trace ${PATH_TO_TRACE} " << "-config ${PATH_TO_CONFIG}" << std::endl;
        return 1;
    }
    
    const std::string cfgFile = std::string(argv[4]);
    const std::string traceFile = std::string(argv[2]);

    // Initialize global config and config parser
    std::shared_ptr<GlobalCfg> globalCfgPtr = std::make_shared<GlobalCfg>();
    std::unique_ptr<CfgParser> cfgParserPtr = std::make_unique<CfgParser>(cfgFile, *globalCfgPtr);

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
        mrf_ptr->ServeInst(parser_ptr->ParseTrace());
    } 
    mrf_ptr->PrintStats();
    mrf_ptr->Reset();

    std::cout << "[MAIN] Reset Parser..." << std::endl;
    std::cout << "==============================================" << std::endl;
    parser_ptr->Reset(trace_file);
    
    /*
     *  Turn on RFC
     */
    
    // Each Sub-Core, n_entry x 8 (warps) x 32 (lanes) x 4 bytes
    // 6 entries -> 6 KB
    // 8 entries -> 8 KB   
    unsigned numEntry = globalCfgPtr->numEntry;

    std::unique_ptr<RfConfig> config_0_ptr = std::make_unique<RfConfig>(0, numEntry, LRU);
    std::unique_ptr<RfConfig> config_1_ptr = std::make_unique<RfConfig>(1, numEntry, LRU);
    std::unique_ptr<RfConfig> config_2_ptr = std::make_unique<RfConfig>(2, numEntry, LRU);
    std::unique_ptr<RfConfig> config_3_ptr = std::make_unique<RfConfig>(3, numEntry, LRU);

    std::unique_ptr<Rfc> rfc_0_ptr = std::make_unique<Rfc>(config_0_ptr, mrf_ptr);
    std::unique_ptr<Rfc> rfc_1_ptr = std::make_unique<Rfc>(config_1_ptr, mrf_ptr);
    std::unique_ptr<Rfc> rfc_2_ptr = std::make_unique<Rfc>(config_2_ptr, mrf_ptr);
    std::unique_ptr<Rfc> rfc_3_ptr = std::make_unique<Rfc>(config_3_ptr, mrf_ptr);


    std::cout << std::endl;
    std::cout << "[MAIN] Turn on RFC..." << std::endl;
    std::cout << "[MAIN] Start parsing..." << std::endl;

    while(!parser_ptr->IsEOF()) {
        auto inst = parser_ptr->ParseTrace();
        switch(inst.warp_id % 4) {
            case(0): 
                rfc_0_ptr->ServeInst(inst);
                // rfc_0_ptr->PrintStatus();
                break;
            case(1):
                rfc_1_ptr->ServeInst(inst);
                break;
            case(2):
                rfc_2_ptr->ServeInst(inst);
                break;
            case(3):
                rfc_3_ptr->ServeInst(inst);
                break;
            default:
                throw std::runtime_error("[MAIN] Runtime error");
        }
    }
    mrf_ptr->PrintStats();
    return 0;
} 
