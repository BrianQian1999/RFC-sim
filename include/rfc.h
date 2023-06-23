// Register File Cache (RFC) for NVIDIA GPGPU
// Qiran Qian, <qiranq@kth.se>

#pragma once

#include "cfg_parser.h"
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

// #define NDEBUG

struct RfcEntry_t {
    RfcEntry_t() { 
        this->index = std::numeric_limits<unsigned int>::min(); 
        this->age = 0; 
        this->dirty = true;
    } 
    unsigned index;
    unsigned long age;
    bool dirty;
};

struct RfcStats_t {
	RfcStats_t() {
		this->rfcNumRd = 0;
		this->rfcNumWr = 0;
		this->rfcNumRdMiss = 0;
		this->rfcNumRdHit = 0;
		this->rfcNumWrMiss = 0;
		this->rfcNumWrHit = 0;
	}

	unsigned long rfcNumRd;
	unsigned long rfcNumWr;

	unsigned long rfcNumRdMiss;
	unsigned long rfcNumWrMiss;
	unsigned long rfcNumRdHit;
	unsigned long rfcNumWrHit;

	void AccRd() { this->rfcNumRd++; }
	void AccWr() { this->rfcNumWr++; }

	void AccRdMiss() { this->rfcNumRdMiss++; }
	void AccRdHit() { this->rfcNumRdHit++; }
	void AccWrMiss() { this->rfcNumWrMiss++; }
	void AccWrHit() { this->rfcNumWrHit++; }

	float GetRdHitRate() {
		float numHit = static_cast<float>(this->rfcNumRdHit);
		float numTot = static_cast<float>(this->rfcNumRdHit + this->rfcNumRdMiss); 
	}

	float GetWrHitRate() {
		float numHit = static_cast<float>(this->rfcNumWrHit);
		float numTot = static_cast<float>(this->rfcNumWrHit + this->rfcNumWrMiss); 
	}

	void PPrint() {
		std::cout << "[RfcStats_t.PPrint] <Read: " << this->rfcNumRd << "> <Write: " << this->rfcNumWr << ">" << std::endl;
		std::cout << "[RfcStats_t.PPrint] <Read hit rate: " << this->GetRdHitRate() << "> <Write hit rate: " << this->GetWrHitRate() << ">" << std::endl;
	}
};



// A Rfc object corresponds to a single warp scheduler 
class Rfc_t {
private:
	size_t _core_id;
    std::array<std::vector<RfcEntry_t>, 8> _cache;
    std::shared_ptr<GlobalCfg_t> _cfg_ptr;
    std::shared_ptr<Mrf_t> _mrf_ptr;

	// Statistics
	std::shared_ptr<RfcStats_t> _stat_ptr;

    // Accessors
    const std::shared_ptr<Mrf_t> & MrfPtr() const { return this->_mrf_ptr; }
    const std::shared_ptr<GlobalCfg_t> & CfgPtr() const { return this->_cfg_ptr; }

public:
    explicit Rfc_t(const std::shared_ptr<GlobalCfg_t> & cfg_ptr, const std::shared_ptr<Mrf_t> & mrf_ptr, size_t core_id) 
        : _cfg_ptr(cfg_ptr), _mrf_ptr(mrf_ptr), _core_id(core_id) { 
        for(auto & i : this->_cache) {
            i.resize(cfg_ptr->numEntryCfg);
			for(auto & e : i) {
				e.index = std::numeric_limits<unsigned int>::min();
				e.age = 0;
				e.dirty = false;
			}
        }
		this->_stat_ptr = std::make_shared<RfcStats_t>();
    }
    virtual ~Rfc_t() {}

	// Get Sub-Core ID
	size_t GetCoreId() {
		return this->_core_id;
	}

	// Get Statistics
	inline unsigned long GetAcc(bool is_rd) const { return is_rd ? this->_stat_ptr->rfcNumRd : this->_stat_ptr->rfcNumWr; }
	inline unsigned long GetAccHit(bool is_rd) const { return is_rd ? this->_stat_ptr->rfcNumRdHit : this->_stat_ptr->rfcNumWrHit; }
	inline unsigned long GetAccMiss(bool is_rd) const { return is_rd ? this->_stat_ptr->rfcNumRdMiss : this->_stat_ptr->rfcNumWrMiss; }

