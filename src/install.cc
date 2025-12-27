#include <iostream>
#include "install.h"
#include <filesystem>
#include <random>
#include <cstdlib>

namespace fs = std::filesystem;

std::string Installer::mktemp() {
    fs::path tmp = fs::temp_directory_path();

    std::string dirname = "install_";

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0,15);

    for (int i = 0; i < 8; ++i) {
	dirname += "0123456789abcdeffghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"[dis(gen)];
    }

    fs::path fp = tmp / dirname;

    if (fs::create_directory(fp)) {
	return fp.string();
    } else {
	std::cerr << "Failed to create temporary dir; are you root?"  << std::endl;
        return 0;
    }
}
void Installer::install(int argc, char* argv[]) {
    std::string installtmp = mktemp();
    if (argc < 2) {
	std::cout << "Package not provided, try hello_world?";
	exit(1);
    }
    std::string pkgname = argv[2];

    std::cout << "Using temporary installation directory " << installtmp << " for installation of package " << pkgname << " using bupm-install(1) \n";
    fs::remove_all(installtmp);
}
