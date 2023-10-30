#include "Alloc.h"

void WriteAllocator::alloc(const reg::Oprd& oprd, uint32_t tid) {
    if (oprd.type == reg::OprdT::dst) {
        auto p = cc->replWrapper(tid, cc->getCacheSet(oprd));
        cc->cam->vMem[tid].at(p.second).set(oprd.index / cc->cfg->nDW, 1, true);
        
        cc->simdBuf.at(1).set(tid); // RFC.W
        if (p.first) cc->simdBuf.at(3).set(tid); // MRF.W;
    }
    else if (oprd.type == reg::OprdT::src)
        cc->simdBuf.at(2).set(tid); // MRF.R
}

void CplAidedAllocator::alloc(const reg::Oprd& oprd, uint32_t tid) {
    if (oprd.type == reg::OprdT::src) {
        if (cc->flags.test(3 - oprd.pos)) {
            auto p = cc->replWrapper(tid, cc->getCacheSet(oprd));
            cc->cam->vMem[tid].at(p.second).set(oprd.index / cc->cfg->nDW, 1, false);

            cc->simdBuf.at(1).set(tid); // RFC.W
            cc->simdBuf.at(2).set(tid); // MRF.R

            if (p.first) cc->simdBuf.at(3).set(tid); // MRF.W;
        }
        else
            cc->simdBuf.at(2).set(tid); // MRF.R
    }
    else if (oprd.type == reg::OprdT::dst) {
        auto p = cc->replWrapper(tid, cc->getCacheSet(oprd));
        cc->cam->vMem[tid].at(p.second).set(oprd.index / cc->cfg->nDW, 1, true);
        cc->simdBuf.at(1).set(tid); // RFC.W
        if (p.first) cc->simdBuf.at(3).set(tid); // MRF.W;
    }
}

void LookAheadAllocator::alloc(const reg::Oprd& oprd, uint32_t tid) {
    if (oprd.type == reg::OprdT::src) {
        if (cc->flags.test(3 - oprd.pos)) {
            auto p = cc->replWrapper(tid, cc->getCacheSet(oprd));
            cc->cam->vMem[tid].at(p.second).set(oprd.index / cc->cfg->nDW, 1, false);

            cc->simdBuf.at(1).set(tid); // RFC.W
            cc->simdBuf.at(2).set(tid); // MRF.R

            if (p.first) cc->simdBuf.at(3).set(tid); // MRF.W;
        }
        else
            cc->simdBuf.at(2).set(tid); // MRF.R
    }
    else if (oprd.type == reg::OprdT::dst) {
        auto iQueue = cc->iQueue; 
        
        while (!iQueue.empty()) {
            auto inst = iQueue.front();
            for (auto & bufferedOprd : inst.regPool) {
                if (bufferedOprd.index == oprd.index) { // allocate
                    auto p = cc->replWrapper(tid, cc->getCacheSet(oprd));
                    cc->cam->vMem[tid].at(p.second).set(oprd.index / cc->cfg->nDW, 1, true);
                    cc->simdBuf.at(1).set(tid); // RFC.W
                    if (p.first) 
                        cc->simdBuf.at(3).set(tid); // MRF.W;
                    return;
                }
            }
            iQueue.pop();
        }
        cc->simdBuf.at(3).set(tid); // if not presented in the look-ahead table, then just write back
    }
} 


BaseAllocator * AllocatorFactory::getInstance(Rfc * cc, const cfg::GlobalCfg& config) {
    BaseAllocator* allocator = nullptr;
    switch (config.alloc) {
        case cfg::AllocPlcy::writeAlloc: 
            allocator = new WriteAllocator(cc);
            break;
        case cfg::AllocPlcy::cplAidedAlloc: 
            allocator = new CplAidedAllocator(cc);
            break;
        case cfg::AllocPlcy::lookAheadAlloc:
            allocator = new LookAheadAllocator(cc);
            break;
        default: 
            return nullptr;
    }
    return allocator;
}