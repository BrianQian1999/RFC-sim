#include <iostream>
#include <fstream>

#include "Stat.h"
#include "CfgParser.h"

namespace Logger {

    void logging(
        std::ofstream &,
        const cfg::GlobalCfg&,
        const stat::InstStat&, 
        const stat::RfcStat&,
        const stat::RfcStat&
    );

};
