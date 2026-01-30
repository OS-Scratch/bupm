#include <curl/urlapi.h>
#include <cwchar>
#include <iostream>
#include "install.h"
#include <filesystem>
#include <random>
#include <cstdlib>
#include <cstdio>
#include <curl/curl.h>
#include <archive.h>
#include <string>
#include <archive_entry.h>
#include <fstream>
#include <cctype>
#include <sodium.h>
#include <toml++/toml.hpp>
#include <vector>
#include <algorithm>
#include <set>

namespace fs = std::filesystem;

namespace {

std::set<std::string> seen_packages;

bool is_installed(const std::string& pkgname) {
    std::ifstream file("/etc/bupm/installed");
    std::string line;
    while (std::getline(file, line)) {
        if (line == pkgname) return true;
    }
    return false;
}

void mark_as_installed(const std::string& pkgname) {
    std::ofstream file("/etc/bupm/installed", std::ios::app);
    file << pkgname << "\n";
}

bool verify_signature(const std::string& tarPath, const std::string& sigPath) {
    if (sodium_init() < 0) return false;
    unsigned char pk[crypto_sign_PUBLICKEYBYTES];
    std::ifstream pkFile("/etc/bupm/bupm.pub", std::ios::binary);
    if (!pkFile) return false;
    pkFile.read(reinterpret_cast<char*>(pk), sizeof pk);
    unsigned char sig[crypto_sign_BYTES];
    std::ifstream sFile(sigPath, std::ios::binary);
    if (!sFile) return false;
    sFile.read(reinterpret_cast<char*>(sig), sizeof sig);
    crypto_sign_state state;
    crypto_sign_init(&state);
    std::ifstream tFile(tarPath, std::ios::binary);
    char buffer[4096];
    while (tFile.read(buffer, sizeof buffer) || tFile.gcount() > 0) {
        crypto_sign_update(&state, reinterpret_cast<unsigned char*>(buffer), tFile.gcount());
    }
    if (crypto_sign_final_verify(&state, sig, pk) != 0) return false;
    return true;
}

int extract(const std::string& path, const std::string tgt) {
    struct archive* a = archive_read_new();
    struct archive* ext = archive_write_disk_new();
    struct archive_entry* entry;
    int r;
    archive_read_support_format_tar(a);
    archive_read_support_filter_all(a);
    archive_write_disk_set_options(ext, ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_PERM | ARCHIVE_EXTRACT_ACL | ARCHIVE_EXTRACT_FFLAGS | ARCHIVE_EXTRACT_SECURE_NODOTDOT | ARCHIVE_EXTRACT_UNLINK);
    archive_write_disk_set_standard_lookup(ext);
    if ((r = archive_read_open_filename(a, path.c_str(), 10240)) != ARCHIVE_OK) return 1;
    while ((r = archive_read_next_header(a, &entry)) == ARCHIVE_OK) {
        std::string currentFile = tgt + "/" + archive_entry_pathname(entry);
        archive_entry_set_pathname(entry, currentFile.c_str());
        r = archive_write_header(ext, entry);
        if (r == ARCHIVE_OK) {
            const void* buff;
            size_t size;
            la_int64_t offset;
            while (true) {
                r = archive_read_data_block(a, &buff, &size, &offset);
                if (r == ARCHIVE_EOF) break;
                if (r != ARCHIVE_OK) return 1;
                archive_write_data_block(ext, buff, size, offset);
            }
        }
        archive_write_finish_entry(ext);
    }
    archive_read_close(a);
    archive_read_free(a);
    archive_write_close(ext);
    archive_write_free(ext);
    return 0;
}

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

std::string getrepopath(const std::string& repoconf) {
    std::ifstream file(repoconf);
    std::string line;
    std::getline(file, line);
    return line;
}

std::string make_url(const std::string& base, const std::string& pkgname, const std::string& file) {
    char first = std::tolower(pkgname[0]);
    return base + "/" + first + "/" + pkgname + "/" + file;
}

void do_install(const std::string& pkgname, const std::string& baseurl) {
    if (is_installed(pkgname)) return;
    if (seen_packages.count(pkgname)) return;
    seen_packages.insert(pkgname);

    std::string manifest_url = make_url(baseurl, pkgname, "pkgdesc.toml");
    std::cout << "Fetching package description for " << pkgname << std::endl;
    std::string manifest_content = fetch_content(manifest_url);
    if (manifest_content.empty()) return;

    try {
        auto tbl = toml::parse(manifest_content);
        if (auto deps = tbl["dependencies"].as_table()) {
            if (auto direct = (*deps)["direct"].as_array()) {
                for (auto&& val : *direct) {
                    if (auto depname = val.as_string()) {
                        std::cout << pkgname << " depends on " << depname << ", installing" << std::endl;
                        do_install(**depname, baseurl);
                    }
                }
            }
        }
    } catch (...) {}

    fs::path tmp = fs::temp_directory_path();
    std::string dirname = "install_" + pkgname;
    fs::path fp = tmp / dirname;
    std::cout << "Creating installation directory";
    fs::create_directory(fp);

    std::string tar_url = make_url(baseurl, pkgname, "source.tar");
    std::string sig_url = tar_url + ".sig";
    fs::path tar_path = fp / "source.tar";
    fs::path sig_path = fp / "source.tar.sig";

    std::cout << "Starting download for package " << pkgname << std::endl;
    
    if (download_file(tar_url, tar_path.string()) && download_file(sig_url, sig_path.string())) {
      std::cout << "Verifying integrity for " << pkgname;
        if (verify_signature(tar_path.string(), sig_path.string())) {
          std::cout << "Extracting package archive" << std::endl;
            if (extract(tar_path.string(), "/") == 0) {
                mark_as_installed(pkgname);
                std::cout << pkgname << " installed successfully." << std::endl;
            }
        }
    }
    std::cout << "Cleaning up..." << std::endl;
    fs::remove_all(fp);
}

} // namespace

std::string Installer::mktemp() {
    return "";
}

void Installer::install(int argc, char* argv[]) {
    if (argc < 3) return;
    std::string pkgname = argv[2];
    std::string repoconf = "/etc/bupm/repositories";
    std::string baseurl = getrepopath(repoconf);
    if (baseurl.empty()) return;
    do_install(pkgname, baseurl);
}
