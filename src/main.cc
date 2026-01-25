#include "help.h"
#include "fetch.h"
#include "repo.h"
#include "about.h"
#include "install.h"
#include <string>
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "No command specified, try --help \n";
        return 1;
    }
    else if (std::string(argv[1]) == "--about") {
        About::show();
        return 0;
    }
    else if (std::string(argv[1]) == "--help") {
        Help::show();
        return 0;
    }
    else if (std::string(argv[1]) == "fetch") {
        Delivery::show(argc, argv);
        return 0;
    }
    else if (std::string(argv[1]) == "install") {
	      Installer::install(argc, argv);
	      return 0;
    }
    else if (std::string(argv[1]) == "repo") {
	Repository::handle(argc, argv);
    }
    else {
        std::cerr << "Unknown option: " << argv[1] << " \n Try using --help instead. \n";
        return 1;
    }
}

