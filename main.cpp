#include <iostream>
#include "utils.h"
#include "trace_parser.h"
int main(int argc, char ** argv) {
	utils::Dim3<int> tb_id(1, 2, 3);
	std::cout << tb_id << std::endl;

	utils::Dim3<int> gridDim(10, 1, 1);
	utils::Dim3<int> blockDim(128, 1, 1);
	unsigned id = 1;
	std::string name = "cudaTensorCoreGemm";

	KernelInfo info(name, id, gridDim, blockDim);
	std::cout << info << std::endl;
	return 0;
} 
