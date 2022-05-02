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


void index_work_mt(tbb::concurrent_bounded_queue<std::pair<sys::path, std::string>>* deque,  my_mt_thread<std::map<std::string, int>>* dictionaries, const std::locale& loc, size_t ind_thr) {

    while(true) {
        std::pair<sys::path, std::string> elem;
        deque->pop(elem);
        if(elem.second.empty()) {
            dictionaries->set_counter_inc(1);
            if (dictionaries->get_counter() == ind_thr) {
                dictionaries->close();
            }
            if (!dictionaries->get_status()) {
                std::map<std::string, int> pill;
                pill["./"];
                dictionaries->push_back(pill);
                dictionaries->set_status(true);
            }
            deque->push(std::make_pair(elem.first, ""));
            return;
        }
//        const std::string &temp_front = elem.second;
//        std::map<std::string, int> local = split(&temp_front, loc);
        std::map<std::string, int> local;
        if (elem.first.extension().string() == ".txt") {
            local = split(&elem.second, loc);
        }
//        else if (elem.first.extension().string() == ".zip") {
//            auto data = extract_archive_files(elem.second);
//            local = split(&data, loc);
//        }
        dictionaries->push_front(local);
    }
}

void merge_work_mt(my_mt_thread<std::map<std::string, int>>* dictionaries) {
    while (true) {
        auto elem_1 = dictionaries->pop_front();
        if (elem_1.find("./") != elem_1.end()) {
            dictionaries->push_back(elem_1);
            if (dictionaries->is_closed()) {
                return;
            }
        }
        else {
            auto elem_2 = dictionaries->pop_front();
            if (elem_2.find("./") != elem_2.end()) {
                dictionaries->push_back(elem_2);
                dictionaries->push_front(elem_1);
                if (dictionaries->is_closed()) {
                    return;
                }
            }
            else {
                std::map<std::string, int> *elem_2p;
                elem_2p = &elem_2;
                merge(elem_1, elem_2p);
                dictionaries->push_front(elem_2);
            }
        }
    }
}
