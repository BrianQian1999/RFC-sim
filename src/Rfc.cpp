#include "Rfc.h"

RfcEntry::RfcEntry() {
    tag = 256;
    lruAge = 0;
    fifoAge = 0;
    isDirty = false;
}

std::ostream & operator<<(std::ostream & os, const RfcEntry & e) {
    os << "(" << e.tag << "|" << e.lruAge << "|" << e.fifoAge << "|" << e.isDirty << ")";
    return os;
}

void RfcEntry::clear() noexcept {
    tag = 256;
    lruAge = 0;
    fifoAge = 0;
    isDirty = false;
}

void RfcEntry::aging() noexcept {
    this->lruAge++;
    this->fifoAge++;
}

void RfcEntry::set(uint32_t tag, uint32_t lruAge, uint32_t fifoAge, bool isDirty) noexcept {
    this->tag = tag;
    this->lruAge = lruAge;
    this->fifoAge = fifoAge;
    this->isDirty = isDirty;
}

// struct Cam
std::ostream & operator<<(std::ostream & os, const Cam & cam) {
    os << "Cam line: ";
    for (auto & e : cam.mem)
        os << e << " ";
    os << "\n";
    return os;
}

void Cam::flush() noexcept {
    for (auto & e : mem) {
        e.clear();
    }
}

void Cam::aging() noexcept {
    for (auto & e : mem)
        e.aging();
}

std::pair<bool, uint32_t> Cam::search(uint32_t addr) noexcept {
    size_t idx = 0;
    for (const auto & e : mem) {
        if (e.tag == addr)
            return std::make_pair<bool, uint32_t>(true, std::move(idx));
        idx++;
    }    
    return std::make_pair<bool, uint32_t>(false, std::move(idx));
}

std::pair<bool, uint32_t> Cam::search(uint32_t addr, uint32_t setId) noexcept {
    uint32_t startIdx = setId * assoc;
    uint32_t endIdx = startIdx + assoc;
    
    for (auto i = startIdx; i < endIdx; i++) {
        const auto & e = mem.at(i);
        if (e.tag == addr)
            return std::make_pair<bool, uint32_t>(true, std::move(i));
    }    
    return std::make_pair<bool, uint32_t>(false, std::move(endIdx));
}

// ============================================== RFC ===============================
Rfc::Rfc(
    const std::shared_ptr<cfg::GlobalCfg> & cfg, 
    const std::shared_ptr<stat::RfcStat> & scb, 
    const std::shared_ptr<Mrf> & mrf 
) : cfg(cfg), scb(scb), mrf(mrf), cam(std::make_unique<Cam>(cfg->assoc)) {} 

void Rfc::aging() noexcept {
    cam->aging();
}

uint32_t Rfc::cnt() {
    auto n = cfg->bitWidth / 32;
    uint32_t acc = 0;
    for (auto i = 0; i < 32; i += n) {
        if (mask.to_string().substr(i, n) != "0000")
            acc++;
    }    
    return acc;
}

uint32_t Rfc::setIdx(const reg::Oprd& oprd) noexcept {
    if (oprd.type == reg::OprdT::SRC)
        return oprd.pos % (8 / cfg->assoc);
    else if (oprd.type == reg::OprdT::DST) {
        switch(cfg->allocPlcy) {
        case(cfg::AllocPlcy::cplAidedAlloc):
            return oprd.index / 64;
        case(cfg::AllocPlcy::cplAidedItlAlloc):
            return oprd.index % (8 / cfg->assoc);
        default:
            return 0;
        }
    }
    return 0;
}

std::pair<bool, uint32_t> Rfc::search(const reg::Oprd & oprd) {
    return cam->search(oprd.index);
}

std::pair<bool, uint32_t> Rfc::search(const reg::Oprd & oprd, uint32_t setId) {
    return cam->search(oprd.index, setId);
}

void Rfc::exec(const TraceInst & inst) {
    aging();
    flags = inst.reuseFlag;
    mask = inst.mask;
    for (const auto & oprd : inst.regPool) {
        auto tp = oprd.type;
        if (tp == reg::OprdT::ADDR)
            continue;

        auto sp = search(oprd, setIdx(oprd));
        
        if (!sp.first) { // cache miss
            scb->accMiss(tp == reg::OprdT::SRC);
            allocWrapper(oprd);
        }
        else {
            scb->accHit(tp == reg::OprdT::SRC);
            if (tp == reg::OprdT::SRC) {
                scb->rfcRdNum += cnt();
                cam->mem.at(sp.second).set(cam->mem.at(sp.second).tag, 1, cam->mem.at(sp.second).fifoAge, false);
            }
            else {
                if (cfg->evictPlcy == cfg::EvictPlcy::writeBack) {
                    scb->mrfWrNum += mask.count();
                    cam->mem.at(sp.second).set(cam->mem.at(sp.second).tag, 1, 1, false);
                }
                else if (cfg->evictPlcy == cfg::EvictPlcy::writeThrough) {
                    cam->mem.at(sp.second).set(cam->mem.at(sp.second).tag, 1, 1, true);
                }
            }
        } 
    }
}

