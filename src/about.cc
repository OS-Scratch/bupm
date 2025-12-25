#include <iostream>
#include <about.h>
// Use for compability with C code
#include <stdio.h>

void About::show() {
    std::cout << "bupm version 1.1.6 build 25122025, development release\n";
    std::cout << "Binary-based package manager for Linux \n";
}
