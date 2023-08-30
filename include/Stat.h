#pragma once

#include <cstdint>
#include <iostream>

#include "CfgParser.h"

namespace stat {

    struct InstStat {
        uint64_t mmaInstNum;
        uint64_t totInstNum;
        InstStat();
        void clear() noexcept;
    };

    inline std::ostream & operator<<(std::ostream & os, const InstStat & s) {
        float mmaInstDensity = (float) s.mmaInstNum / s.totInstNum * 100.0;
        os << "(tot inst, mma inst, mma density) -> (" 
           << s.totInstNum << ", " << s.mmaInstNum << ", " << mmaInstDensity << "\%)"
           << std::endl;
        return os;
    }
    
    struct RfcStat {
        cfg::EngyMdl engyMdl;
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
        void clear() noexcept;
        float calcRfEngy() const;

        static void printCmp(const RfcStat &, const RfcStat&);
    };

    inline std::ostream & operator<<(std::ostream & os, const RfcStat & s) {
        os << "(rfc.r | rfc.w | mrf.r | mrf.w) -> (" 
           << s.rfcRdNum << " | " << s.rfcWrNum << " | "
           << s.mrfRdNum << "| " << s.mrfWrNum << ")\n";

        double rdHitRate = double (s.rfcRdHitNum) / (s.rfcRdHitNum+s.rfcRdMissNum) * 100;
        double wrHitRate = double (s.rfcWrHitNum) / (s.rfcWrHitNum+s.rfcWrMissNum) * 100;
        double totHitRate = double (s.rfcRdHitNum+s.rfcWrHitNum) 
            / (s.rfcRdHitNum+s.rfcRdMissNum+s.rfcWrHitNum+s.rfcWrMissNum) * 100;

        os << "(Read Hit Rate | Write Hit Rate | Total Hit Rate) -> ("
           << rdHitRate << "\% | " 
           << wrHitRate << "\% | "
           << totHitRate << "\%)" << std::endl;

        os << "(E.rfc.r | E.rfc.w | E.mrf.r | E.mrf.w (uJ) ) -> "
           << s.rfcRdNum * s.engyMdl.rfcRdEngy / 1e6<< " | "
           << s.rfcWrNum * s.engyMdl.rfcWrEngy / 1e6<< " | "
           << s.mrfRdNum * s.engyMdl.mrfRdEngy / 1e6 << " | "
           << s.mrfWrNum * s.engyMdl.mrfWrEngy / 1e6 << ")\n"; 

        float rfEngy = s.calcRfEngy();
        os << "RF Energy -> " << rfEngy / 1e6 << " (uJ)" << std::endl; 

        return os;
    }

}; /*namespace stat*/
