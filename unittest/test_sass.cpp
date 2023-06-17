#include <iostream>
#include <stdexcept>

#include "reuse_info.h"



int main(int argc, char ** argv) {
	if(argc < 3 || std::string(argv[1]) != "-s")
		throw std::invalid_argument("[test_sass] Invalid argument.");
 	
	try {
		ReuseInfo_t reuse_info(argv[2]);
		reuse_info.ParseSASS();
		reuse_info.PPrint();	
	} catch (std::exception & e) {
		std::cerr << "[test_sass] " << e.what() << std::endl;
	}
	return 0;
}
