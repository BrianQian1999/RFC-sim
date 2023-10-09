#include "Rfc.h"

RfcBlock::RfcBlock() {
    tag = 256;
    age = 0;
    dt = false;
}

std::ostream & operator<<(std::ostream & os, const RfcBlock & e) {
    os << "(" << e.tag << "," << e.age << "," << e.dt << ")";
    return os;
}

void RfcBlock::clear() noexcept {
    tag = 256;
    age = 0;
    dt = false;
}

void RfcBlock::step() noexcept {
    age++;
}

void RfcBlock::set(uint32_t tag, uint32_t age, bool dt) noexcept {
    this->tag = tag;
    this->age = age;
    this->dt = dt;
}

// struct Cam
std::ostream & operator<<(std::ostream & os, const Cam & cam) {
    os << "[FSM]:\n\t";
    for (auto & blk : cam.mem) {
        for (auto & e : blk) {
            os << e << " ";
        }
        os << "\n\t";
    }
    return os;
}

void Cam::flush() {
    
    for (auto & blk : mem) {
        for (auto & e : blk) e.clear();
    }
    
    for (auto & blk : vMem) {
        for (auto & e : blk) e.clear();
    }

}

void Cam::step() {
    for (auto & blk : vMem) {
        for (auto & e : blk) e.step();
    }
}

std::pair<bool, uint32_t> Cam::search(uint32_t tid, uint32_t tag, uint32_t setId) {
    uint32_t startIdx = setId * assoc;
    uint32_t endIdx = startIdx + assoc;
    
    for (auto i = startIdx; i < endIdx; i++) {
        const auto & e = mem[tid].at(i);
        if (e.tag == tag)
            return std::make_pair<bool, uint32_t>(true, std::move(i));
    }    
    return std::make_pair<bool, uint32_t>(false, std::move(endIdx));
}

// ============================================== RFC ===============================
Rfc::Rfc(
    const std::shared_ptr<cfg::GlobalCfg> & cfg, 
    const std::shared_ptr<stat::RfcStat> & scbBase, 
    const std::shared_ptr<stat::RfcStat> & scb 
) : cfg(cfg), scbBase(scbBase), scb(scb), cam(std::make_unique<Cam>(cfg->assoc, cfg->nBlk, cfg->nDW)) {} 

void Rfc::step() noexcept {
    cam->step();
}

// Cache bank transaction count
uint32_t Rfc::bankTxCnt(const std::bitset<32>& buf) {
    uint32_t acc = 0;
    auto nLane = cfg->bw / 32;
    for (auto i = 0; i < 32; i += nLane) {
        for (auto j = i; j < i + nLane; j++) {
            if (buf[j])  {
                acc++;
                break;
            }
        } 
    }
    return acc;
}

uint32_t Rfc::sMap(const reg::Oprd& oprd) noexcept {
    
    // Map source operands based on index
    if (oprd.type == reg::OprdT::src)
        return oprd.pos % (cfg->nBlk / cfg->assoc);

    // Map destination operands based on index
    else if (oprd.type == reg::OprdT::dst) {
        if (cfg->dMap == cfg::DestMap::ln)
            return (oprd.index / cfg->nDW) * cfg->nBlk / (256 * cfg->assoc);
        else if (cfg->dMap == cfg::DestMap::itl) 
            return (oprd.index / cfg->nDW) % (cfg->nBlk / cfg->assoc);
    }

    return 0;
}

std::pair<bool, uint32_t> Rfc::search(const reg::Oprd & oprd, uint32_t tid, uint32_t setId) {
    return cam->search(tid, oprd.index / cfg->nDW, setId);
}

// FSM state transition
void Cam::sync() {
    mem = vMem;
}

void Rfc::sync() {
    cam->sync();
}

void Rfc::flushSimdBuf() {
    for (auto & bs: simdBuf) {
        bs.reset();
    }
}

