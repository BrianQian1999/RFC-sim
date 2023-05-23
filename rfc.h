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

#define NDEBUG

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
	size_t __core_id;
    std::array<std::vector<RfcEntry>, 8> __cache;
    std::shared_ptr<GlobalCfg> __cfg_ptr;
    std::shared_ptr<Mrf> __mrf_ptr;


    // Accessors
    const std::shared_ptr<Mrf> & MrfPtr() const { return this->__mrf_ptr; }
    const std::shared_ptr<GlobalCfg> & CfgPtr() const { return this->__cfg_ptr; }

public:
    explicit Rfc(const std::shared_ptr<GlobalCfg> & cfg_ptr, const std::shared_ptr<Mrf> & mrf_ptr, size_t core_id) 
        : __cfg_ptr(cfg_ptr), __mrf_ptr(mrf_ptr), __core_id(core_id) { 
        for(auto & i : this->__cache) {
            i.resize(cfg_ptr->numEntryCfg);
			for(auto & e : i) {
				e.index = std::numeric_limits<unsigned int>::max();
				e.age = 0;
				e.dirty = true;
			}
        }
    }
    virtual ~Rfc() {}

    // Flush Rfc
    void Flush() {
        for(auto & line : this->__cache) {
            for(auto & e : line) {
                e.index = std::numeric_limits<unsigned int>::max();
                e.age = 0;
                e.dirty = true;
            }
        }
    }

    // Aging Rfc entries
    void Aging() {
        for(auto & line : this->__cache) {
            for(auto & e : line) {
                if(e.index != std::numeric_limits<unsigned int>::max() && !e.dirty) {
                    if(e.age < std::numeric_limits<unsigned long>::max()) {
                        e.age++;
                    }
                }
            }
        }
    }
    
    // If the RFC entries are full
    inline bool Full(size_t line_id) const {
        for(const auto & e : this->__cache[line_id]) {
            if(e.dirty) {
#ifndef NDEBUG
				std::cout << "[Rfc.Full] RFC line is NOT full." << std::endl; 
#endif
				return false;
			}
        }
#ifndef NDEBUG
		std::cout << "[Rfc.Full] RFC line is full." << std::endl; 
#endif
        return true;
    }

    // Return the index of first empty entry
    size_t EmptyEntryPos(size_t line_id) const { 
        if(this->Full(line_id)) throw std::runtime_error("[Rfc.EmptyEntryPos] Runtime error: RFC full.");

        for(size_t i = 0; i < this->__cache[line_id].size(); i++)  {
            if(this->__cache[line_id][i].index == std::numeric_limits<unsigned int>::max()
                    || this->__cache[line_id][i].dirty) { // dirty or simply empty
                return i;
            }
        }

        return this->CfgPtr()->numEntryCfg; // No empty entry
    }

    // Return the oldest age value
    unsigned OldestAge(size_t line_id) const {
        unsigned maxAge = 0;
        for(const auto & entry : this->__cache[line_id]) {
            if(entry.age > maxAge || !entry.dirty) maxAge = entry.age;
        }
        return maxAge;
    }

    size_t OldestAgePos(size_t line_id) const {
        size_t maxPos = 0;
        unsigned maxAge = 0;
        for(size_t i = 0; i < this->__cache[line_id].size(); i++) {
            if(this->__cache[line_id][i].age > maxAge && !this->__cache[line_id][i].dirty) {
                maxAge = this->__cache[line_id][i].age;
                maxPos = i; 
            }
        }
#ifndef NDEBUG
        std::cout << "[Rfc.OldestAgePos] " << maxPos << " " << this->OldestAge(line_id) << std::endl;
#endif
        return maxPos;
    }

    // RegOperand -> bool
    inline bool Hit(const regOps::RegOperand & reg, size_t line_id) const {
		for(const auto & e : this->__cache[line_id]) {
            if(e.index == reg.RegIndex()) {
#ifndef NDEBUG
                std::cout << "[Rfc.Hit] Register cache hit: " << reg << std::endl;
#endif
				return true;
			}
        }
#ifndef NDEBUG
		std::cout << "[Rfc.Hit] Register cache miss: " << reg << std::endl;
#endif
        return false;
    }

    void Allocate(const regOps::RegOperand & reg, size_t line_id) {
        if(this->EmptyEntryPos(line_id) == this->CfgPtr()->numEntryCfg)
            throw std::runtime_error("[Rfc.Allocate] Runtime error: allocating a full RFC");
#ifndef NDEBUG
		std::cout <<  "[Rfc.Allocate] Allocate cache entry for " << reg << std::endl;  
#endif
        size_t pos = this->EmptyEntryPos(line_id);
        this->__cache[line_id][pos].index = reg.RegIndex();
        this->__cache[line_id][pos].age = 0;
        this->__cache[line_id][pos].dirty = false;
    }

    // RFC Eviction
    void Evict(size_t line_id) {
        if(this->CfgPtr()->evictPlcyCfg == cfg::LRU) {
            auto pos = this->OldestAgePos(line_id);
#ifndef NDEBUG
			std::cout << "[Rfc.Evict] Evicted Pos = " << pos << std::endl;
#endif
            if(pos >= this->__cache[line_id].size()) throw std::runtime_error("[Rfc.Evict] Runtime error: index out of range.");
			this->__cache[line_id][pos].dirty = true;
            this->__cache[line_id][pos].age = 0;
        }
        else {
            this->__cache[line_id][0].dirty = true;
            // TODO: Implement other replacement policies
        }
    }

    // Process a register operand
    void ProcReg(const regOps::RegOperand & reg, size_t line_id) {
#ifndef NDEBUG
		std::cout << "[Rfc.ProcReg] Process register: " << reg << std::endl;
#endif
		// Hit
		if(Hit(reg, line_id)) return;

        // Miss
        if(reg.RegType() == regOps::SRC) {
            if(!this->Full(line_id)) { // If there are empty entries
                this->MrfPtr()->Access(true);
                this->Allocate(reg, line_id);
            }
            else { // If there is no empty entry
                this->Evict(line_id);
                this->MrfPtr()->Access(true); // Access the MRF
                this->Allocate(reg, line_id);
            }
        }
        else if(reg.RegType() == regOps::DST) {
            if(!this->Full(line_id)) {
                this->MrfPtr()->Access(false);
                this->Allocate(reg, line_id);
            }
            else {
                // Write through
                this->MrfPtr()->Access(false);
            }
        }
        else 
            throw std::runtime_error("[Rfc.ProcReg] Runtime error.");
    }    

    // Process a trace instruction
    void ProcInst(const TraceInst & inst) {
        // Check warp id
		size_t subcore_id;
		if(this->CfgPtr()->archCfg == cfg::TURING) 
        	subcore_id = inst.warp_id % 4;
		else // TODO: support other archs
        	subcore_id = inst.warp_id % 4;
			

        if(subcore_id != this->__core_id) 
            throw std::runtime_error("[Rfc.ProcInst] Runtime error: Warp - Core id mismatch.");
        
        for(const auto & reg : inst.regs) {
			// Deal with the source regs then dest reg
			if(reg.RegType() != regOps::DST) {
				this->ProcReg(reg, inst.warp_id % 8);
            	this->Aging();
			}
        }

		if(inst.regs.empty()) return;
		else {
			for(const auto & reg : inst.regs) {
				if(reg.RegType() == regOps::DST) {
					this->ProcReg(reg, inst.warp_id % 8);
					this->Aging();
				}
			}
		}
    }

    void PrintRfc() {
        for(const auto & l : this->__cache) {
            std::cout << "[PrintRfc] Print RFC line..." << std::endl;
            for(const auto & e : l){
                // if(!e.dirty) std::cout << "R" << e.index << " ";
                std::cout << "R" << e.index << "[" << e.age << "]" << "[" << e.dirty << "]" << " ";
            }
            std::cout << std::endl;
        }
        std::cout << std::endl;
    }
};

// Class RfcArry
class RfcArry {
private:
	std::array<std::unique_ptr<Rfc>, 4> __rfc_arry;
public:
	explicit RfcArry(const std::shared_ptr<GlobalCfg> & cfg_ptr, const std::shared_ptr<Mrf> & mrf_ptr) {
		for(size_t i = 0; i < 4; i++) {
			this->__rfc_arry[i] = std::make_unique<Rfc>(cfg_ptr, mrf_ptr, i);
		}
	}

	void ProcInst(const TraceInst & inst) {
		switch(inst.warp_id % 4) {
			case(0):
				this->__rfc_arry[0]->ProcInst(inst);
				break;
			case(1):
				this->__rfc_arry[1]->ProcInst(inst);
				break;
			case(2):
				this->__rfc_arry[2]->ProcInst(inst);
				break;
			case(3):
				this->__rfc_arry[3]->ProcInst(inst);
				break;
			default:
				throw std::runtime_error("[RfcArry.ProcInst] Runtime error");
		}	
	}		
};

#endif
