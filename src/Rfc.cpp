#include "Rfc.h"

RfcEntry::RfcEntry() {
    tag = 0;
    lruAge = 0;
    fifoAge = 0;
    isDirty = 0;
}

std::ostream & operator<<(std::ostream & os, const RfcEntry & e) {
    os << "(" << e.tag << "|" << e.lruAge << "|" << e.fifoAge << "|" << e.isDirty << ")";
    return os;
}

void RfcEntry::clear() noexcept {
    tag = 0;
    lruAge = 0;
    fifoAge = 0;
    isDirty = true;
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

std::pair<bool, uint32_t> Cam::search(uint32_t addr, uint32_t setId) noexcept {
    uint32_t startIdx = 8 / assoc * setId;
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

std::pair<bool, uint32_t> Rfc::search(const reg::RegOprd & oprd, uint32_t setId) {
    return cam->search(oprd.regIndex, setId);
}

void Rfc::exec(const TraceInst & inst) {
    aging();
    flags = inst.reuseFlag;
    mask = inst.mask;
    for (const auto & oprd : inst.regPool) {
        auto & tp = oprd.regType;
        if (tp == reg::RegOprdT::ADDR) continue;
        
        auto sp = search(oprd, oprd.regPos % (8 / cam->assoc));
        if (!sp.first) { // cache miss
            scb->accMiss(tp == reg::RegOprdT::SRC);
            allocWrapper(oprd);
        }
        else { // cache hit
            if (tp == reg::RegOprdT::SRC) {
                scb->accHit(true);
                scb->rfcRdNum += mask.count();
                cam->mem.at(sp.second).set(cam->mem.at(sp.second).tag, 1, cam->mem.at(sp.second).fifoAge, false);
            }
            else if (tp == reg::RegOprdT::DST) {
                scb->accHit(false);
                scb->rfcWrNum += mask.count();
                cam->mem.at(sp.second).set(cam->mem.at(sp.second).tag, 1, 1, true);
            }
        }
    }
}

void Rfc::evict() noexcept {
    scb->mrfWrNum += mask.count();
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
    uint32_t start = setId / cam->assoc;
    uint32_t end = start + cam->assoc;
    uint32_t maxAge = 0;
    uint32_t maxPos = 0;

    for (auto i = start; i < end; i++) {
        if (cam->mem.at(i).tag == 0) // if empty
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
    uint32_t start = setId / cam->assoc;
    uint32_t end = start + cam->assoc;
    uint32_t maxAge = 0;
    uint32_t maxPos = 0;

    for (auto i = start; i < end; i++) {
        if (cam->mem.at(i).tag == 0) // if empty
            return std::make_pair<bool, uint32_t>(false, std::move(i));
        if (cam->mem.at(i).fifoAge > maxAge) {
            maxAge = cam->mem.at(i).fifoAge;
            maxPos = i;
        }
    }
    bool dirty = cam->mem.at(maxPos).isDirty;
    return std::make_pair<bool, uint32_t>(std::move(dirty), std::move(maxPos));
}

void Rfc::allocWrapper(const reg::RegOprd & oprd) {
    switch(cfg->allocPlcy) {
    case(cfg::AllocPlcy::fullCplAlloc):
        return fullCplAlloc(oprd);
    case(cfg::AllocPlcy::readAlloc):
        return readAlloc(oprd);
    case(cfg::AllocPlcy::writeAlloc):
        return writeAlloc(oprd);
    default:
        return fullCplAlloc(oprd);
    }
}

void Rfc::fullCplAlloc(const reg::RegOprd & oprd) {
    if (oprd.regType == reg::RegOprdT::DST) {
        scb->mrfWrNum += mask.count();
    } 
    else if (oprd.regType == reg::RegOprdT::SRC) {
        if (flags[oprd.regPos]) {
            auto p = replWrapper(oprd.regPos % (8 / cam->assoc));
            cam->mem.at(p.second).set(oprd.regIndex, 1, 1, false);
        
            scb->rfcWrNum += mask.count();
            scb->mrfRdNum += mask.count();

            if (p.first) evict(); 
        }
        else {
            scb->mrfRdNum += mask.count();
        }
    }
    else return;
}

void Rfc::readAlloc(const reg::RegOprd & oprd) {
    if (oprd.regType == reg::RegOprdT::SRC) {
        auto p = replWrapper(oprd.regPos % (8 / cam->assoc)); // full-associate
        cam->mem.at(p.second).set(oprd.regIndex, 1, 1, false);

        // read from mrf, write to rfc
        scb->rfcWrNum += mask.count();
        scb->mrfRdNum += mask.count();

        if (p.first) evict();
    } 
    else if (oprd.regType == reg::RegOprdT::DST) {
        scb->mrfWrNum += mask.count();
    }
}

void Rfc::writeAlloc(const reg::RegOprd & oprd) {
    if (oprd.regType == reg::RegOprdT::SRC) {
        auto p = replWrapper(oprd.regPos % (8 / cam->assoc));
        cam->mem.at(p.second).set(oprd.regIndex, 1, 1, false);
        
        scb->rfcWrNum += mask.count();
        scb->mrfRdNum += mask.count();        
    
        if (p.first) evict();
    }
    else if (oprd.regType == reg::RegOprdT::DST) {
        auto p = replWrapper(0);
        cam->mem.at(p.second).set(oprd.regIndex, 1, 1, true);
        scb->rfcWrNum += mask.count();
        if (p.first) evict();
    }
}

void Rfc::writeOnlyAlloc(const reg::RegOprd & oprd) {
    if (oprd.regType == reg::RegOprdT::DST) {
        auto p = replWrapper(oprd.regPos % (8 / cam->assoc)); // full-associate
        cam->mem.at(p.second).set(oprd.regIndex, 1, 1, false);

        // read from mrf, write to rfc
        scb->rfcWrNum += mask.count();
        scb->mrfRdNum += mask.count();

        if (p.first) evict();
    } 
    else if (oprd.regType == reg::RegOprdT::SRC) {
        scb->mrfRdNum += mask.count();
    }
}

std::ostream & operator<<(std::ostream & os, const Rfc & rfc) {
    os << *(rfc.cam);
    return os;
}