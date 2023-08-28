#include "Mrf.h"

void Mrf::clear() {
    stat->clear();
    iStat->clear();
}

void Mrf::req(bool rd) noexcept {
    if (rd) stat->mrfRdNum += 32;
    else stat->mrfWrNum += 32;
}

void Mrf::exec(const TraceInst & inst) {
    iStat->totInstNum++;
    iStat->mmaInstNum += (uint64_t) (inst.opcode==op::OP_HMMA || inst.opcode==op::OP_IMMA || inst.opcode==op::OP_BMMA);
   
    for(const auto & reg : inst.regPool) {
        req(reg.regType == reg::RegOprdT::SRC);
    }
}