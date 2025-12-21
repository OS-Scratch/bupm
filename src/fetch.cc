#include <iostream>
#include <fstream>
#include <fetch.h>
#include <string>
#include <cctype>
#include <curl/curl.h>
#include <yaml-cpp/yaml.h>
// Use for compability with C code
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
	std::cerr << "File " << repoconf << "doesn't exist or mismatch. " << std::endl << "Try creating one or reinstalling bupm.";
	return "";
    }
    std::string line;
    std::getline(file, line);
    return line;
}

std::string makeurl(const std::string& base, const std::string pkgname) {
    if (pkgname.empty()) return "";
    char first = std::tolower(pkgname[0]);
    return base + "/" + first + "/" + pkgname + "/pkgdesc.yaml";
}

std::string fetch_url(const std::string& url) {
    CURL* curl = curl_easy_init();
    std::string readBuffer;

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        // Save files in buffer
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        // Check SSL
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

void yamlread(const std::string& yamlStr) {
    YAML::Node root;
    try {
        root = YAML::Load(yamlStr);
    } catch (const YAML::ParserException& e) {
        std::cerr << "YAML parsing error: " << e.what() << std::endl;
        return;
    }

    if (root["package"]) {
        const YAML::Node& pkg = root["package"];
        if (pkg["name"])
            std::cout << "Package Name: " << pkg["name"].as<std::string>() << std::endl;
        if (pkg["version"])
            std::cout << "Version: " << pkg["version"].as<std::string>() << std::endl;
        if (pkg["desc"])
            std::cout << "Description: " << pkg["desc"].as<std::string>() << std::endl;
    } else {
        std::cout << "No package information found.\n";
    }

    if (root["files"] && root["files"]["targets"]) {
        YAML::Node targets = root["files"]["targets"];
        std::cout << "Files to copy:" << std::endl;

        for (YAML::const_iterator it = targets.begin(); it != targets.end(); ++it) {
            std::string fileKey = it->first.as<std::string>();
            YAML::Node fileNode = it->second;

            std::string name = fileNode["name"] ? fileNode["name"].as<std::string>() : "unknown";
            std::string target = fileNode["target"] ? fileNode["target"].as<std::string>() : "unknown";

            std::cout << "  " << fileKey << ":" << std::endl;
            std::cout << "    name: " << name << std::endl;
            std::cout << "    target: " << target << std::endl;
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
	std::cout << "You found me, even though I'm still in development! \n";
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
	yamlread(content);
    } else {
	std::cout << "Failed to fetch or empty content; try again later. \n If this issue persists, create a new issue.";
	return;
    }
}