std::pair<bool, uint32_t> Rfc::replWrapper(uint32_t setId) noexcept {
    switch (cfg->replPlcy)
    {
    case(cfg::ReplPlcy::lru):
        return lruRepl(setId);
    case(cfg::ReplPlcy::fifo):
        return fifoRepl(setId);
    default:
        return lruRepl(setId);
    }
}

std::pair<bool, uint32_t> Rfc::lruRepl(uint32_t setId) noexcept {
    uint32_t start = setId * cam->assoc;
    uint32_t end = start + cam->assoc;
    uint32_t maxAge = 0;
    uint32_t maxPos = 0;

    for (auto i = start; i < end; i++) {
        if (cam->mem.at(i).tag == 256) // if empty
            return std::make_pair<bool, uint32_t>(false, std::move(i));
        if (cam->mem.at(i).lruAge > maxAge) {
            maxAge = cam->mem.at(i).lruAge;
            maxPos = i;
        }
    }

    bool dirty = cam->mem.at(maxPos).isDirty;
    return std::make_pair<bool, uint32_t>(std::move(dirty), std::move(maxPos));
}

std::pair<bool, uint32_t> Rfc::fifoRepl(uint32_t setId) noexcept {
    uint32_t start = setId * cam->assoc;
    uint32_t end = start + cam->assoc;
    uint32_t maxAge = 0;
    uint32_t maxPos = 0;

    for (auto i = start; i < end; i++) {
        if (cam->mem.at(i).tag == 256) // if empty
            return std::make_pair<bool, uint32_t>(false, std::move(i));
        if (cam->mem.at(i).fifoAge > maxAge) {
            maxAge = cam->mem.at(i).fifoAge;
            maxPos = i;
        }
    }
    bool dirty = cam->mem.at(maxPos).isDirty;
    return std::make_pair<bool, uint32_t>(std::move(dirty), std::move(maxPos));
}

void Rfc::allocWrapper(const reg::Oprd & oprd) {
    switch(cfg->allocPlcy) {
    case(cfg::AllocPlcy::cplAidedAlloc):
        return cplAidedAlloc(oprd);
    case(cfg::AllocPlcy::readAlloc):
        return readAlloc(oprd);
    case(cfg::AllocPlcy::writeAlloc):
        return writeAlloc(oprd);
    case(cfg::AllocPlcy::cplAidedItlAlloc):
        return cplAidedItlAlloc(oprd);
    default:
        return cplAidedAlloc(oprd);
    }
}

void Rfc::readAlloc(const reg::Oprd & oprd) {
    if (oprd.type == reg::OprdT::SRC) {
        auto p = replWrapper(setIdx(oprd)); // full-associate
        cam->mem.at(p.second).set(oprd.index, 1, 1, false);

        // read from mrf, write to rfc
        scb->rfcWrNum += cnt();
        scb->mrfRdNum += mask.count();

        if (p.first) scb->mrfWrNum += mask.count();
    } 
    else if (oprd.type == reg::OprdT::DST) {
        scb->mrfWrNum += mask.count();
    }
}

void Rfc::writeAlloc(const reg::Oprd & oprd) {
    if (oprd.type == reg::OprdT::DST) {
        auto p = replWrapper(setIdx(oprd));
        cam->mem.at(p.second).set(oprd.index, 1, 1, true);
        scb->rfcWrNum += cnt();
        if (p.first) scb->mrfWrNum += mask.count();
    }
    else if (oprd.type == reg::OprdT::SRC) {
        scb->mrfRdNum += mask.count();
    }
}


void Rfc::cplAidedAlloc(const reg::Oprd & oprd) {
    if (oprd.type == reg::OprdT::SRC) {
        if (flags.test(3 - oprd.pos)) {
            auto p = replWrapper(setIdx(oprd));
            cam->mem.at(p.second).set(oprd.index, 1, 1, false);
        
            scb->rfcWrNum += cnt();
            scb->mrfRdNum += mask.count();

            if (p.first)
                scb->mrfWrNum += mask.count(); 
        }
        else {
            scb->mrfRdNum += mask.count();
        }
    }
    else if (oprd.type == reg::OprdT::DST) {
        auto p = replWrapper(setIdx(oprd));
        cam->mem.at(p.second).set(oprd.index, 1, 1, true);
        scb->rfcWrNum += cnt();
    }
}

void Rfc::cplAidedItlAlloc(const reg::Oprd& oprd) {
    if (oprd.type == reg::OprdT::SRC) {
        if (flags.test(3 - oprd.pos)) {
            auto p = replWrapper(setIdx(oprd));
            cam->mem.at(p.second).set(oprd.index, 1, 1, false);
        
            scb->rfcWrNum += cnt();
            scb->mrfRdNum += mask.count();

            if (p.first)
                scb->mrfWrNum += mask.count(); 
        }
        else {
            scb->mrfRdNum += mask.count();
        }
    }
    else if (oprd.type == reg::OprdT::DST) {
        auto p = replWrapper(setIdx(oprd));
        cam->mem.at(p.second).set(oprd.index, 1, 1, true);
        scb->rfcWrNum += cnt();
    }
}

std::ostream & operator<<(std::ostream & os, const Rfc & rfc) {
    os << *(rfc.cam);
    return os;
}
