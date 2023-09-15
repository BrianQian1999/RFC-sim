#include "AsmParser.h"

AsmParser::AsmParser() {}

AsmParser::AsmParser(
    	const std::string & asmFile,
    	const std::shared_ptr<std::vector<mapT>>& tab,
		const std::shared_ptr<std::unordered_map<std::string, size_t>>& map
    ) : tab(tab), map(map) {
    asmIfs = std::make_shared<std::ifstream>(asmFile);
}

void AsmParser::parse() {
	if (!asmIfs->is_open())
		return;

	size_t idx = 0;
	std::string kernel;
    
	while(!asmIfs->eof()) {
		std::string line_s;
		std::getline(*asmIfs, line_s);
		if(line_s.empty()) 
			continue;
		
		std::stringstream ss(line_s);
		std::vector<std::string> toks;
		std::string tok;

		while(std::getline(ss, tok, ' '))
			toks.push_back(tok);
		
		for (auto i = 0; i < toks.size(); i++) {
			if (toks.at(i) == "\t\tFunction") {
				kernel = toks.at(i + 2);
				map->insert(std::pair<std::string, size_t>(kernel, idx));
				tab->push_back(mapT());
				idx++;
				break;
			}
		}
		if (toks.size() < 10)
			continue;

		// Regex to detect input pattern
		std::regex inst1st_regex("/\\*([0-9A-Fa-f]{4})\\*/");
		std::regex inst2nd_regex("0x([0-9A-Fa-f]{16})");
		std::smatch matches;
         
		if(std::regex_search(toks[8], matches, inst1st_regex)) {
			std::string pc_s;
			std::string flag_s;
			uint32_t pc;

			try {
				pc_s = matches[0].str().substr(2, 4);
				std::stringstream pc_ss(pc_s);
				pc_ss >> std::hex >> pc; 
			} catch (std::exception & e) {
				std::cerr << e.what() << std::endl;
			}

			std::getline(*asmIfs, line_s);
			toks.clear();
			toks.resize(0);

			ss = std::stringstream(line_s);
			while(std::getline(ss, tok, ' '))
				toks.push_back(tok);

            if(std::regex_search(toks[toks.size() - 2], matches, inst2nd_regex)) {
				flag_s = matches[0].str().substr(2, 2); // e.g., for a 0x080fe2..., extract 08
				
				uint32_t flag_ui;
				std::stringstream flag_ss(flag_s);
				flag_ss >> std::hex >> flag_ui;
			
				std::bitset<8> ctrl_flags(flag_ui);
                std::bitset<4> flags(ctrl_flags.to_string().substr(2, 4));
	   			
				bool f0 = flags.test(0);
				bool f1 = flags.test(1);
				bool f2 = flags.test(2);
				bool f3 = flags.test(3);

				flags.set(0, f3);
				flags.set(1, f2);
				flags.set(2, f1);
				flags.set(3, f0);
                (tab->at(idx - 1)).insert(std::make_pair(std::move(pc), std::move(flags)));
			}
			else 
				throw std::runtime_error("Runtime error: failed to parse asm file.\n");
		}
		else 
			continue;
	}		
}
