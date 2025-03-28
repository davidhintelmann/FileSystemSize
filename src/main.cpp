#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <queue>
#include <chrono>
#include <stdio.h>

namespace fs = std::filesystem;
std::string ROOT_DIR = "C:\\";

std::pair<double, std::string> format_filesize(uintmax_t fsize) {
    if (fsize > 1024 && fsize < 1024*1024) {
        return std::make_pair(double(fsize)/1024.0, "KB");
    } else if (fsize > 1024*1024 && fsize < 1024*1024*1024) {
        return std::make_pair(double(fsize)/(1024.0*1024.0), "MB");
    } else if (fsize > 1024*1024*1024) {
        return std::make_pair(double(fsize)/(1024.0*1024.0*1024.0), "GB");
    } else {
        return std::make_pair(double(fsize), "bytes");
    }
}

std::pair<double, std::string> format_filesize(double fsize) {
    const char* units[] = {"bytes", "KB", "MB", "GB"};
    int index = 0;
    while (fsize > 1024 && index < 3) {
        fsize /= 1024;
        index++;
    }
    return {fsize, units[index]};
}

void process_directory(std::string& root_dir, std::vector<fs::directory_entry>& dirs, std::vector<std::pair<std::string, double>>& fsizes, std::vector<std::string>& error_files) {
    std::error_code ec;

    if (!fs::is_directory(root_dir, ec) || ec) {
        // std::cerr << "Skipping inaccessible directory: " << root_dir << " (" << ec.message() << ")\n";
        return;
    }

    fs::directory_iterator iter(root_dir, fs::directory_options::skip_permission_denied, ec);
    if (ec) {
        // std::cerr << "Error accessing directory: " << root_dir << " (" << ec.message() << ")\n";
        return;
    }

    for (const auto& dir_entry : iter) {
        const std::string& filename = dir_entry.path().filename().string();
        if (filename.rfind("$", 0) == 0) {
            continue;
        } else if (dir_entry.is_directory()) {
            dirs.push_back(dir_entry);
            continue;
        } else {
            try {
                double file_size = static_cast<double>(dir_entry.file_size());
                fsizes.emplace_back(dir_entry.path().string(), file_size);
            } catch(const std::exception& e) {
                error_files.push_back(dir_entry.path().string());
            }
        }
    }
}

int main(int argc, char **argv) {
    int depth = 1;
    int num_files_output = 10;

    for (int i = 1; i < argc-1; i++) {
        if (std::string(argv[i]) == "-depth") {
            depth = std::stoi(argv[i+1]);
        }
    }

    std::vector<fs::directory_entry> dirs{};
    std::vector<std::pair<std::string, double>> fsizes{};
    std::vector<std::string> error_files;

    auto t1 = std::chrono::high_resolution_clock::now();
    process_directory(ROOT_DIR, dirs, fsizes, error_files);
    for (int j = 1; j < depth && !dirs.empty(); j++) {
        std::vector<fs::directory_entry> next_dirs;
        for (auto it = dirs.begin(); it != dirs.end(); ) {
            std::string sub_dir = it->path().string();
            if (sub_dir == "C:\\Documents and Settings" || sub_dir == "C:\\Recovery" || sub_dir == "C:\\Windows") {
                it = dirs.erase(it);
                continue;
            }
            process_directory(sub_dir, next_dirs, fsizes, error_files);
            it = dirs.erase(it); // Avoid invalidating iterators
        }
        dirs.swap(next_dirs);
    }

    // Use a min-heap to keep track of top N largest files efficiently
    using FileSizePair = std::pair<double, std::string>;
    std::priority_queue<FileSizePair, std::vector<FileSizePair>, std::greater<>> top_files;

    for (const auto& [filename, size] : fsizes) {
        if (top_files.size() < num_files_output) {
            top_files.emplace(size, filename);
        } else if (size > top_files.top().first) {
            top_files.pop();
            top_files.emplace(size, filename);
        }
    }

    std::vector<FileSizePair> sorted_files;
    while (!top_files.empty()) {
        sorted_files.push_back(top_files.top());
        top_files.pop();
    }
    std::reverse(sorted_files.begin(), sorted_files.end());

    std::cout << "Top " << num_files_output << " Largest Files:\n";
    for (const auto& [size, filename] : sorted_files) {
        auto [formatted_size, unit] = format_filesize(size);
        std::cout << filename << ": " << formatted_size << " " << unit << '\n';
    }

    auto t2 = std::chrono::high_resolution_clock::now();
    std::chrono::microseconds duration = std::chrono::duration_cast<std::chrono::microseconds>(t2-t1);
    float seconds = duration.count()/1'000'000.0;
    std::cout << "duration: " << seconds << " s" << '\n';

    // std::cout << "directories left to traverse: " << '\n';
    // for (int i = 0; i < dirs.size(); i++) {
    //     std::cout << dirs[i] << '\n';
    // }

    return 0;
}