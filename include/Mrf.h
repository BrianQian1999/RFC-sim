#pragma once

#include "TraceInst.h"
#include "Stat.h"

class Mrf {
private:
    std::shared_ptr<stat::RfcStat> stat;
    std::shared_ptr<stat::InstStat> iStat;
public:
    explicit Mrf(
        const std::shared_ptr<stat::RfcStat> & stat, 
        const std::shared_ptr<stat::InstStat> & iStat
    ) : stat(stat), iStat(iStat) {}

    void clear(); 
    void req(bool) noexcept;
    void exec(const TraceInst&);
}; /*class Mrf*/

