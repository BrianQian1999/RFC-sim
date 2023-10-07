#pragma once

#include "Mrf.h"
#include "Stat.h"

#include <iostream>
#include <vector>
#include <array>
#include <memory>
#include <limits>
#include <stdexcept>
#include <algorithm>

struct RfcBlock {
	RfcBlock();
    uint32_t tag; 
	uint32_t age;
    bool dt;

	void clear() noexcept;
	void step() noexcept;
	void set(uint32_t, uint32_t, bool) noexcept;
};

inline std::ostream & operator<<(std::ostream &, const RfcBlock&);

struct Cam {

	explicit Cam(uint32_t assoc, uint32_t nBlk, uint32_t nDW) : assoc(assoc), nDW(nDW) {
		for (auto & b : mem) b.resize(nBlk);
		for (auto & b : vMem) b.resize(nBlk);
	}

	std::array<std::vector<RfcBlock>, 32> mem;
	std::array<std::vector<RfcBlock>, 32> vMem;

	uint32_t assoc;
	uint32_t nDW; 
	
	void flush();
	void step();
	void sync();

	std::pair<bool, uint32_t> search(uint32_t, uint32_t, uint32_t);
};

inline std::ostream & operator<<(std::ostream &, const Cam&);

class Rfc{
private:
	std::shared_ptr<cfg::GlobalCfg> cfg;
	std::shared_ptr<stat::RfcStat> scb;
	std::shared_ptr<Mrf> mrf;
	std::unique_ptr<Cam> cam;

	std::bitset<32> mask;
	std::bitset<4> flags;

	std::array<std::bitset<32>, 4> simdBuf;
public:
	explicit Rfc(
		const std::shared_ptr<cfg::GlobalCfg>&,
		const std::shared_ptr<stat::RfcStat>&,
		const std::shared_ptr<Mrf>&
	);

	inline uint32_t bankTxCnt(const std::bitset<32>&);
	uint32_t sMap(const reg::Oprd&) noexcept;

	std::pair<bool, uint32_t> search(const reg::Oprd&, uint32_t, uint32_t);
	
	void step() noexcept;
	void sync();
	void exec(const TraceInst&);
	void flushSimdBuf();
	
	inline void hitHandler(const reg::Oprd&, uint32_t, uint32_t);
	
	std::pair<bool, uint32_t> replWrapper(uint32_t, uint32_t);
	void allocWrapper(const reg::Oprd&, uint32_t);
	
	void readAlloc(const reg::Oprd&, uint32_t);
	void writeAlloc(const reg::Oprd&, uint32_t);
	void cplAidedAlloc(const reg::Oprd&, uint32_t);

	friend std::ostream & operator<<(std::ostream&, const Rfc&);
};
