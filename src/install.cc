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

namespace {

bool verify_signature(const std::string& tarPath, const std::string& sigPath) {
    if (sodium_init() < 0) {
        std::cerr << "Sodium initialization failed" << std::endl;
        return false;
    }

    unsigned char pk[crypto_sign_PUBLICKEYBYTES];
    std::ifstream pkFile("/etc/bupm/bupm.pub", std::ios::binary);
    if (!pkFile) {
        std::cerr << "Trusted public key not found in /etc/bupm/bupm.pub" << std::endl;
        return false;
    }
    pkFile.read(reinterpret_cast<char*>(pk), sizeof pk);
    if (pkFile.gcount() != sizeof pk) {
        std::cerr << "Invalid public key file size" << std::endl;
        return false;
    }
    
    unsigned char sig[crypto_sign_BYTES];
    std::ifstream sFile(sigPath, std::ios::binary);
    if (!sFile) {
        std::cerr << "Signature file not found: " << sigPath << std::endl;
        return false;
    }
    sFile.read(reinterpret_cast<char*>(sig), sizeof sig);
    if (sFile.gcount() != sizeof sig) {
        std::cerr << "Invalid signature file size" << std::endl;
        return false;
    }

    crypto_sign_state state;
    crypto_sign_init(&state);

    std::ifstream tFile(tarPath, std::ios::binary);
    if (!tFile) {
        std::cerr << "Failed to open source for verification: " << tarPath << std::endl;
        return false;
    }

    char buffer[4096];
    while (tFile.read(buffer, sizeof buffer) || tFile.gcount() > 0) {
        crypto_sign_update(&state, reinterpret_cast<unsigned char*>(buffer), tFile.gcount());
    }

    if (crypto_sign_final_verify(&state, sig, pk) != 0) {
        std::cerr << "Signature for package is invalid. This means the file may have been tampered with. Cancelled installation." << std::endl;
        return false;
    }

    std::cout << "Package signature verified successfully." << std::endl;
    return true;
}

int extract(const std::string& path, const std::string tgt) {
    struct archive* a = archive_read_new();
    struct archive* ext = archive_write_disk_new();
    struct archive_entry* entry;
    int r;

    archive_read_support_format_tar(a);
    archive_read_support_filter_all(a);

    archive_write_disk_set_options(ext, ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_PERM | ARCHIVE_EXTRACT_ACL | ARCHIVE_EXTRACT_FFLAGS | ARCHIVE_EXTRACT_SECURE_NODOTDOT | ARCHIVE_EXTRACT_SECURE_SYMLINKS);
    archive_write_disk_set_standard_lookup(ext);

    if ((r = archive_read_open_filename(a, path.c_str(), 10240)) != ARCHIVE_OK) {
        std::cerr << "Archive does not exist: " << archive_error_string(a) << std::endl;
        return 1;
    }

     while ((r = archive_read_next_header(a, &entry)) == ARCHIVE_OK) {
        std::string currentFile = tgt + "/" + archive_entry_pathname(entry);
        archive_entry_set_pathname(entry, currentFile.c_str());

        r = archive_write_header(ext, entry);
        if (r != ARCHIVE_OK) {
            std::cerr << "Warning: " << archive_error_string(ext) << "\n";
        } else {
            const void* buff;
            size_t size;
            la_int64_t offset;

            while (true) {
                r = archive_read_data_block(a, &buff, &size, &offset);
                if (r == ARCHIVE_EOF)
                    break;
                if (r != ARCHIVE_OK) {
                    std::cerr << "tar read error: " << archive_error_string(a) << "\n";
                    return 1;
                }
                r = archive_write_data_block(ext, buff, size, offset);
                if (r != ARCHIVE_OK) {
                    std::cerr << "tar write error: " << archive_error_string(ext) << "\n";
                    return 1;
                }
            }
        }
        r = archive_write_finish_entry(ext);
        if (r != ARCHIVE_OK) {
            std::cerr << "Finish entry error: " << archive_error_string(ext) << "\n";
            return 1;
        }
    }

    archive_read_close(a);
    archive_read_free(a);
    archive_write_close(ext);
    archive_write_free(ext);

    if (r != ARCHIVE_EOF) {
        std::cerr << "Archive read error: " << archive_error_string(a) << "\n";
        return 1;
    }

    return 0;
}

size_t callback(char* ptr, size_t size, size_t nmemb, void* userdata) {
    FILE* stream = static_cast<FILE*>(userdata);
    return fwrite(ptr, size, nmemb, stream);
}

bool download(const std::string& url, const std::string path) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "Failed to start curl \n";
        return false;
    }

    FILE* fp = fopen(path.c_str(), "wb");
    if (!fp) {
        std::cerr << "Failed to open file to write: " << path << std::endl;
        curl_easy_cleanup(curl);
        return false;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

    CURLcode res = curl_easy_perform(curl);
    fclose(fp);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        std::cerr << "Error while downloading with curl: " << curl_easy_strerror(res) << std::endl;
        return false;
    }
    return true;
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
    return base + "/" + first + "/" + pkgname + "/source.tar";
}

}