void Rfc::exec(const TraceInst & inst) {
    step();
    
    flags = inst.reuseFlag;
    mask = inst.mask;

    // CC Execution Flow 
    for (const auto & oprd : inst.regPool) {
        auto tp = oprd.type;
        if (tp == reg::OprdT::addr)  continue;

        for (auto tid = 0; tid < 32; tid++) {
            if (!mask[31 - tid])
                continue;

            (oprd.type == reg::OprdT::src) ? scbBase->trigger(stat::Event::mrfRd) : scbBase->trigger(stat::Event::mrfWr);

            std::pair<bool, uint32_t> s;
            s = search(oprd, tid, sMap(oprd));

            if (!s.first) {
                (oprd.type == reg::OprdT::src) ? 
                    scb->trigger(stat::Event::rdMiss) : scb->trigger(stat::Event::wrMiss);

                allocWrapper(oprd, tid);
            }

            else {
                hitHandler(oprd, tid, s.second);
            }
        }

        // Synchronize warp
        scb->trigger(stat::Event::rfcRd, bankTxCnt(simdBuf.at(0)));
        scb->trigger(stat::Event::rfcWr, bankTxCnt(simdBuf.at(1)));
        scb->trigger(stat::Event::mrfRd, simdBuf.at(2).count());
        scb->trigger(stat::Event::mrfWr, simdBuf.at(3).count());

        flushSimdBuf();
    }
    sync();
}

void Rfc::execTC(const TraceInst & inst) {
    step();
    
    flags = inst.reuseFlag;
    mask = inst.mask;

    // CC Execution Flow 
    for (const auto & oprd : inst.regPool) {
        auto tp = oprd.type;
        if (tp == reg::OprdT::addr)  continue;

        for (auto tid = 0; tid < 32; tid++) {
            if (!mask[31 - tid])
                continue;

            (oprd.type == reg::OprdT::src) ? scbBase->trigger(stat::Event::mrfRd) : scbBase->trigger(stat::Event::mrfWr);

            std::pair<bool, uint32_t> s;
            s = search(oprd, tid, sMap(oprd));

            if (!s.first) {
                (oprd.type == reg::OprdT::src) ? 
                    scb->trigger(stat::Event::rdMiss) : scb->trigger(stat::Event::wrMiss);

                // Dedicated Allocation Logic
                if (oprd.type == reg::OprdT::src) {
                    if (flags.test(3 - oprd.pos)) {
                        auto p = replWrapper(tid, sMap(oprd));
                        cam->vMem[tid].at(p.second).set(oprd.index / cfg->nDW, 1, false);

                        simdBuf.at(1).set(tid); // RFC.W
                        simdBuf.at(2).set(tid); // MRF.R

                        if (p.first) simdBuf.at(3).set(tid); // MRF.W;
                    }
                    else
                        simdBuf.at(2).set(tid); // MRF.R
                }
                else if (oprd.type == reg::OprdT::dst) {
                    simdBuf.at(3).set(tid); // MRF.W;
                }
            }

            else {
                hitHandler(oprd, tid, s.second);
            }
        }

        // Synchronize warp
        scb->trigger(stat::Event::rfcRd, bankTxCnt(simdBuf.at(0)));
        scb->trigger(stat::Event::rfcWr, bankTxCnt(simdBuf.at(1)));
        scb->trigger(stat::Event::mrfRd, simdBuf.at(2).count());
        scb->trigger(stat::Event::mrfWr, simdBuf.at(3).count());

        flushSimdBuf();
    }
    sync();
}


inline void Rfc::hitHandler(const reg::Oprd& oprd, uint32_t tid, uint32_t idx) {
    
    if (oprd.type == reg::OprdT::src) {
        scb->trigger(stat::Event::rdHit);
        auto age = cfg->repl == cfg::ReplPlcy::lru ? 1 : cam->vMem[tid].at(idx).age;
        cam->vMem[tid].at(idx).set(cam->vMem[tid].at(idx).tag, age, false);
        simdBuf.at(0).set(tid); // RFC.R
    }
    
    else if (oprd.type == reg::OprdT::dst) {
        scb->trigger(stat::Event::wrHit);
        simdBuf.at(1).set(tid);
        
        if (cfg->ev == cfg::EvictPlcy::writeThrough) {
            simdBuf.at(3).set(tid); // MRF.W
            cam->vMem[tid].at(idx).set(cam->vMem[tid].at(idx).tag, 1, false);
        }
        else if (cfg->ev == cfg::EvictPlcy::writeBack)
            cam->vMem[tid].at(idx).set(cam->vMem[tid].at(idx).tag, 1, true);
    }
}

