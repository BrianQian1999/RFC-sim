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
#include <functional>

struct RfcEntry {
	RfcEntry();

    uint32_t tag; 
    uint32_t lruAge;
	uint32_t fifoAge;
    bool isDirty;

	void clear() noexcept;
	void aging() noexcept;
	void set(uint32_t, uint32_t, uint32_t, bool) noexcept;
};

inline std::ostream & operator<<(std::ostream &, const RfcEntry&);

struct Cam {
	explicit Cam(uint32_t assoc) : assoc(assoc) {}
	std::array<RfcEntry, 8> mem;
	uint32_t assoc; // associativity

	void flush() noexcept;
	void aging() noexcept;
	std::pair<bool, uint32_t> search(uint32_t addr) noexcept;
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
public:
	explicit Rfc(
		const std::shared_ptr<cfg::GlobalCfg>&,
		const std::shared_ptr<stat::RfcStat>&,
		const std::shared_ptr<Mrf>&
	);

	std::pair<bool, uint32_t> search(const reg::RegOprd&);
	void aging() noexcept;
	uint32_t cnt() noexcept;

	void exec(const TraceInst&);
	void evict() noexcept;

	std::pair<bool, uint32_t> replWrapper(uint32_t) noexcept;
	std::pair<bool, uint32_t> lruRepl(uint32_t) noexcept;
	std::pair<bool, uint32_t> fifoRepl(uint32_t) noexcept;

	void allocWrapper(const reg::RegOprd&);
	void fullCplAlloc(const reg::RegOprd&);
	void readAlloc(const reg::RegOprd&);
	void writeAlloc(const reg::RegOprd&);
	friend std::ostream & operator<<(std::ostream&, const Rfc&);
};