    // Flush Rfc
    void Flush() {
        for(auto & line : this->_cache) {
            for(auto & e : line) {
                e.index = std::numeric_limits<unsigned int>::min();
                e.age = 0;
                e.dirty = false;
            }
        }
    }

    // Aging Rfc entries
    void Aging() {
        for(auto & line : this->_cache) {
            for(auto & e : line) {
                if(e.index != std::numeric_limits<unsigned int>::min() && e.age < std::numeric_limits<unsigned long>::max()) {
                        e.age++;
                }
            }
        }
    }
    
    // If the RFC entries are full
    inline bool Full(size_t line_id) const {
        for(const auto & e : this->_cache[line_id]) {
			// Basically, R0 is an empty entry
            if(e.index == std::numeric_limits<unsigned int>::min()) {
				return false;
			}
        }
        return true;
    }

    // Return the index of first empty entry
    size_t EmptyEntryPos(size_t line_id) const { 
        if(this->Full(line_id)) throw std::runtime_error("[Rfc.EmptyEntryPos] Runtime error: RFC full.");

        for(size_t i = 0; i < this->_cache[line_id].size(); i++)  {
            if(this->_cache[line_id][i].index == std::numeric_limits<unsigned int>::min()) { 
                return i;
            }
        }

        return this->CfgPtr()->numEntryCfg; // No empty entry
    }

    // Return the oldest age value
    unsigned OldestAge(size_t line_id) const {
        unsigned maxAge = 0;
        for(const auto & entry : this->_cache[line_id]) {
            if(entry.age > maxAge || !entry.dirty) maxAge = entry.age;
        }
        return maxAge;
    }

    size_t OldestAgePos(size_t line_id) const {
        size_t maxPos = 0;
        unsigned maxAge = 0;
        for(size_t i = 0; i < this->_cache[line_id].size(); i++) {
            if(this->_cache[line_id][i].age > maxAge && !this->_cache[line_id][i].dirty) {
                maxAge = this->_cache[line_id][i].age;
                maxPos = i; 
            }
        }

#ifndef NDEBUG
std::cout << "[Rfc.OldestAgePos] " << maxPos << " " << this->OldestAge(line_id) << std::endl;
#endif
        return maxPos;
    }

    // RegOperand_t -> bool
    inline bool Hit(const regOps::RegOperand_t & reg, size_t line_id) {
		for(auto & e : this->_cache[line_id]) {
            if(e.index == reg.RegIndex()) {
#ifndef NDEBUG
                std::cout << "[Rfc.Hit] Register cache hit: " << reg << std::endl;
#endif
				e.age = 0; // Reset age if hit
				return true;
			}
        }
#ifndef NDEBUG
		std::cout << "[Rfc.Hit] Register cache miss: " << reg << std::endl;
#endif
        return false;
    }

    // RFC Entry Allocator
    void Allocate(const regOps::RegOperand_t & reg, size_t line_id, bool dFlag) {
        if(this->EmptyEntryPos(line_id) == this->CfgPtr()->numEntryCfg)
            throw std::runtime_error("[Rfc.Allocate] Runtime error: allocating a full RFC");

#ifndef NDEBUG
		std::cout <<  "[Rfc.Allocate] Allocate cache entry for " << reg << std::endl;  
#endif

		// Accumulate stats
		this->_stat_ptr->AccWr();

        size_t pos = this->EmptyEntryPos(line_id);
        this->_cache[line_id][pos].index = reg.RegIndex();
        this->_cache[line_id][pos].age = 0;
        this->_cache[line_id][pos].dirty = dFlag;
    }

    // RFC Replacement
	// Return if the replaced entry is dirty
    bool Replace(size_t line_id) {
        if(this->CfgPtr()->rePlcyCfg == cfg::LRU) {
            auto pos = this->OldestAgePos(line_id);

#ifndef NDEBUG
std::cout << "[Rfc.Replace] Replace Pos = " << pos << std::endl;
#endif

            if(pos >= this->_cache[line_id].size()) 
				throw std::runtime_error("[Rfc.Replace] Runtime error: index out of range.");
			bool isDirty = this->_cache[line_id][pos].dirty;

			this->_cache[line_id][pos].index = std::numeric_limits<unsigned int>::min();
			this->_cache[line_id][pos].dirty = false;
            this->_cache[line_id][pos].age = 0;
			return isDirty;
        }
        else {
            // TODO: Implement other replacement policies
            this->_cache[line_id][0].dirty = false;
			return false;
        }
    }

