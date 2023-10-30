#pragma once

#include <memory>

#include "Oprd.h"
#include "Rfc.h"
#include "CfgParser.h"

struct Rfc;
    
struct BaseAllocator {
    virtual void alloc(const reg::Oprd &, uint32_t) = 0;
};

// Write-allocate
struct WriteAllocator : BaseAllocator {
    Rfc * cc;
    WriteAllocator(Rfc * cc) : cc(cc) {} 
    void alloc(const reg::Oprd &, uint32_t) final;
};

// Compiler-aided allocator
struct CplAidedAllocator : BaseAllocator {
    Rfc * cc;
    CplAidedAllocator(Rfc * cc) : cc(cc) {} 
    void alloc(const reg::Oprd &, uint32_t) final;
};

// Compiler-aided allocation with looking ahead
struct LookAheadAllocator : BaseAllocator {
    Rfc * cc;
    LookAheadAllocator(Rfc * cc) : cc(cc) {} 
    void alloc(const reg::Oprd &, uint32_t) final;
};

struct AllocatorFactory {
    static BaseAllocator * getInstance(Rfc *, const cfg::GlobalCfg&);
};