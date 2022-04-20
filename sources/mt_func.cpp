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


void index_work_mt(my_mt_thread<std::pair<sys::path, std::string>>* deque,  my_mt_thread<std::map<std::string, int>>* dictionaries, const std::locale& loc) {

    while(true) {
        auto elem = deque->pop_front();
        if(elem.second.empty()) {
            if (!dictionaries->get_status()) {
                std::map<std::string, int> pill;
                pill["./"];
                dictionaries->push_back(pill);
                dictionaries->set_status(true);
            }
            deque->push_front(std::make_pair(elem.first, ""));
            return;
        }
        const std::string &temp_front = elem.second;
        std::map<std::string, int> local = split(&temp_front, loc);
        dictionaries->push_front(local);
    }
}

void merge_work_mt(my_mt_thread<std::map<std::string, int>>* dictionaries) {
    while (true) {
        auto elem_1 = dictionaries->pop_front();
        if (elem_1["./"] == 0) {
            dictionaries->push_back(elem_1);
            return;
        }
        auto elem_2 = dictionaries->pop_front();
        if (elem_2["./"] == 0) {
            dictionaries->push_back(elem_2);
            return;
        }
        std::map<std::string, int> *elem_2p;
        elem_2p = &elem_2;
        merge(elem_1, elem_2p);
        dictionaries->push_front(elem_2);
    }
}