namespace fs = std::filesystem;

std::string Installer::mktemp() {
    fs::path tmp = fs::temp_directory_path();

    std::string dirname = "install_";

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 61);

    for (int i = 0; i < 8; ++i) {
	dirname += "0123456789abcdeffghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"[dis(gen)];
    }

    fs::path fp = tmp / dirname;

    if (fs::create_directory(fp)) {
	return fp.string();
    } else {
	std::cerr << "Failed to create temporary dir; are you root?"  << std::endl;
        return "";
    }
}

void Installer::install(int argc, char* argv[]) {
    std::string installtmp = mktemp();
    
    if (installtmp.empty()) {
        std::cerr << "Error while creating temporary dir. Are you root?" << std::endl;
        exit(1);
    }

    if (argc < 3) {
	std::cout << "Package not provided, try hello_world?" << std::endl;
	exit(1);
    }
    std::string pkgname = argv[2];

    std::cout << "Using temporary installation directory " << installtmp << " for installation of package " << pkgname << " using bupm-install(1) \n";
    fs::remove_all(installtmp);
    fs::create_directories(installtmp);

    std::string repoconf = "/etc/bupm/repositories";
    std::string baseurl = getrepopath(repoconf);

    if (baseurl.empty()) {
        std::cerr << "Error while reading repo file, empty." << std::endl;
        return;
    }

    std::string url = makeurl(baseurl, pkgname);

    std::cout << "Fetching package " << pkgname << " from " << url << std::endl;

    fs::path installtgt = fs::path(installtmp) / "source.tar";

    if (!download(url, installtgt.string())) {
        std::cerr << "Error while downloading source archive" << std::endl;
        exit(1);
    }

    std::string sigUrl = url + ".sig";
    fs::path sigtgt = fs::path(installtmp) / "source.tar.sig";

    std::cout << "Downloading signature from " << sigUrl << std::endl;
    if (!download(sigUrl, sigtgt.string())) {
        std::cerr << "Error: Could not download signature. Cancelling installation for security issues." << std::endl;
        exit(1);
    }

    if (!verify_signature(installtgt.string(), sigtgt.string())) {
        std::cerr << "Verification failed. Aborting installation." << std::endl;
        exit(1);
    }

    std::cout << "Package download complete. You might need to install some external dependencies prior installation. Consult bupm fetch " << pkgname << " for details." <<std::endl;
    std::cout << ".tar archive present in " << installtgt << std::endl;

    std::cout << "Start extracting for package " << pkgname << std::endl;

    int ret = extract(installtgt.string(), "/");
    if (ret != 0) {
        std::cerr << "Extract error." << std::endl;
        exit(1);
    }

    std::cout << std::endl << "Package " << pkgname << " successfully installed." << std::endl;
}
