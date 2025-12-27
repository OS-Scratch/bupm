#pragma once
#include <string>

class Repository {
public:
    static void handle(int argc, char* argv[]);
    static void update(int argc, char* argv[]);
    static std::string show();
    static void set(int argc, char* argv[]);
};
