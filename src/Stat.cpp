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

    void RfcStat::trigger(Event e) noexcept {
        switch(e) {
            case(Event::RD_HIT): rfcRdHitNum++; break;
            case(Event::RD_MISS): rfcRdMissNum++; break;
            case(Event::WR_HIT): rfcWrHitNum++; break;
            case(Event::WR_MISS):rfcWrMissNum++; break;

            case(Event::CC_RD): rfcRdNum++; break;
            case(Event::CC_WR): rfcWrNum++; break;
            case(Event::RF_RD): mrfRdNum++; break;
            case(Event::RF_WR): mrfWrNum++; break;
            default: break;
        }
    } 
    
    void RfcStat::trigger(Event e, uint32_t acc) noexcept {
        switch(e) {
            case(Event::RD_HIT): rfcRdHitNum+=acc; break;
            case(Event::RD_MISS): rfcRdMissNum+=acc; break;
            case(Event::WR_HIT): rfcWrHitNum+=acc; break;
            case(Event::WR_MISS):rfcWrMissNum+=acc; break;

            case(Event::CC_RD): rfcRdNum+=acc; break;
            case(Event::CC_WR): rfcWrNum+=acc; break;
            case(Event::RF_RD): mrfRdNum+=acc; break;
            case(Event::RF_WR): mrfWrNum+=acc; break;
            default: break;
        }
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
        std::cout << "\t(MRF.R avoided, MRF.W avoided) -> ("
                  << (double) mrfRdReduction / mrfOnlyStat.mrfRdNum * 100 << "\%, "
                  << (double) mrfWrReduction / mrfOnlyStat.mrfWrNum * 100 << "\%)\n";

        std::cout << "\t(Dynamic energy reduction) -> " 
                  << (mrfOnlyStat.calcRfEngy() - rfcStat.calcRfEngy()) 
                        / mrfOnlyStat.calcRfEngy() * 100 << "\%\n";
    }

};
