#include <iostream>
#include "repo.h"
#include <string>

static void handle(int argc, char* argv[]) {
    if (argc < 2) {
	std::cout << "What do you want me to do? \n";
	return 1;
    }
    
    if (std::string(argv[2]) == "show") {
	std::cout << "I'm going to show you the enabled repository. \n";
	return 0;
    }
    else {
	std::cout << "Hmm, I don't know how to do that. \n";
	return 1;
    }
}
