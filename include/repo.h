#pragma once
#include <string>

class Repository {
public:
    static void help();
    static int handle(int argc, char* argv[]);
    static void update(int argc, char* argv[]);
    static std::string show();
    static void set(int argc, char* argv[]);
};
