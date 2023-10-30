#pragma once 

#include <iostream>
#include <vector>
#include <array>
#include <bitset>
#include <queue>
#include <memory>
#include <limits>
#include <stdexcept>
#include <algorithm>

#include "Alloc.h"
#include "Stat.h"
#include "Instr.h"

struct CacheEntry {
	CacheEntry();
    uint32_t tag; 
	uint32_t age;
    bool dt;

	void clear() noexcept;
	void step() noexcept;
	void set(uint32_t, uint32_t, bool) noexcept;
};

inline std::ostream & operator<<(std::ostream &, const CacheEntry&);

struct Cam {

	explicit Cam(uint32_t assoc, uint32_t nBlk, uint32_t nDW) : assoc(assoc), nDW(nDW) {
		for (auto & b : mem) b.resize(nBlk);
		for (auto & b : vMem) b.resize(nBlk);
	}

	std::array<std::vector<CacheEntry>, 32> mem;
	std::array<std::vector<CacheEntry>, 32> vMem;

	uint32_t assoc;
	uint32_t nDW; 
	
	void flush();
	void step();
	void sync();

	std::pair<bool, uint32_t> search(uint32_t, uint32_t, uint32_t);
};

inline std::ostream & operator<<(std::ostream &, const Cam&);

struct BaseAllocator;

struct Rfc{
	std::shared_ptr<cfg::GlobalCfg> cfg; // configuration
	std::shared_ptr<stat::Stat> scbBase; // scoreboard baseline
	std::shared_ptr<stat::Stat> scb; // sccoreboard
	std::unique_ptr<Cam> cam; // 
	BaseAllocator * allocator;
	
	std::bitset<32> mask;
	std::bitset<4> flags;
	std::queue<sass::Instr> iQueue; // Instruction Queue
	std::array<std::bitset<32>, 4> simdBuf;
	
	
	explicit Rfc(
		const std::shared_ptr<cfg::GlobalCfg>&,
		const std::shared_ptr<stat::Stat>&,
		const std::shared_ptr<stat::Stat>&
	);

	~Rfc();

	Rfc(const Rfc&); // copy constructor

	inline uint32_t bankTxCnt(const std::bitset<32>&);
	uint32_t getCacheSet(const reg::Oprd&) noexcept;

	std::pair<bool, uint32_t> search(const reg::Oprd&, uint32_t, uint32_t);
	
	void step() noexcept;
	void sync();
	bool exec(const sass::Instr&);
	void flushSimdBuf();
	
	inline void hitHandler(const reg::Oprd&, uint32_t, uint32_t);
	
	std::pair<bool, uint32_t> replWrapper(uint32_t, uint32_t);
	friend std::ostream & operator<<(std::ostream&, const Rfc&);
};
