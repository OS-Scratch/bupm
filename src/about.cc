#include <iostream>
#include <about.h>
// Use for compability with C code
#include <stdio.h>

void About::show() {
    std::cout << "bupm version 1.1.4, released Dec 19 2025 \n";
    std::cout << "Source-based package / build package tree manager for Linux \n";
}
