#include <iostream>
#include <fstream>
#include <sstream>
#include "../includes/files.h"
#include "../includes/mt_func.hpp"


void extract_files_mt(const sys::path& path, my_mt_thread<sys::path>* deque) {
    for (const auto & file : sys::recursive_directory_iterator(path)) {
        if ((file.path().extension().string() == ".txt") && (sys::file_size(file.path())) > 1){ //< 100kb - skip
            deque->push_back(file.path().string());
        }
    }
    deque->push_back("./");
}

void read_files_mt(my_mt_thread<sys::path>* file_deque, my_mt_thread<std::pair<sys::path, std::string>>* string_deque) {
    while(true) {
        auto elem = file_deque->front();
        if (!elem.compare("./")) { //must be 0 to be equal
            string_deque->push_back(std::make_pair(elem, ""));
            return;
        }
        try{
            std::ifstream file(elem);
            std::string str;
            if(file) {
                std::ostringstream ss;
                ss << file.rdbuf();
                str = ss.str();
            }
            string_deque->push_back(std::make_pair(elem, str));
            file_deque->pop_front();
        }
        catch (...) {
            std::cerr << "Помилка читання вхідного файлу: " << elem << std::endl;
        }
    }
}


void index_work_mt(my_mt_thread<std::pair<sys::path, std::string>>* deque,  my_mt_map<std::string, int>* global, const std::locale& loc) {

    while(true) {
        auto elem = deque->pop_front();
        if(elem.second.empty()) {
            deque->push_back(std::make_pair(elem.first, ""));
            return;
        }
        const std::string &temp_front = elem.second;
        std::map<std::string, int> local = split(&temp_front, loc);

        my_mt_map<std::string, int>& global_ref = *global;
        global_ref.merge(local);
    }
}
