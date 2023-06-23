#pragma once

#include "trace_inst.h"
#include "reg_operand.h"

// Main register File (MRF)
class Mrf_t {
private:
    unsigned long _n_access_rd; // number of read accesses to the MRF
	unsigned long _n_access_wr; // number of write accesses to the MRF
                            
public:
    Mrf_t() {
		this->_n_access_rd = 0;
		this->_n_access_wr = 0;
	} 
    
    void Reset() { 
		this->_n_access_rd = 0;
		this->_n_access_wr = 0;
	}


    inline void Accumulate(bool is_rd) {
        if(is_rd)
			this->_n_access_rd++;
		else
			this->_n_access_wr++;
    }

    void Access(bool is_rd) {
        this->Accumulate(is_rd);
    }

    void ProcInst(const TraceInst_t & inst) {
        for(auto reg : inst.regPool) {
            if(reg.RegType() == regOps::SRC) 
				this->Access(true);
			else 
				this->Access(false);
        }
    }

    void PrintStats() {
        std::cout << "[Mrf.PrintStats] Total number of READ accesses to MRF: " << this->_n_access_rd << std::endl;
		std::cout << "[Mrf.PrintStats] Total number of WRITE accesses to MRF: " << this->_n_access_wr << std::endl;
    }
};

