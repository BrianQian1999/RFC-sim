#include "Stat.h"

namespace stat {

    InstStat::InstStat() {
        mmaInstNum = 0;
        totInstNum = 0;
    }

    void InstStat::clear() noexcept {
        mmaInstNum = 0;
        totInstNum = 0;
    }

    RfcStat::RfcStat(const cfg::EngyMdl& mdl) : engyMdl(mdl) {
        mrfRdNum=0;
        mrfWrNum=0;
        rfcRdNum=0;
        rfcWrNum=0;
        rfcRdHitNum=0;
        rfcRdMissNum=0;
        rfcWrHitNum=0;
        rfcWrMissNum=0;
    }

    void RfcStat::accMiss(bool rd) noexcept {
        if (rd) rfcRdMissNum++;
        else rfcWrMissNum++;
    }

    void RfcStat::accHit(bool rd) noexcept {
        if (rd) rfcRdHitNum++;
        else rfcWrHitNum++;
    }

    void RfcStat::clear() noexcept {
        mrfRdNum=0;
        mrfWrNum=0;
        rfcRdNum=0;
        rfcWrNum=0;
        rfcRdHitNum=0;
        rfcRdMissNum=0;
        rfcWrHitNum=0;
        rfcWrMissNum=0;
    }

    float RfcStat::calcRfEngy() const {
        return (mrfRdNum * engyMdl.mrfRdEngy) + 
               (mrfWrNum * engyMdl.mrfWrEngy) + 
               (rfcRdNum * engyMdl.rfcRdEngy) + 
               (rfcWrNum * engyMdl.rfcWrEngy);
    }
    
    void RfcStat::printCmp(const RfcStat & mrfOnlyStat, const RfcStat & rfcStat) {
        int64_t mrfRdReduction = mrfOnlyStat.mrfRdNum - rfcStat.mrfRdNum;
        int64_t mrfWrReduction = mrfOnlyStat.mrfWrNum - rfcStat.mrfWrNum;
        std::cout << "|------------------------ Comparison (MRF-only vs. RFC) ----------------------| " << std::endl;
        std::cout << "(mrf.r avoided | mrf.w avoided) -> ("
                  << (double) mrfRdReduction / mrfOnlyStat.mrfRdNum * 100 << "\% | "
                  << (double) mrfWrReduction / mrfOnlyStat.mrfWrNum * 100 << "\%)\n";

        std::cout << "E reduction -> " 
                  << (mrfOnlyStat.calcRfEngy() - rfcStat.calcRfEngy()) 
                        / mrfOnlyStat.calcRfEngy() * 100 << "\%\n";
    }

};
