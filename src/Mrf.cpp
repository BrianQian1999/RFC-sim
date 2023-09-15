#include "Mrf.h"

void Mrf::clear() {
    stat->clear();
    iStat->clear();
}

void Mrf::req(bool rd) noexcept {
    if (rd) stat->mrfRdNum += mask.count();
    else stat->mrfWrNum += mask.count();
}

void Mrf::exec(const TraceInst & inst) {
    this->mask = inst.mask;
    iStat->totInstNum++;
    iStat->mmaInstNum += (uint64_t) (inst.opcode==op::OP_HMMA || inst.opcode==op::OP_IMMA || inst.opcode==op::OP_BMMA);
    for(const auto & reg : inst.regPool) {
        if (reg.type == reg::OprdT::ADDR) return;
		req(reg.type == reg::OprdT::SRC);
    }
}
