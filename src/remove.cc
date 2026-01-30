#include "remove.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <filesystem>
#include <curl/curl.h>
#include <archive.h>
#include <archive_entry.h>
#include <toml++/toml.hpp>
#include <set>
#include <unistd.h>

namespace fs = std::filesystem;

namespace {

size_t callback(char* ptr, size_t size, size_t nmemb, void* userdata) {
    std::string* s = static_cast<std::string*>(userdata);
    s->append(ptr, size * nmemb);
    return size * nmemb;
}

size_t file_callback(char* ptr, size_t size, size_t nmemb, void* userdata) {
    FILE* stream = static_cast<FILE*>(userdata);
    return fwrite(ptr, size, nmemb, stream);
}

std::string fetch_content(const std::string& url) {
    CURL* curl = curl_easy_init();
    std::string buffer;
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
        curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }
    return buffer;
}

bool download_file(const std::string& url, const std::string& path) {
    CURL* curl = curl_easy_init();
    if (!curl) return false;
    FILE* fp = fopen(path.c_str(), "wb");
    if (!fp) {
        curl_easy_cleanup(curl);
        return false;
    }
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, file_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
    CURLcode res = curl_easy_perform(curl);
    fclose(fp);
    curl_easy_cleanup(curl);
    return res == CURLE_OK;
}

std::string getrepopath() {
    std::ifstream file("/etc/bupm/repositories");
    std::string line;
    std::getline(file, line);
    return line;
}

std::string make_url(const std::string& base, const std::string& pkgname, const std::string& file) {
    char first = std::tolower(pkgname[0]);
    return base + "/" + first + "/" + pkgname + "/" + file;
}

std::vector<std::string> get_installed_packages() {
    std::vector<std::string> pkgs;
    std::ifstream file("/etc/bupm/installed");
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty()) pkgs.push_back(line);
    }
    return pkgs;
}

bool is_installed(const std::string& pkgname) {
    auto pkgs = get_installed_packages();
    return std::find(pkgs.begin(), pkgs.end(), pkgname) != pkgs.end();
}

void remove_from_db(const std::string& pkgname) {
    auto pkgs = get_installed_packages();
    std::ofstream file("/etc/bupm/installed", std::ios::trunc);
    for (const auto& pkg : pkgs) {
        if (pkg != pkgname) file << pkg << "\n";
    }
}

std::vector<std::string> get_dependencies(const std::string& pkgname, const std::string& baseurl) {
    std::vector<std::string> deps_list;
    std::string url = make_url(baseurl, pkgname, "pkgdesc.toml");
    std::string content = fetch_content(url);
    if (content.empty()) return deps_list;
    try {
        auto tbl = toml::parse(content);
        if (auto deps = tbl["dependencies"].as_table()) {
            if (auto direct = (*deps)["direct"].as_array()) {
                for (auto&& val : *direct) {
                    if (auto depName = val.as_string()) {
                        deps_list.push_back(**depName);
                    }
                }
            }
        }
    } catch (...) {}
    return deps_list;
}

bool is_needed(const std::string& pkgname, const std::string& baseurl) {
    auto installed = get_installed_packages();
    for (const auto& other : installed) {
        if (other == pkgname) continue;
        auto deps = get_dependencies(other, baseurl);
        for (const auto& d : deps) {
            if (d == pkgname) return true;
        }
    }
    return false;
}

} // namespace

int Remover::Remove(const std::string& pkgname) {
    if (!is_installed(pkgname)) {
        std::cerr << "Package " << pkgname << " is not installed." << std::endl;
        return 1;
    }

    std::string baseurl = getrepopath();
    if (baseurl.empty()) return 1;

    std::cout << "Analyzing dependencies";
    auto deps = get_dependencies(pkgname, baseurl);

    fs::path tmp = fs::temp_directory_path();
    std::string dirname = "remove_" + pkgname;
    fs::path fp = tmp / dirname;
    fs::create_directory(fp);
    std::string tar_url = make_url(baseurl, pkgname, "source.tar");
    fs::path tar_path = fp / "source.tar";

    std::cout << "Downloading package archive for " << pkgname << std::endl;
    
    if (download_file(tar_url, tar_path.string())) {
        struct archive* a = archive_read_new();
        archive_read_support_format_tar(a);
        archive_read_support_filter_all(a);
        std::cout << "Checking archive..." << std::endl;
        if (archive_read_open_filename(a, tar_path.c_str(), 10240) == ARCHIVE_OK) {
          std::cout << "Removing package " << pkgname;
            struct archive_entry* entry;
            while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
                std::string path = "/" + std::string(archive_entry_pathname(entry));
                std::error_code ec;
                if (!fs::is_directory(path, ec)) {
                    fs::remove(path, ec);
                }
            }
        }
        archive_read_free(a);
    }
    fs::remove_all(fp);

    remove_from_db(pkgname);
    std::cout << "Removed " << pkgname << std::endl;

    for (const auto& dep : deps) {
        if (is_installed(dep) && !is_needed(dep, baseurl)) {
            Remove(dep);
        }
    }

    return 0;
}
