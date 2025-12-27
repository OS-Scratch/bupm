#pragma once
#include <string>

class Installer {
public:
    static void install(int argc, char* argv[]);
    static std::string mktemp();
};
