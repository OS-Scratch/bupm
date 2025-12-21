#include <iostream>
#include <about.h>
// Use for compability with C code
#include <stdio.h>

void About::show() {
    std::cout << "bupm version 1.1.5, released Dec 21 2025 \n";
    std::cout << "Binary-based package manager for Linux \n";
}
