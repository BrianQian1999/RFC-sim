#include "Logger.h"

namespace Logger {
    void logging (
        std::ofstream& of,
        const cfg::GlobalCfg& cfg,
        const stat::InstStat& iStat,
        const stat::RfcStat& statBase,
        const stat::RfcStat& statOpt 
    ) {
        of << "(" << cfg.assoc << "-way : " << cfg.alloc << " : " << cfg.repl << " : " <<  cfg.ev << ");"; 
        
        double rdHitRate = double (statOpt.rfcRdHitNum) / (statOpt.rfcRdHitNum+statOpt.rfcRdMissNum) * 100;
        double wrHitRate = double (statOpt.rfcWrHitNum) / (statOpt.rfcWrHitNum+statOpt.rfcWrMissNum) * 100;
        double totHitRate = double (statOpt.rfcRdHitNum+statOpt.rfcWrHitNum) 
            				/ (statOpt.rfcRdHitNum+statOpt.rfcRdMissNum+statOpt.rfcWrHitNum+statOpt.rfcWrMissNum) * 100;
    
        of << rdHitRate << ";" << wrHitRate << ";" << totHitRate << ";";
        
        int64_t mrfRdReduction = statBase.mrfRdNum - statOpt.mrfRdNum;
        int64_t mrfWrReduction = statBase.mrfWrNum - statOpt.mrfWrNum;
        
        of << (double) mrfRdReduction / statBase.mrfRdNum * 100 << ";"
           << (double) mrfWrReduction / statBase.mrfWrNum * 100 << ";";

        of << statOpt.rfcRdNum << ";" << statOpt.rfcWrNum << ";"
           << statOpt.mrfRdNum << ";" << statOpt.mrfWrNum << ";";

        of << (statBase.calcRfEngy() - statOpt.calcRfEngy()) 
                        / statBase.calcRfEngy() * 100 << "\n";
    }
};
