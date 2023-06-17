#ifndef CAM_H
#define CAM_H

#include <bitset>

#include "trace_inst.h"

// The Content-Addressable Memory
// Sub-core-wise structure
// Size = 8 (warps) * 32 (threads) * 4 (bits) = 128 Bytes
struct CAM_t {
	std::array<std::array<std::bitset<4>, 32>, 8> cam;  
	void SetCAM(const TraceInst * inst) {
		for(int i = 0; i < 32; i++) {
			if(inst.mask[i] == true)
				this->cam[inst.warp_id % 8][i] = inst.reuse_flags;
		}
	}
}
