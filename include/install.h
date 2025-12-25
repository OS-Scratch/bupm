#pragma once

class Installer {
public:
    static void install(int argc, char* argv[]);
    std::string mktemp();
};
