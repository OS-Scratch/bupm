#include <iostream>
#include <help.h>
// Use for compability with C code
#include <stdio.h>
void Help::show() {
    std::cout << "Usage: bupm [--help, --about] [fetch, repo, install] [repourl, pkgname] \n";
    std::cout << "HELPFUL FLAGS: \n --help: Display this page and general infos about each bupm command. \n";
    std::cout << "--about: Show version information. \n";
    std::cout << "BUPM MANAGEMENT: \n";
    std::cout << "repo: manage your repositories and update the local package list. \n";
    std::cout << "PACKAGE MANAGEMENT: \n";
    std::cout << "install: Install the provided package. \n";
    std::cout << "fetch: Fetch package manifests. \n";
    std::cout << "\n This Package Manager can really fetch packages. \n";
}
