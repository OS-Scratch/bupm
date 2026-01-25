#include <iostream>
#include <fstream>
#include <fetch.h>
#include <string>
#include <cctype>
#include <curl/curl.h>
#include <toml++/toml.hpp>
#include <stdio.h>

size_t WriteCallback(void * contents, size_t size, size_t nmemb, void* userp) {
    size_t totalsize = size * nmemb;
    std::string* str = static_cast<std::string*>(userp);
    str->append(static_cast<char*>(contents), totalsize);
    return totalsize;
}

std::string getrepopath(const std::string& repoconf) {
    std::ifstream file(repoconf);
    if (!file) {
	std::cerr << "File " << repoconf << " doesn't exist or mismatch. " << std::endl << "Try creating one." << std::endl;
	return "";
    }
    std::string line;
    std::getline(file, line);
    return line;
}

std::string makeurl(const std::string& base, const std::string pkgname) {
    if (pkgname.empty()) return "";
    char first = std::tolower(pkgname[0]);
    return base + "/" + first + "/" + pkgname + "/pkgdesc.toml";
}

std::string fetch_url(const std::string& url) {
    CURL* curl = curl_easy_init();
    std::string readBuffer;

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
            readBuffer.clear();
        }
        curl_easy_cleanup(curl);
    }
    return readBuffer;
}

void tomlread(const std::string& tomlStr) {
    toml::table tbl;
    try {
        tbl = toml::parse(tomlStr);
    } catch (const toml::parse_error& e) {
        std::cerr << "TOML parsing error: " << e.description() << std::endl;
        return;
    }

    if (auto pkg = tbl["package"].as_table()) {
        if (auto name = (*pkg)["name"].as_string())
            std::cout << "Package Name: " << **name << std::endl;
        if (auto version = (*pkg)["version"].as_string())
            std::cout << "Version: " << **version << std::endl;
        if (auto desc = (*pkg)["desc"].as_string())
            std::cout << "Description: " << **desc << std::endl;
    } else {
        std::cout << "No package information found.\n";
    }

    if (auto files = tbl["files"].as_table()) {
        if (auto targets = (*files)["targets"].as_table()) {
            std::cout << "Files to copy:" << std::endl;
            for (auto&& [key, value] : *targets) {
                std::cout << "  " << key << ":" << std::endl;
                if (auto fileNode = value.as_table()) {
                    std::string name = (*fileNode)["name"].value_or("unknown");
                    std::string target = (*fileNode)["target"].value_or("unknown");
                    std::cout << "    name: " << name << std::endl;
                    std::cout << "    target: " << target << std::endl;
                }
            }
        }
    } else {
        std::cout << "No files to copy found.\n";
    }
}

void Delivery::show(int argc, char* argv[]) {
    if (argc <= 2 || std::string(argv[2]).empty()) {
	std::cout << "No package name provided, try hello_world? \n";
	return;
    }
    std::string pkgname = argv[2];
    if (std::string(argv[2]) == "packages") {
	std::cout << "Hey, I'm not a postman. Go and fetch your own packages.\n";
	return;
    }

    std::string repoconf = "/etc/bupm/repositories";
    std::string baseurl = getrepopath(repoconf);

    if (baseurl.empty()) {
	std::cerr << "Error while reading file, empty.";
	return;
    }
    
    std::string url = makeurl(baseurl, pkgname);
    
    std::cout << "Fetching package " << pkgname << " from " << url << std::endl;

    std::string content = fetch_url(url);
    
    if (!content.empty()) {
	tomlread(content);
    } else {
	std::cout << "Failed to fetch or empty content; try again later. \n If this issue persists, create a new issue on the project repo.";
	return;
    }
}