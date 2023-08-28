# RFC-sim
RFC-sim is an analytic model for evaluating the effect of Register File Cache (RFC) in NVIDIA GPUs. 

For more information, please check the original paper describing RFC: 
Energy-efficient Mechanisms for Managing Thread Context in Throughput Processors
<https://ieeexplore.ieee.org/document/6307762>

The model is utilized in my thesis project: 
Register File Cache Design for Energy Efficient GPU Tensor Cores
(Coming soon...)

## Usage
1. Run `make -j`;
2. `./rfc-sim.out -t <path_to_trace_dir> -c <path_to_config>`

The `<path_to_trace>` should be a directory contains `*kernelslist.g` generated by NVBit, a binary utility tool provided by NVIDIA. 
For more information about NVBit, please check <https://github.com/NVlabs/NVBit>

The `<path_to_config>` should be a text file describing the RFC configuration (Later I will migrate it to YAML format). 
An example of the confirguation file can be checked in `Configs/example.cfg`

## Output
(Coming soon...)
