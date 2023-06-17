#!/bin/bash
for i in $(seq 4 2 8); do
	cfg_name="configs/alloc_dst/alloc_dst_$i.yaml"
	./rfc-sim.out -t Apps/CUTLASS_TENCORE_GEMM_FP16/m512n512k512 -c $cfg_name
	./rfc-sim.out -t Apps/CUTLASS_TENCORE_GEMM_FP16/m768n768k768 -c $cfg_name
	./rfc-sim.out -t Apps/CUTLASS_TENCORE_GEMM_FP16/m1024n1024k1024 -c $cfg_name
	./rfc-sim.out -t Apps/CUTLASS_TENCORE_GEMM_FP16/m1280n1280k1280 -c $cfg_name
done
