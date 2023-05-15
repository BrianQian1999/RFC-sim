// Do whatever you want here to test the model

#include <iostream>
#include "cfg_parser.h"

int main(int argc, char ** argv) {
    if(argc < 3 || std::string(argv[1]) != "-config") {
        std::cerr << "Usage: " << argv[0] << " -config ${PATH_TO_CONFIG}" << std::endl;
       return 1; 
    }

    const std::string config_file = argv[2];

    // Initialize global config
    GlobalCfg globalCfg;

    // Initialize config parser
    std::unique_ptr<CfgParser> cfgParserPtr = std::make_unique<CfgParser>(config_file, globalCfg);

    // Parse global configuration
    cfgParserPtr->ParseCfg();
    cfgParserPtr->PrintCfg();

    return 0;
}
