#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <stdexcept>
#include <array>
#include <unordered_map>
#include <vector>
#include <bitset>
#include <regex>
#include <iomanip>

#include "trace_inst.h"

struct ReuseInfo_t {
	std::unique_ptr<std::ifstream> dump_ifs_ptr; // SASS input file stream
	std::unordered_map<unsigned, std::bitset<4>> reuse_info_tab;

	// Constructor	
	explicit ReuseInfo_t(const std::string & dump_file) {
		this->dump_ifs_ptr = std::make_unique<std::ifstream>(dump_file);
		if(!this->dump_ifs_ptr->is_open())
			throw std::runtime_error("[ReuseInfo_t.constructor] Runtime error.");
	}
	
	// Deconstructor
	~ReuseInfo_t() noexcept {
		try { 
			this->dump_ifs_ptr->close();
		} catch (const std::exception & e) {
			std::cerr << "[~ReuseInfo_t] " << e.what() << std::endl;
		}	
	}
	
	void ParseSASS() {
		while(!dump_ifs_ptr->eof()) {
			std::string line_s;
			std::getline(*dump_ifs_ptr, line_s);  // Get a new line
			if(line_s.empty()) 
				continue;
			
			std::stringstream ss(line_s);
			std::vector<std::string> toks;
			std::string tok;
			while(std::getline(ss, tok, ' '))
				toks.push_back(tok);
			if(toks.size() < 10)
				continue; 			

			// Regex to detect input pattern
			std::regex inst1st_regex("/\\*([0-9A-Fa-f]{4})\\*/");
			std::regex inst2nd_regex("0x([0-9A-Fa-f]{16})");
			std::smatch matches;
         
            // Search for the pattern
			if(std::regex_search(toks[8], matches, inst1st_regex)) {
				std::string pc_s;
				std::string flag_s;
				unsigned pc;

				// Convert PC to unsigned
				try {
					pc_s = matches[0].str().substr(2, 4);
					std::stringstream pc_ss(pc_s);
					pc_ss >> std::hex >> pc; // Convert from hex to dec
				} catch (std::exception & e) {
					std::cerr << "[ReuseInfo_t.ParseSASS] " << e.what() << std::endl;
				}

				std::getline(*dump_ifs_ptr, line_s); // Get the 2nd part of the instruction
				toks.clear();
				ss = std::stringstream(line_s);
				while(std::getline(ss, tok, ' '))
					toks.push_back(tok);

                if(std::regex_search(toks[toks.size() - 2], matches, inst2nd_regex)) {
					flag_s = matches[0].str().substr(2, 2); // e.g., for a 0x080fe2..., extract 08
                    
					unsigned long flag_ui;
					std::stringstream flag_ss(flag_s);
					flag_ss >> std::hex >> flag_ui;
					std::bitset<8> ctrl_flags(flag_ui);
                    std::bitset<4> flags(ctrl_flags.to_string().substr(2, 4));
		        	this->reuse_info_tab.insert(std::make_pair(pc, flags)); // insert a new pair
				}
				else 
					throw std::runtime_error("[ReuseInfo_t.ParseSASS] Runtime error.");
			}
			else 
				continue;
		}		
	}

	void PPrint() {
		std::cout << "[ReuseInfo_t.PPrint]: \n";
	    for(auto e : this->reuse_info_tab) {
            std::cout << "<PC: " << std::hex << e.first << "> <Flags: " << e.second << ">" << std::endl;
		}		
	}
};

