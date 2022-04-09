
#include <fstream>
#include <sstream>
#include <algorithm>
#include <utility>
#include "../includes/errors.h"
#include "../includes/files.h"
#include "../includes/mt_func.hpp"


void extract_files_mt(const sys::path& path, my_mt_thread<std::string>* deque) {
    for (const auto & file : sys::recursive_directory_iterator(path)) {
        if (file.path().extension().string() == ".txt"){ //< 100kb - skip
            deque->push_back(file.path().string());
        }
    }
    deque->push_back("./");
}

void read_files_mt(my_mt_thread<std::string>* file_deque, my_mt_thread<std::pair<std::string, std::string>>* string_deque) {
    while(true) {
        auto elem = file_deque->front();
        if (!elem.compare("./")) { //must be 0 to be equal
            string_deque->push_back(std::make_pair(elem, ".//"));
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
            std::cerr << "Помилка читання вхідного файлу!" << std::endl;
//            exit(TXT_FILE_READ_ERROR);
        }
    }
}

void merge_mt(const std::map<std::string, int>& local, my_mt_map<std::string, int>* global) {
    my_mt_map<std::string, int>& global_ref = *global;
    for (auto const& elem: local){
        global_ref.merge(elem);
    }
}

void index_work_mt(my_mt_thread<std::pair<std::string, std::string>>* deque,  my_mt_map<std::string, int>* global) {
    while(true) {
        auto elem = deque->pop_front();
        if(!elem.second.compare(".//")) {
            deque->push_back(std::make_pair(elem.first, ".//"));
            return;
        }
        const std::string &temp_front = elem.second;
        std::map<std::string, int> local = split(&temp_front);

        merge_mt(local, global);

    }
}