    // Process a register operand
    void ProcReg(const regOps::RegOperand_t & reg, size_t line_id, bool is_mma_acc_src) {

#ifndef NDEBUG
		std::cout << "[Rfc.ProcReg] Process register: " << reg << std::endl;
#endif

		// Hit
		if(Hit(reg, line_id)) {
			if(reg.RegType() == regOps::DST) {
				// If hit, we don't need to write to RFC
				this->_stat_ptr->AccWrHit();
			}
			else if(reg.RegType() == regOps::SRC) {
				this->_stat_ptr->AccRd(); // Read from RFC
				this->_stat_ptr->AccRdHit();
			}
			return;
		}

        // Miss
        if(reg.RegType() == regOps::SRC) {
			this->_stat_ptr->AccRdMiss();
            if(this->CfgPtr()->allocPlcyCfg == cfg::ALLOC_BASE) {
                if(!this->Full(line_id)) { // If there are empty entries
                    this->MrfPtr()->Access(true);
                    this->Allocate(reg, line_id, false);
                }
                else { // If there is no empty entry
                    bool dFlag = this->Replace(line_id);
					if(dFlag) this->MrfPtr()->Access(false); // If a dirty Reg is replaced, write back to MRF
                    this->MrfPtr()->Access(true); // Read from MRF
                    this->Allocate(reg, line_id, false); // Allocate a RFC entry
                }
            }
            else if(this->CfgPtr()->allocPlcyCfg == cfg::ALLOC_DST) {
                this->MrfPtr()->Access(true); // simply Read from MRF
            }
			// Implement the custom allocation policy
			else if(this->CfgPtr()->allocPlcyCfg == cfg::ALLOC_CUSTOM) {
				// If alloc_custom is used, then allocate RFC entry of accumulation matrix (ACC SRC)
				if(is_mma_acc_src) {
					if(!this->Full(line_id)) {
						this->MrfPtr()->Access(true); // Read from MRF
						this->Allocate(reg, line_id, false); // Write to RFC
					}
					else {
						bool dFlag = this->Replace(line_id);
						if(dFlag) this->MrfPtr()->Access(false); // Write back to MRF
						this->MrfPtr()->Access(true); // read from MRF
						this->Allocate(reg, line_id, false); // Write to RFC
					}
				}	
				else {
					this->MrfPtr()->Access(true); // Read from MRF
				}
			}
            else 
                throw std::runtime_error("[Rfc.ProcReg] Runtime error.");
        }
        else if(reg.RegType() == regOps::DST) {
			this->_stat_ptr->AccWrMiss();
            if(!this->Full(line_id)) {
				// If the corresponding RFC line is not full, simply allocate a new entry, and do not access MRF
                this->Allocate(reg, line_id, true); // write to RFC
            }
            else {
                // Write back
				if(this->CfgPtr()->evPlcyCfg == cfg::WR_BACK) {
					bool dFlag = this->Replace(line_id); // If a dirty reg is to be replaced, it must be written back to MRF to maintain consistency
					this->Allocate(reg, line_id, true); // write to a new RFC entry
					if(dFlag) this->MrfPtr()->Access(false); // If dirty, write back to MRF
				}
				// Write through
				else {
					this->Replace(line_id); 
					this->Allocate(reg, line_id, false); // Write to RFC
                	this->MrfPtr()->Access(false); // write to MRF immediately so consistency is guranteed
				}
            }
        }
        else 
            throw std::runtime_error("[Rfc.ProcReg] Runtime error.");
    }    

