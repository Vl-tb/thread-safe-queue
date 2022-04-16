#ifndef MT_FUNC_HPP
#define MT_FUNC_HPP
#include <iostream>
#include <deque>
#include <map>
#include <filesystem>
#include <chrono>
#include <atomic>


#include "../includes/parser.h"
#include "../includes/my_mt_thread.hpp"


namespace sys = std::filesystem;

void extract_files_mt(const sys::path& path, my_mt_thread<sys::path>* deque);
void read_files_mt(my_mt_thread<sys::path>* file_deque, my_mt_thread<std::pair<sys::path, std::string>>* string_deque);
void index_work_mt(my_mt_thread<std::pair<sys::path, std::string>>* deque,  my_mt_map<std::string, int>* global);
#endif // MT_FUNC_HPP
