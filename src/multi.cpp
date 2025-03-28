#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <queue>
#include <future>
#include <mutex>
#include <chrono>

namespace fs = std::filesystem;

const std::string ROOT_DIR = "C:\\";
std::mutex mtx; // Mutex for thread safety

// Optimized single function for formatting file sizes
std::pair<double, std::string> format_filesize(double fsize) {
    const char* units[] = {"bytes", "KB", "MB", "GB"};
    int index = 0;
    while (fsize > 1024 && index < 3) {
        fsize /= 1024;
        index++;
    }
    return {fsize, units[index]};
}

// Multi-threaded directory processing function with counters
void process_directory(const std::string& root_dir,
                        int depth,
                        std::vector<std::pair<std::string, double>>& fsizes, 
                        std::vector<std::string>& error_files,
                        size_t& file_count, size_t& dir_count) {
    std::error_code ec;
    if (!fs::is_directory(root_dir, ec) || ec) return;

    { // Mutex lock for directory count increment
        std::lock_guard<std::mutex> lock(mtx);
        dir_count++; // Increment directory count
    }

    std::vector<std::future<void>> futures;
    std::vector<std::string> subdirs;

    for (const auto& dir_entry : fs::directory_iterator(root_dir, fs::directory_options::skip_permission_denied, ec)) {
        if (depth == 0) return;
        if (ec) continue;
        
        const auto& path = dir_entry.path();
        if (path.filename().string().rfind("$", 0) == 0) continue; // Skip system folders

        if (dir_entry.is_directory()) {
            subdirs.push_back(path.string());
        } else {
            try {
                double file_size = static_cast<double>(dir_entry.file_size());

                { // Mutex lock for file count and vector update
                    std::lock_guard<std::mutex> lock(mtx);
                    fsizes.emplace_back(path.string(), file_size);
                    file_count++; // Increment file count
                }
            } catch (const fs::filesystem_error&) {
                std::lock_guard<std::mutex> lock(mtx);
                error_files.push_back(path.string());
            }
        }
    }

    // Process subdirectories asynchronously
    for (const auto& subdir : subdirs) {
        futures.push_back(std::async(std::launch::async, process_directory, subdir, depth-1, std::ref(fsizes), std::ref(error_files), std::ref(file_count), std::ref(dir_count)));
    }

    // Ensure all async tasks finish
    for (auto& future : futures) {
        future.get();
    }
}

int main(int argc, char** argv) {
    int depth = 1;
    int num_files_output = 10;
    size_t file_count = 0; // Counter for files found
    size_t dir_count = 0;  // Counter for directories traversed

    for (int i = 1; i < argc - 1; i++) { // Prevent out-of-bounds access
        if (std::string(argv[i]) == "-depth") {
            depth = std::stoi(argv[i + 1]);
        }
        if (std::string(argv[i]) == "-num") { // number of files outputted to console, default top 10
            num_files_output = std::stoi(argv[i + 1]);
        }
    }

    std::vector<std::pair<std::string, double>> fsizes;
    std::vector<std::string> error_files; // Stores inaccessible files

    auto t1 = std::chrono::high_resolution_clock::now();
    process_directory(ROOT_DIR, depth, fsizes, error_files, file_count, dir_count);

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

    // Print statistics
    std::cout << "\nSummary:\n";
    std::cout << "Total files found: " << file_count << '\n';
    std::cout << "Total directories traversed: " << dir_count << '\n';

    // Print error files
    // if (!error_files.empty()) {
    //     std::cerr << "\nSkipped " << error_files.size() << " files due to access errors:\n";
    //     for (const auto& err_file : error_files) {
    //         std::cerr << err_file << '\n';
    //     }
    // }

    auto t2 = std::chrono::high_resolution_clock::now();
    std::chrono::microseconds duration = std::chrono::duration_cast<std::chrono::microseconds>(t2-t1);
    float seconds = duration.count()/1'000'000.0;
    std::cout << "duration: " << seconds << " s" << '\n';

    return 0;
}
