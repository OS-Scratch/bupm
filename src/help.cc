#include <iostream>
#include <help.h>
// Use for compability with C code
#include <stdio.h>
void Help::show() {
    std::cout << "Usage: bupm [--help, --about] [fetch] \n";
    std::cout << "BUPM MANAGEMENT: \n --help: Display this page and general infos about each bupm command. \n";
    std::cout << "--about: Show version information. \n";
    std::cout << "PACKAGE MANAGEMENT: \n";
    std::cout << "fetch: Fetch package manifests and install them. \n";
    std::cout << "\n This Package Manager can really fetch packages. \n";
}
