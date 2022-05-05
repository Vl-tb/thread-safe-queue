#include <iostream>
#include <fstream>
#include <sstream>
#include "../includes/files.h"
#include "../includes/mt_func.hpp"


void extract_files_mt(const sys::path& path, tbb::concurrent_bounded_queue<sys::path>* deque, size_t max_file_size) {
    for (const auto & file : sys::recursive_directory_iterator(path)) {
        if ((file.path().extension().string() == ".txt" || file.path().extension().string() == ".zip") && (sys::file_size(file.path()) > 1) && (sys::file_size(file.path()) < max_file_size)){ //< 100kb - skip
            deque->push(file.path().string());
        }
    }
    deque->push("./");
}

void read_files_mt(tbb::concurrent_bounded_queue<sys::path>* file_deque, tbb::concurrent_bounded_queue<std::pair<sys::path, std::string>>* string_deque) {
    while(true) {
        sys::path elem;
        file_deque->pop(elem);
        if (!elem.compare("./")) { //must be 0 to be equal
            string_deque->push(std::make_pair(elem, ""));
            return;
        }
        try{
            auto str = read_binary_file(elem);
            string_deque->push(std::make_pair(elem, str));
        }
        catch (...) {
            std::cerr << "Помилка читання вхідного файлу: " << elem << std::endl;
        }
    }
}

void index_work_mt(tbb::concurrent_bounded_queue<std::pair<sys::path, std::string>>* deque, tbb::concurrent_hash_map<std::string, int>* global, const std::locale& loc) {

    while(true) {
        std::pair<sys::path, std::string> elem;
        deque->pop(elem);
        if(elem.second.empty()) {
            deque->push(std::make_pair(elem.first, ""));
            return;
        }
        std::map<std::string, int> local;
        if (elem.first.extension().string() == ".txt") {
            local = split(&elem.second, loc);
        }
        else if (elem.first.extension().string() == ".zip") {
            auto data = extract_archive_files(elem.second);
            local = split(&data, loc);
        }

//        merge(&global, local);
    }
}
