#include <iostream>
#include <about.h>
// Use for compability with C code
#include <stdio.h>

void About::show() {
    std::cout << "bupm version 1.2.1, released 22.01.2026 \n";
    std::cout << "Binary-based package manager for Linux \n";
    std::cout << "Currently, bupm is alpha software, so problems may occur.\n";
}
