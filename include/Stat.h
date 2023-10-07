#pragma once

#include <cstdint>
#include <iostream>

#include "CfgParser.h"

namespace stat {

    enum class Event {
        rdMiss,
        rdHit,
        wrMiss,
        wrHit,

        rfcRd,
        rfcWr,
        mrfRd,
        mrfWr
    };

    struct InstStat {
        uint64_t mmaInstNum;
        uint64_t totInstNum;
        InstStat();
        void clear() noexcept;
    };

    inline std::ostream & operator<<(std::ostream & os, const InstStat & s) {
        float mmaInstDensity = (float) s.mmaInstNum / s.totInstNum * 100.0;
        os << "\t(tot inst, mma inst, mma density) -> (" 
           << s.totInstNum << ", " << s.mmaInstNum << ", " << mmaInstDensity << "\%)"
           << std::endl;
        return os;
    }
    
    struct RfcStat {
        cfg::EngyMdl eMdl;
        explicit RfcStat(const cfg::EngyMdl&);

        uint64_t mrfRdNum;
        uint64_t mrfWrNum;

        uint64_t rfcRdNum;
        uint64_t rfcWrNum; 

        uint64_t rfcRdHitNum;
        uint64_t rfcRdMissNum; 
        uint64_t rfcWrHitNum;
        uint64_t rfcWrMissNum;

        void accMiss(bool) noexcept;
        void accHit(bool) noexcept;

        void trigger(Event) noexcept;
        void trigger(Event, uint32_t) noexcept;
        
        void clear() noexcept;
        float calcRfEngy() const;

        static void printCmp(const RfcStat &, const RfcStat&);
    };

    inline std::ostream & operator<<(std::ostream & os, const RfcStat & s) {
        os << "\t(RFC.R, RFC.W, MRF.R, MRF.W) -> (" 
           << s.rfcRdNum << ", " << s.rfcWrNum << ", "
           << s.mrfRdNum << ", " << s.mrfWrNum << ")\n";
 
        double rdHitRate = double (s.rfcRdHitNum) / (s.rfcRdHitNum+s.rfcRdMissNum) * 100;
        double wrHitRate = double (s.rfcWrHitNum) / (s.rfcWrHitNum+s.rfcWrMissNum) * 100;
        double totHitRate = double (s.rfcRdHitNum+s.rfcWrHitNum) 
            / (s.rfcRdHitNum+s.rfcRdMissNum+s.rfcWrHitNum+s.rfcWrMissNum) * 100;

        os << "\t(Read-hit Rate, Write-hit Rate) -> ("
           << rdHitRate << "\%, " 
           << wrHitRate << "\%)" << std::endl;

        os << "\t(E.RFC.R, E.RFC.W, E.MRF.R, E.MRF.W (uJ) ) -> "
           << s.rfcRdNum * s.eMdl.eRfcRd / 1e6<< ", "
           << s.rfcWrNum * s.eMdl.eRfcWr / 1e6<< ", "
           << s.mrfRdNum * s.eMdl.eMrfRd / 1e6 << ", "
           << s.mrfWrNum * s.eMdl.eMrfWr / 1e6 << ")\n"; 

        float rfEngy = s.calcRfEngy();
        os << "\t(RF Dynamic Energy) -> " << rfEngy / 1e6 << " (uJ)" << std::endl; 

        return os;
    }

}; /*namespace stat*/
