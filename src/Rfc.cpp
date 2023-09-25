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

void RfcBlock::aging() noexcept {
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

void Cam::aging() {
    for (auto & blk : vMem) {
        for (auto & e : blk) e.aging();
    }
}

std::pair<bool, uint32_t> Cam::search(uint32_t tid, uint32_t tag) {
    size_t idx = 0;
    for (const auto & e : mem.at(tid)) {
        if (e.tag == tag)
            return std::make_pair<bool, uint32_t>(true, std::move(idx));
        idx++;
    }    
    return std::make_pair<bool, uint32_t>(false, std::move(idx));
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
    const std::shared_ptr<stat::RfcStat> & scb, 
    const std::shared_ptr<Mrf> & mrf 
) : cfg(cfg), scb(scb), mrf(mrf), cam(std::make_unique<Cam>(cfg->assoc, cfg->nBlock, cfg->nDW)) {} 

void Rfc::aging() noexcept {
    cam->aging();
}

uint32_t Rfc::ioCount(const std::bitset<32>& buf) {
    uint32_t acc = 0;
    auto nLane = cfg->bitWidth / 32;
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

uint32_t Rfc::mapSet(const reg::Oprd& oprd) noexcept {
    if (oprd.type == reg::OprdT::SRC)
        return oprd.pos % (cfg->nBlock / cfg->assoc);
    else if (oprd.type == reg::OprdT::DST) {
        switch(cfg->allocPlcy) {
            case(cfg::AllocPlcy::cplAidedAlloc): return oprd.index / cfg->nDW / 64;
            case(cfg::AllocPlcy::cplAidedItlAlloc): return oprd.index / cfg->nDW % (cfg->nBlock / cfg->assoc);
            default: return 0;
        }
    }
    return 0;
}

std::pair<bool, uint32_t> Rfc::search(const reg::Oprd & oprd, uint32_t tid) {
    return cam->search(tid, oprd.index / cfg->nDW); // Map addr -> tag
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

void Rfc::flushIOBuffer() {
    for (auto & bs: ioBuffer) {
        bs.reset();
    }
}

void Rfc::exec(const TraceInst & inst) {
    aging();
    
    flags = inst.reuseFlag;
    mask = inst.mask;

	// Dedicated Policy for TC instructions
    /*
    if (inst.opcode == op::OP_IMMA || inst.opcode == op::OP_HMMA || inst.opcode == op::OP_BMMA) {
        for (const auto & oprd : inst.regPool) {
            auto tp = oprd.type;
            for (auto tid = 0; tid < 32; tid++) {
                if (!mask[tid])
                    continue;
                auto s = search(oprd, tid, mapSet(oprd));
                if (!s.first) {
                    oprd.type == reg::OprdT::SRC ? scb->trigger(stat::Event::RD_MISS) : scb->trigger(stat::Event::WR_MISS);
                    
                    // Custom allocation for TC
                    if (oprd.type == reg::OprdT::SRC) {
                        if (flags.test(3 - oprd.pos)) {
                            auto p = replWrapper(tid, mapSet(oprd));
                            cam->vMem[tid].at(p.second).set(oprd.index / cfg->nDW, 1, false);
                            ioBuffer.at(1).set(tid); // RFC.W
                            ioBuffer.at(2).set(tid); // MRF.R
                            if (p.first) ioBuffer.at(3).set(tid); // MRF.W;
                        }
                        else 
                            ioBuffer.at(2).set(tid); // MRF.R
                    }
                    else if (oprd.type == reg::OprdT::DST)
                        ioBuffer.at(3).set(tid); // Bypass RFC
                }
                else
                    ccHitHandler(oprd, tid, s.second);
            }

            // Synchronize warp
            scb->trigger(stat::Event::CC_RD, ioCount(ioBuffer.at(0)));
            scb->trigger(stat::Event::CC_WR, ioCount(ioBuffer.at(1)));
            scb->trigger(stat::Event::RF_RD, ioBuffer.at(2).count());
            scb->trigger(stat::Event::RF_WR, ioBuffer.at(3).count());
            flushIOBuffer();
        }
        sync();
        return;
    }
    */

    // CC Execution Flow 
    for (const auto & oprd : inst.regPool) {
        auto tp = oprd.type;
        if (tp == reg::OprdT::ADDR)  continue;

        for (auto tid = 0; tid < 32; tid++) {
            if (!mask[tid])
                continue;
            auto s = search(oprd, tid, mapSet(oprd));
            if (!s.first) {
                oprd.type == reg::OprdT::SRC ? scb->trigger(stat::Event::RD_MISS) : scb->trigger(stat::Event::WR_MISS);
                allocWrapper(oprd, tid);
            }
            else
                ccHitHandler(oprd, tid, s.second);
        }

        // Synchronize warp
        scb->trigger(stat::Event::CC_RD, ioCount(ioBuffer.at(0)));
        scb->trigger(stat::Event::CC_WR, ioCount(ioBuffer.at(1)));
        scb->trigger(stat::Event::RF_RD, ioBuffer.at(2).count());
        scb->trigger(stat::Event::RF_WR, ioBuffer.at(3).count());
        flushIOBuffer();
    }
    sync();
}

inline void Rfc::ccHitHandler(const reg::Oprd& oprd, uint32_t tid, uint32_t idx) {
    if (oprd.type == reg::OprdT::SRC) {
        scb->trigger(stat::Event::RD_HIT);
        auto age = cfg->replPlcy == cfg::ReplPlcy::lru ? 1 : cam->vMem[tid].at(idx).age;
        cam->vMem[tid].at(idx).set(cam->vMem[tid].at(idx).tag, age, false);
        ioBuffer.at(0).set(tid); // RFC.R
    }
    else if (oprd.type == reg::OprdT::DST) {
        scb->trigger(stat::Event::WR_HIT);
        ioBuffer.at(1).set(tid);
        
        if (cfg->evictPlcy == cfg::EvictPlcy::writeThrough) {
            ioBuffer.at(3).set(tid); // MRF.W
            cam->vMem[tid].at(idx).set(cam->vMem[tid].at(idx).tag, 1, false);
        }
        else if (cfg->evictPlcy == cfg::EvictPlcy::writeBack)
            cam->vMem[tid].at(idx).set(cam->vMem[tid].at(idx).tag, 1, true);
    }
}

std::pair<bool, uint32_t> Rfc::replWrapper(uint32_t tid, uint32_t setId) {
    uint32_t start = setId * cam->assoc;
    uint32_t end = start + cam->assoc;
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

    bool dirty = cam->vMem[tid].at(maxPos).dt;
    return std::make_pair<bool, uint32_t>(std::move(dirty), std::move(maxPos));
}

void Rfc::allocWrapper(const reg::Oprd & oprd, uint32_t tid) {
    switch(cfg->allocPlcy) {
    case(cfg::AllocPlcy::cplAidedAlloc): return cplAidedAlloc(oprd, tid);
    case(cfg::AllocPlcy::cplAidedItlAlloc): return cplAidedItlAlloc(oprd, tid);
    case(cfg::AllocPlcy::readAlloc): return readAlloc(oprd, tid);
    case(cfg::AllocPlcy::writeAlloc): return writeAlloc(oprd, tid);
    default: return cplAidedAlloc(oprd, tid);
    }
}

void Rfc::readAlloc(const reg::Oprd & oprd, uint32_t tid) {
    if (oprd.type == reg::OprdT::SRC) {
        auto p = replWrapper(tid, mapSet(oprd)); 
        cam->vMem[tid].at(p.second).set(oprd.index / cfg->nDW, 1, false);

        ioBuffer.at(1).set(tid); // RFC.W
        ioBuffer.at(2).set(tid); // MRF.R

        if (p.first) ioBuffer.at(3).set(tid); // MRF.W
    } 
    else if (oprd.type == reg::OprdT::DST) {
        ioBuffer.at(3).set(tid); // MRF.W
    }
}

void Rfc::writeAlloc(const reg::Oprd & oprd, uint32_t tid) {
    if (oprd.type == reg::OprdT::DST) {
        auto p = replWrapper(tid, mapSet(oprd));
        cam->vMem[tid].at(p.second).set(oprd.index / cfg->nDW, 1, true);
        
        ioBuffer.at(1).set(tid); // RFC.W
        if (p.first) ioBuffer.at(3).set(tid); // MRF.W;
    }
    else if (oprd.type == reg::OprdT::SRC)
        ioBuffer.at(2).set(tid); // MRF.R
}


void Rfc::cplAidedAlloc(const reg::Oprd & oprd, uint32_t tid) {
    if (oprd.type == reg::OprdT::SRC) {
        if (flags.test(3 - oprd.pos)) {
            auto p = replWrapper(tid, mapSet(oprd));
            cam->vMem[tid].at(p.second).set(oprd.index / cfg->nDW, 1, false);

            ioBuffer.at(1).set(tid); // RFC.W
            ioBuffer.at(2).set(tid); // MRF.R

            if (p.first) ioBuffer.at(3).set(tid); // MRF.W;
        }
        else
            ioBuffer.at(2).set(tid); // MRF.R
    }
    else if (oprd.type == reg::OprdT::DST) {
        auto p = replWrapper(tid, mapSet(oprd));
        cam->vMem[tid].at(p.second).set(oprd.index / cfg->nDW, 1, true);
        ioBuffer.at(1).set(tid); // RFC.W
        if (p.first) ioBuffer.at(3).set(tid); // MRF.W;
    }
}

void Rfc::cplAidedItlAlloc(const reg::Oprd& oprd, uint32_t tid) {
    if (oprd.type == reg::OprdT::SRC) {
        if (flags.test(3 - oprd.pos)) {
            auto p = replWrapper(tid, mapSet(oprd));
            cam->vMem[tid].at(p.second).set(oprd.index / cfg->nDW, 1, false);
        
            ioBuffer.at(1).set(tid); // RFC.W
            ioBuffer.at(2).set(tid); // MRF.R
            if (p.first) ioBuffer.at(3).set(tid); // MRF.W;
        }
        else 
            ioBuffer.at(2).set(tid); // MRF.R
    }
    else if (oprd.type == reg::OprdT::DST) {
        auto p = replWrapper(tid, mapSet(oprd));
        cam->vMem[tid].at(p.second).set(oprd.index / cfg->nDW, 1, true);
        ioBuffer.at(1).set(tid); // RFC.W
        if (p.first) ioBuffer.at(3).set(tid); // MRF.W;
    }
}

std::ostream & operator<<(std::ostream & os, const Rfc & cc) {
    os << *(cc.cam);
    return os;
}
