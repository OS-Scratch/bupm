#include <iostream>
#include "repo.h"
#include <string>
#include <cstring>
#include <fstream>
#include <cstdio>
#include <curl/curl.h>
#include <cctype>

size_t write_data(void* ptr, size_t size, size_t nmemb, FILE* stream) {
    return fwrite(ptr, size, nmemb, stream);
}

bool dl(const std::string& url, const std::string& output) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "Failed to initialize curl." << std::endl;
        return false;
    }

    FILE* fp = fopen(output.c_str(), "wb");
    if (!fp) {
        std::cerr << "Failed to open output file: " << output << std::endl;
        curl_easy_cleanup(curl);
        return false;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L); 
    CURLcode res = curl_easy_perform(curl);
    fclose(fp);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        std::cerr << "curl failed: " << curl_easy_strerror(res) << std::endl;
        return false;
    }

    return true;
}

int Repository::handle(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "bupm: repo: invalid number of arguments" << std::endl;
        return 1;
    }

    if (strcmp(argv[2], "update") == 0) {
        Repository::update(argc, argv);
        return 0;
    }

    if (strcmp(argv[2], "show") == 0) {
        std::cout << Repository::show() << std::endl;
        return 0;
    }

    if (strcmp(argv[2], "set") == 0) {
        Repository::set(argc, argv);
        return 0;
    }
    
    if (strcmp(argv[2], "--help") == 0) {
	Repository::help;
	return 0;
    }

    std::cerr << "bupm: repo: invalid argument, provide --help for details." << std::endl;
    return 1;
}

void Repository::help() {
    std::cout << "Usage: bupm repo [set, update, show] \n";
    std::cout << "set: Sets the repository to the provided string. \n";
    std::cout << "update: Updates the local package database. \n";
    std::cout << "show: Shows the repository in use. \n";
}	
void Repository::update(int argc, char* argv[]) {
    std::cout << "Updating repositories..." << std::endl;
    const std::string urlfile = "/etc/bupm/repositories";
    const std::string pkgs_output = "/etc/bupm/pkgs";
    const std::string pkgtxt_output = "/etc/bupm/pkgtxt";

    std::ifstream file(urlfile);
    if (!file) {
        std::cerr << "Error opening file: " << urlfile << std::endl;
        return;
    }

    std::string base_url;
    if (!std::getline(file, base_url) || base_url.empty()) {
        std::cerr << "File is empty or invalid." << std::endl;
        return;
    }

    // Trim trailing whitespace/newline
    while (!base_url.empty() && std::isspace(base_url.back())) {
        base_url.pop_back();
    }

    std::string url_pkglist = base_url + "/pkglist";
    std::string url_pkgtxt = base_url + "/pkgtxt";

    std::cout << "Downloading pkglist from URL: " << url_pkglist << std::endl;
    if (dl(url_pkglist, pkgs_output)) {
        std::cout << "Download succeeded: " << pkgs_output << std::endl;
    } else {
        std::cout << "Download failed for pkglist." << std::endl;
    }

    std::cout << "Downloading pkgtxt from URL: " << url_pkgtxt << std::endl;
    if (dl(url_pkgtxt, pkgtxt_output)) {
        std::cout << "Download succeeded: " << pkgtxt_output << std::endl;
    } else {
        std::cout << "Download failed for pkgtxt." << std::endl;
    }
}

std::string Repository::show() {
    const std::string repofile = "/etc/bupm/repositories";
    std::ifstream file(repofile);

    if (!file) {
	std::cerr << "Error while opening file " << repofile << " . No repositories set up." << std::endl;
	return "";
    }

    std::string line;
    if (std::getline(file, line)) {
	    return line;
    } else {
	std::cerr << "Line could not be read. File is most probably empty.";
	return "";
    }
}

void Repository::set(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "bupm: repo: set: invalid number of arguments" << std::endl;
        return;
    }

    std::string repo_url = argv[3];
    std::cout << "Setting repository to " << repo_url << std::endl;

    const std::string repofile = "/etc/bupm/repositories";
    std::ofstream set(repofile, std::ios::trunc);

    if (!set) {
	std::cerr << "Error while opening repo file for writing." << std::endl;
	return;
    }

    set << repo_url;

    if (!set) {
	std::cerr << "Error while setting repository." << std::endl;
    }
}
