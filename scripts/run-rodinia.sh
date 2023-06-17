#!/bin/bash
trace_dir="./Apps/rodinia_3.1"
dirs=$(find "$trace_dir" -maxdepth 1 -type d)
for i in $(seq 4 2 8); do 
	config_file="configs/alloc_dst/alloc_dst_$i.yaml"
	echo "Using config file: $config_file"
	for dir in $dirs; do
		echo "Run benchmark: $dir"
		./rfc-sim.out -t "$dir" -c "$config_file"
	done
done
