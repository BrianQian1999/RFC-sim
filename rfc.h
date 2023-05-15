// Register File Cache (RFC) for NVIDIA GPGPU
// Qiran Qian, <qiranq@kth.se>

#ifndef RFC_H
#define RFC_H

#include "reg_operand.h"
#include "trace_inst.h"
#include "mrf.h"

#include <iostream>
#include <vector>
#include <array>
#include <memory>
#include <limits>
#include <stdexcept>
#include <algorithm>

// Replacement Policy
enum REPOLICY {
    LRU = 0,
    FIFO
};

struct RfcEntry {
    RfcEntry() { 
        this->index = std::numeric_limits<unsigned int>::max(); 
        this->age = 0; 
        this->dirty = true;
    } 
    unsigned index;
    unsigned long age;
    bool dirty;
};

// Basically, we have warp 0-31, assigned to 4 warp schedulers (4 Sub-Core),
// each scheduler handles 8 warps. 
// A Rfc object corresponds to a single scheduler 
class Rfc {
private:
    std::array<std::vector<RfcEntry>, 8> __cache;
    std::shared_ptr<GlobalCfg> __cfg_ptr;
    std::shared_ptr<Mrf> __mrf_ptr;

    // Accessors
    const std::vector<RfcEntry> & RfCache() const { return this->__cache; }
    const std::shared_ptr<Mrf> & MrfPtr() const { return this->__mrf_ptr; }
    const std::shared_ptr<GlobalCfg> & CfgPtr() const { return this->__cfg_ptr; }

public:
    explicit Rfc(const std::shared_ptr<GlobalCfg> & cfg_ptr, const std::shared_ptr<Mrf> & mrf_ptr) 
        : __cfg_ptr(cfg_ptr), __mrf_ptr(mrf_ptr) { 
        for(auto i : this->__cache) {
            i.resize(cfg_ptr->numEntry);
        }
    }
    virtual ~Rfc() {}

    // Flush Rfc
    void Flush() {
        for(auto cacheLine : this->RfCache()) {
            for(auto entry : cacheLine) {
                entry.index = std::numeric_limits<unsigned int>::max();
                entry.age = 0;
                entry.dirty = true;
            }
        }
    }

    // Aging Rfc entries
    void Aging() {
        for(auto cacheLine : this->RfCache()) {
            for(auto entry : cacheLine) {
                if(entry.index != std::numeric_limits<unsigned int>::max() && !entry.dirty) {
                    if(entry.age < std::numeric_limits<unsigned long>::max()) {
                        entry.age++;
                    }
                }
            }
        }
    }
    
    // If the RFC entries are full
    inline bool Full(size_t lineIdx) const {
        for(const auto & e : this->RfCache()[lineIdx]) {
            if(e.dirty) return false;
        }
        return true;
    }

    // Return the index of first empty entry
    size_t EmptyEntryPos(size_t lineIdx) const { 
        if(this->Full(lineIdx)) throw std::runtime_error("[Rfc.EmptyEntryPos] Runtime error: RFC full.");

        for(size_t i = 0; i < this->RfCache()[lineIdx].size(); i++)  {
            if(this->RfCache()[lineIdx][i].index == std::numeric_limits<unsigned int>::max()
                    || this->RfCache()[lineIdx][i].dirty) { // dirty or simply empty
                return i;
            }
        }

        return this->CfgPtr()->numEntry; // No empty entry
    }

    // Return the oldest age value
    unsigned OldestAge(size_t lineIdx) const {
        unsigned maxAge = 0;
        for(const auto & entry : this->RfCache()[lineIdx]) {
            if(entry.age > maxAge || !entry.dirty) maxAge = entry.age;
        }
        return maxAge;
    }

    size_t OldestAgePos(size_t lineIdx) const {
        const auto & arry = this->RfCache()[lineIdx];
        size_t maxPos;
        unsigned maxAge = 0;
        for(size_t i = 0; i < arry.size(); i++) {
            if(arry[i].age > maxAge && !arry[i].dirty) {
                maxAge = arry[i].age;
                maxPos = i; 
            }
        }
        // std::cout << "[OldestAgePos] " << maxPos << " " << this->OldestAge() << std::endl;
        return maxPos;
    }

    // RegOperand -> bool
    bool Hit(const regOps::RegOperand & reg, size_t lineIdx) const {
        for(auto entry : this->RfCache()[lineIdx]) {
            if(entry.index == reg.RegIndex())
                return true;
        }
        return false;
    }

    void Allocate(const regOps::RegOperand & reg) {
        if(this->EmptyEntryPos() == this->CfgPtr()->numEntry)
            throw std::runtime_error("[Rfc.Allocate] Runtime error: Allocating a full RFC");
       
        size_t pos = this->EmptyEntryPos();
        this->__cache[pos].index = reg.RegIndex();
        this->__cache[pos].age = 0;
        this->__cache[pos].dirty = false;
    }

    // Evict RFC
    // Basically, set an entry as dirty so we can allocate 
    void Evict() {
        if(this->CfgPtr()->repolicy == LRU) {
            auto pos = this->OldestAgePos();
            this->__cache[pos].dirty = true;
            this->__cache[pos].age = 0;
            // std::cout << "[Rfc.Evict] Evict pos = " << pos << std::endl;
        }
        else {
            this->__cache[0].dirty = true;
            // TODO: Implement other replacement policies
        }
    }

    // Serve a register operand
    void ServeReg(const regOps::RegOperand & reg) {
        // std::cout << "[Rfc.ServeReg] index = " << reg.RegIndex() << std::endl;

        // Hit
        if(Hit(reg)) return;

        // Miss
        if(reg.RegType() == regOps::SRC) {
            if(!this->Full()) { // If there is empty entry
                this->MrfPtr()->Access();
                this->Allocate(reg);
            }
            else { // If there is no empty entry
                this->Evict();
                this->MrfPtr()->Access(); // Access the MRF
                this->Allocate(reg);
            }
        }
        else if(reg.RegType() == regOps::DST) {
            if(!this->Full()) {
                this->MrfPtr()->Access();
                this->Allocate(reg);
            }
            else {
                // Write through
                this->MrfPtr()->Access();
            }
        }
        else 
            throw std::runtime_error("[Rfc.ServeReg] Runtime error");
    }    

    // Serve a trace instruction
    void ServeInst(const TraceInst & inst) {
        // Check warp id
        size_t subcore_id = inst.warp_id % 4;
        if(subcore_id != this->CfgPtr()->core_id) 
            throw std::runtime_error("[Rfc.ProcInst] Runtime Error: Warp ID - Core ID mismatch.");
        
        for(const auto & reg : inst.regs) {
            this->ServeReg(reg);
            this->Aging();
        }
    }

    void PrintRfc() {
        for(const auto & l : this->RfCache()) {
            std::cout << "[PrintRfc] Print RFC line..." << std::endl;
            for(const auto & e : this->RfCache()){
                // if(!e.dirty) std::cout << "R" << e.index << " ";
                std::cout << "R" << e.index << "[" << e.age << "]" << "[" << e.dirty << "]" << " ";
            }
            std::cout << std::endl;
        }
        std::cout << std::endl;
    }
};

#endif