    // Process a trace instruction
    void ProcInst(const TraceInst_t & inst) {
        // Check warp id
		size_t subcore_id;
		if(this->CfgPtr()->archCfg == cfg::TURING) 
        	subcore_id = inst.warp_id % 4;
		else // TODO: support other archs
        	subcore_id = inst.warp_id % 4;
			

        if(subcore_id != this->_core_id) 
            throw std::runtime_error("[Rfc.ProcInst] Runtime error: Warp - Core id mismatch.");
       
		// Process source registers 
		size_t reg_idx = 0;
        for(const auto & reg : inst.regPool) {
			if(reg.RegType() != regOps::DST) {
				if((inst.opcode == OP_HMMA || inst.opcode == OP_IMMA || inst.opcode == OP_BMMA) && 
					reg_idx <= 5 && reg_idx >= 4)
					this->ProcReg(reg, inst.warp_id % 8, true);
				else  
					this->ProcReg(reg, inst.warp_id % 8, false);
            	this->Aging();
			}
			reg_idx++;
        }

		// Process destination registers
		if(inst.regPool.empty()) return;
		else {
			for(const auto & reg : inst.regPool) {
				if(reg.RegType() == regOps::DST) {
					this->ProcReg(reg, inst.warp_id % 8, false);
					this->Aging();
				}
			}
		}
    }

	// RFC status Pretty-printer
    void PPrintRfc() {
        for(const auto & l : this->_cache) {
            for(const auto & e : l){
                // if(!e.dirty) std::cout << "R" << e.index << " ";
                std::cout << "R" << e.index << "<age:" << e.age << ", dirty?" << e.dirty << ">" << " ";
            }
            std::cout << std::endl;
        }
        std::cout << std::endl;
    }
};

// Class RfcArry_t
class RfcArry_t {
private:
	std::array<std::unique_ptr<Rfc_t>, 4> _rfc_arry;
public:
	explicit RfcArry_t(const std::shared_ptr<GlobalCfg_t> & cfg_ptr, const std::shared_ptr<Mrf_t> & mrf_ptr) {
		for(size_t i = 0; i < 4; i++) {
			this->_rfc_arry[i] = std::make_unique<Rfc_t>(cfg_ptr, mrf_ptr, i);
		}
	}

	void ProcInst(const TraceInst_t & inst) {
		switch(inst.warp_id % 4) {
			case(0):
				this->_rfc_arry[0]->ProcInst(inst);
				break;
			case(1):
				this->_rfc_arry[1]->ProcInst(inst);
				break;
			case(2):
				this->_rfc_arry[2]->ProcInst(inst);
				break;
			case(3):
				this->_rfc_arry[3]->ProcInst(inst);
				break;
			default:
				throw std::runtime_error("[RfcArry_t.ProcInst] Runtime error");
		}	
	}		
	
	void PPrint() {
		std::cout << "[RfcArry_t.PPrint]: " << std::endl;
		for(auto & block : this->_rfc_arry) {
			std::cout << "<Sub-Core " << block->GetCoreId() << "> " << std::endl;
			block->PPrintRfc();
		}
	}

	void PrintStats() {
		unsigned long n_rd = 0;
		unsigned long n_wr = 0;

		unsigned long n_rd_hit = 0;
		unsigned long n_rd_miss = 0;
		unsigned long n_wr_hit = 0;
		unsigned long n_wr_miss = 0;

		for(const auto & rfcPtr : this->_rfc_arry) {
			n_rd += rfcPtr->GetAcc(true);
			n_wr += rfcPtr->GetAcc(false);

			n_rd_hit += rfcPtr->GetAccHit(true);
			n_wr_hit += rfcPtr->GetAccHit(false);
			n_rd_miss += rfcPtr->GetAccMiss(true);
			n_wr_miss += rfcPtr->GetAccMiss(false);
		}

		float n_rd_hit_flt = static_cast<float>(n_rd_hit);
		float n_wr_hit_flt = static_cast<float>(n_wr_hit);
		float n_rd_miss_flt = static_cast<float>(n_rd_miss);
		float n_wr_miss_flt = static_cast<float>(n_wr_miss);

		std::cout << "[RfcArry_t.PrintStats] Total # of READ from RFC: " << n_rd << std::endl;
		std::cout << "[RfcArry_t.PrintStats] Total # of WRITE to RFC: " << n_wr << std::endl;
		std::cout << "[RfcArry_t.PrintStats] RFC READ hit rate: " << n_rd_hit_flt / (n_rd_hit_flt + n_rd_miss_flt) * 100 << "%" << std::endl;
		std::cout << "[RfcArry_t.PrintStats] RFC WRITE hit rate: " << n_wr_hit_flt / (n_wr_hit_flt + n_wr_miss_flt) * 100 << "%" <<  std::endl;
	}
};

