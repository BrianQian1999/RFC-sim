#ifndef MRF_H
#define MRF_H

#include "trace_inst.h"
#include "reg_operand.h"

// Main register File (MRF)
class Mrf {
private:
    unsigned long __n_access; // number of accesses to the MRF
                            
public:
    Mrf() {} 
    
    void Reset() { this->__n_access = 0; }


    inline void Accumulate() {
        this->__n_access++;
    }

    void Access() {
        this->Accumulate();
    }

    void ProcInst(const TraceInst & inst) {
        for(int i = 0; i < inst.regs.size(); i++) {
            this->Access();
        }
    }

    void PrintStats() {
        std::cout << "[Mrf.PrintStats] Total number of accesses to MRF: " << this->__n_access << std::endl;
    }
};

#endif
