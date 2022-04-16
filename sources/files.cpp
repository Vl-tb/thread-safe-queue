#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <deque>
#include <map>
#include <algorithm>
#include "../includes/errors.h"
#include "../includes/files.h"

namespace sys = std::filesystem;


void extract_files(const sys::path& path, std::deque<std::string>* deque){
    for (const auto & file : sys::recursive_directory_iterator(path)) {
        if (file.path().extension().string() == ".txt" && (sys::file_size(file.path())) > 1) {
            deque->push_back(file.path().string());
        }
    }
}

void read_files(std::deque<std::string>* deque){
    for (unsigned long i=0; i<deque->size(); ++i){
        try{
            std::ifstream file(deque->front());
            std::string str;
            if(file) {
                std::ostringstream ss;
                ss << file.rdbuf();
                str = ss.str();
            }
            deque->push_back(str);
            deque->pop_front();
        }
        catch (...) {
            std::cerr << "Помилка читання вхідного файлу!" << std::endl;
//            exit(TXT_FILE_READ_ERROR);
        }
    }
}



std::map<std::string, int> split(const std::string* str){
    unsigned long last = 0;
    std::map<std::string, int> words;

    for (unsigned long i=0; i<str->size(); ++i){
        if ((isspace(str->at(i)) || (i == str->size()-1 && !isspace(str->at(i)))) && (!isspace(str->at(last)))){
            std::string word;
            if (i != str->size()-1){
                word = str->substr(last, i-last);
            }
            else{
                word = str->substr(last, i+1-last);
            }
            std::transform(word.begin(), word.end(), word.begin(),
                           [](unsigned char c){ return std::tolower(c); });
            ++words[word];
            last = i;
        }
        else if (not isspace(str->at(i)) and isspace(str->at(last))){
            last = i;
        }
    }
    return words;
}

void merge(const std::map<std::string, int>& local, std::map<std::string, int>* global){
    std::map<std::string, int>& global_ref = *global;
    for (auto const& elem: local){
        auto itr = global_ref.find(elem.first);
        if (itr!=global_ref.end()){
            global_ref[elem.first]+= elem.second;
        }
        else{
            global_ref.insert(std::pair<std::string, int>(elem.first, 1));
        }
    }
}

bool compare(const std::pair<std::string, int>& first, const std::pair<std::string, int>& second, int param){
    if (param){
        return (first.first < second.first);
    }
    else{
        if (first.second != second.second){
            return (first.second > second.second);
        }
        return (first.first < second.first);
    }
}


std::vector<std::pair<std::string, int>> sort_by_func(const std::map<std::string, int>& words, int method){
    std::vector<std::pair<std::string, int>> sorted;
    for (auto &pair : words){
        sorted.emplace_back(pair);
    }
    std::sort(sorted.begin(), sorted.end(), [method](auto i, auto j){return compare(i, j, method);} );
    return sorted;
}

void write(const sys::path& name, const std::vector<std::pair<std::string, int>>& words){
    std::ofstream file_1(name);
    if (!file_1) {
        std::cerr << "Не вдалося відкрити файл для запису результату!" << std::endl;
        exit(RESULT_FILE_OPEN_ERROR);
    }
    for (auto &pair: words){
        file_1 << pair.first << " " << pair.second << std::endl;
    }

    if (file_1.fail()) {
        std::cerr << "Помилка запису у вихідний файл!" << std::endl;
        exit(RESULT_WRITING_ERROR);
    }
    file_1.close();
}
