#ifndef QUEUE_FILES_H
#define QUEUE_FILES_H

#include <queue>
#include <map>
#include <filesystem>
#include <chrono>
#include <atomic>
#include "../includes/parser.h"
#include <boost/program_options.hpp>

namespace sys = std::filesystem;

void extract_files(const sys::path& path, std::queue<std::string>* queue);
void read_files(std::queue<std::string>* queue);
bool key_check(const std::map<std::string,int>& map, const std::string& el);
std::map<std::string, int> split(std::string* str);
void merge(const std::map<std::string, int>& local, std::map<std::string, int>* global);
bool compare(const std::pair<std::string, int>& first, const std::pair<std::string, int>& second, int param);
std::vector<std::pair<std::string, int>> sort_by_func(const std::map<std::string, int>& words, int method);
void write(const std::string& name, const std::vector<std::pair<std::string, int>>& words);

inline std::chrono::high_resolution_clock::time_point get_current_time_fenced()
{
    std::atomic_thread_fence(std::memory_order_seq_cst);
    auto res_time = std::chrono::high_resolution_clock::now();
    std::atomic_thread_fence(std::memory_order_seq_cst);
    return res_time;
}

template <class D>
long long to_us(const D &d)
{
    return std::chrono::duration_cast<std::chrono::microseconds>(d).count();
}

#endif //QUEUE_FILES_H
