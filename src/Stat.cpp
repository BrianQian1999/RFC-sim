#include "Stat.h"

namespace stat {

    Stat::Stat(const cfg::EngyMdl& mdl) : eMdl(mdl) {
        mrfRdNum=0;
        mrfWrNum=0;
        rfcRdNum=0;
        rfcWrNum=0;
        rfcRdHitNum=0;
        rfcRdMissNum=0;
        rfcWrHitNum=0;
        rfcWrMissNum=0;
    }

    void Stat::accMiss(bool rd) noexcept {
        if (rd) rfcRdMissNum++;
        else rfcWrMissNum++;
    }

    void Stat::accHit(bool rd) noexcept {
        if (rd) rfcRdHitNum++;
        else rfcWrHitNum++;
    }

    void Stat::trigger(Event e) noexcept {
        switch(e) {
            case(Event::rdHit): rfcRdHitNum++; break;
            case(Event::rdMiss): rfcRdMissNum++; break;
            case(Event::wrHit): rfcWrHitNum++; break;
            case(Event::wrMiss):rfcWrMissNum++; break;

            case(Event::rfcRd): rfcRdNum++; break;
            case(Event::rfcWr): rfcWrNum++; break;
            case(Event::mrfRd): mrfRdNum++; break;
            case(Event::mrfWr): mrfWrNum++; break;
            default: break;
        }
    } 
    
    void Stat::trigger(Event e, uint32_t acc) noexcept {
        switch(e) {
            case(Event::rdHit): rfcRdHitNum+=acc; break;
            case(Event::rdMiss): rfcRdMissNum+=acc; break;
            case(Event::wrHit): rfcWrHitNum+=acc; break;
            case(Event::wrMiss):rfcWrMissNum+=acc; break;

            case(Event::rfcRd): rfcRdNum+=acc; break;
            case(Event::rfcWr): rfcWrNum+=acc; break;
            case(Event::mrfRd): mrfRdNum+=acc; break;
            case(Event::mrfWr): mrfWrNum+=acc; break;
            default: break;
        }
    }

    void Stat::clear() noexcept {
        mrfRdNum=0;
        mrfWrNum=0;
        rfcRdNum=0;
        rfcWrNum=0;
        rfcRdHitNum=0;
        rfcRdMissNum=0;
        rfcWrHitNum=0;
        rfcWrMissNum=0;
    }

    float Stat::calcRfEngy() const {
        return (mrfRdNum * eMdl.eMrfRd) + 
               (mrfWrNum * eMdl.eMrfWr) + 
               (rfcRdNum * eMdl.eRfcRd) + 
               (rfcWrNum * eMdl.eRfcWr);
    }
    
    void Stat::printCmp(const Stat & mrfOnlyStat, const Stat & rfcStat) {
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
