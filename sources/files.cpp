#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <queue>
#include <map>
#include <algorithm>
#include "../includes/errors.h"
#include "../includes/files.h"

namespace sys = std::filesystem;

void extract_files(const sys::path& path, std::queue<std::string>* queue){
    for (const auto & file : sys::recursive_directory_iterator(path)) {
        if (file.path().extension().string() == ".txt"){
            queue->push(file.path().string());
        }
    }
}

void read_files(std::queue<std::string>* queue){
    for (unsigned long i=0; i<queue->size(); ++i){
        try{
            std::ifstream file(queue->front());
            std::string str;
            if(file) {
                std::ostringstream ss;
                ss << file.rdbuf();
                str = ss.str();
            }
            queue->push(str);
            queue->pop();
        }
        catch (...) {
            std::cerr << "Помилка читання вхідного файлу!" << std::endl;
            exit(TXT_FILE_READ_ERROR);
        }
    }
}

bool key_check(const std::map<std::string,int> &map, const std::string& el){
    std::map<std::string,int>::const_iterator itr = map.find(el);
    if(itr!=map.end()){
        return true;
    }
    return false;
}


const std::map<std::string, int>& split(std::string* str){
    unsigned long last = 0;
    std::map<std::string, int> words;

    for (unsigned long i=0; i<str->size(); ++i){
        if ((isspace(str->at(i)) or (i == str->size()-1 and not isspace(str->at(i)))) and (not isspace(str->at(last)))){
            std::string word;
            if (i != str->size()-1){
                word = str->substr(last, i-last);
            }
            else{
                word = str->substr(last, i+1-last);
            }
            std::transform(word.begin(), word.end(), word.begin(),
                           [](unsigned char c){ return std::tolower(c); });
            if (key_check(words, word)){
                ++words[word];
            }
            else{
                words.insert(std::pair<std::string, int>(word, 1));
            }
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
        if (key_check(global_ref, elem.first)){
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

const std::vector<std::pair<std::string, int>>& sort_by_func(const std::map<std::string, int>& words, int method){
    std::vector<std::pair<std::string, int>> sorted;
    for (auto pair: words){
        sorted.push_back(pair);
    }
    std::sort(sorted.begin(), sorted.end(), [method](auto i, auto j){return compare(i, j, method);} );
    return sorted;
}

void write(const std::string& name, const std::vector<std::pair<std::string, int>>& words){
    std::ofstream file_1(name);
    if (!file_1) {
        std::cerr << "Не вдалося відкрити файл для запису результату!" << std::endl;
        exit(RESULT_FILE_OPEN_ERROR);
    }
    for (auto pair: words){
        file_1 << pair.first << " " << pair.second << std::endl;
    }

    if (file_1.fail()) {
        std::cerr << "Помилка запису у вихідний файл!" << std::endl;
        exit(RESULT_WRITING_ERROR);
    }
    file_1.close();
}
