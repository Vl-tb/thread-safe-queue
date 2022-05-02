#ifndef MT_FUNC_HPP
#define MT_FUNC_HPP
#include <iostream>
#include <deque>
#include <map>
#include <filesystem>
#include <chrono>
#include <atomic>
#include <tbb/concurrent_queue.h>

#include "../includes/parser.h"
#include "../includes/my_mt_thread.hpp"


namespace sys = std::filesystem;

void extract_files_mt(const sys::path& path, tbb::concurrent_bounded_queue<sys::path>* deque, size_t max_file_size);
void read_files_mt(tbb::concurrent_bounded_queue<sys::path>* file_deque, tbb::concurrent_bounded_queue<std::pair<sys::path, std::string>>* string_deque);
void index_work_mt(tbb::concurrent_bounded_queue<std::pair<sys::path, std::string>>* deque,
                   my_mt_thread<std::map<std::string, int>>* dictionaries, const std::locale& loc, size_t ind_thr);
void merge_work_mt(my_mt_thread<std::map<std::string, int>>* dictionaries);
#endif // MT_FUNC_HPP