std::pair<bool, uint32_t> Rfc::replWrapper(uint32_t tid, uint32_t setId) {
    uint32_t start = setId * cfg->assoc;
    uint32_t end = start + cfg->assoc;
    uint32_t maxAge = 0;
    uint32_t maxPos = 0;

    for (auto i = start; i < end; i++) {
        if (cam->vMem[tid].at(i).tag == 256) // if empty
            return std::make_pair<bool, uint32_t>(false, std::move(i));

        if (cam->vMem[tid].at(i).age > maxAge) {
            maxAge = cam->vMem[tid].at(i).age;
            maxPos = i;
        }
    }

    return std::make_pair<bool, uint32_t>(std::move(cam->vMem[tid].at(maxPos).dt), std::move(maxPos));
}

void Rfc::allocWrapper(const reg::Oprd & oprd, uint32_t tid) {
    switch(cfg->alloc) {
        case(cfg::AllocPlcy::cplAidedAlloc): return cplAidedAlloc(oprd, tid);
        case(cfg::AllocPlcy::readAlloc): return readAlloc(oprd, tid);
        case(cfg::AllocPlcy::writeAlloc): return writeAlloc(oprd, tid);
        default: return cplAidedAlloc(oprd, tid);
    }
}

void Rfc::readAlloc(const reg::Oprd & oprd, uint32_t tid) {
    if (oprd.type == reg::OprdT::src) {
        auto p = replWrapper(tid, sMap(oprd)); 
        cam->vMem[tid].at(p.second).set(oprd.index / cfg->nDW, 1, false);

        simdBuf.at(1).set(tid); // RFC.W
        simdBuf.at(2).set(tid); // MRF.R

        if (p.first) simdBuf.at(3).set(tid); // MRF.W
    } 
    else if (oprd.type == reg::OprdT::dst) {
        simdBuf.at(3).set(tid); // MRF.W
    }
}

void Rfc::writeAlloc(const reg::Oprd & oprd, uint32_t tid) {
    if (oprd.type == reg::OprdT::dst) {
        auto p = replWrapper(tid, sMap(oprd));
        cam->vMem[tid].at(p.second).set(oprd.index / cfg->nDW, 1, true);
        
        simdBuf.at(1).set(tid); // RFC.W
        if (p.first) simdBuf.at(3).set(tid); // MRF.W;
    }
    else if (oprd.type == reg::OprdT::src)
        simdBuf.at(2).set(tid); // MRF.R
}

void Rfc::cplAidedAlloc(const reg::Oprd & oprd, uint32_t tid) {
    if (oprd.type == reg::OprdT::src) {
        if (flags.test(3 - oprd.pos)) {
            auto p = replWrapper(tid, sMap(oprd));
            cam->vMem[tid].at(p.second).set(oprd.index / cfg->nDW, 1, false);

            simdBuf.at(1).set(tid); // RFC.W
            simdBuf.at(2).set(tid); // MRF.R

            if (p.first) simdBuf.at(3).set(tid); // MRF.W;
        }
        else
            simdBuf.at(2).set(tid); // MRF.R
    }
    else if (oprd.type == reg::OprdT::dst) {
        auto p = replWrapper(tid, sMap(oprd));
        cam->vMem[tid].at(p.second).set(oprd.index / cfg->nDW, 1, true);
        simdBuf.at(1).set(tid); // RFC.W
        if (p.first) simdBuf.at(3).set(tid); // MRF.W;
    }
}

std::ostream & operator<<(std::ostream & os, const Rfc & cc) {
    os << *(cc.cam);
    return os;
}
