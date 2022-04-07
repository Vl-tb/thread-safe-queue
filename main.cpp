//// This is a personal academic project. Dear PVS-Studio, please check it.
//// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include <iostream>
#include <vector>
#include <queue>
#include <fstream>
#include "includes/errors.h"
#include "includes/files.h"
#include "includes/parser.h"

namespace sys = std::filesystem;

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Невірна кількість аргументів!" << std::endl;
        exit(ARGUMENTS_ERROR);
    }
    config_options_t obj(argv);

    sys::path indir = obj.indir;
    sys::path out_path_a = obj.out_by_a;
    sys::path out_path_n = obj.out_by_n;
    int threads_n = obj.indexing_threads;

    std::chrono::high_resolution_clock::time_point find_start;
    std::chrono::high_resolution_clock::time_point find_end;
    std::chrono::high_resolution_clock::time_point read_start;
    std::chrono::high_resolution_clock::time_point read_end;
    std::chrono::high_resolution_clock::time_point write_start;
    std::chrono::high_resolution_clock::time_point write_end;
    std::chrono::high_resolution_clock::time_point total_start;
    std::chrono::high_resolution_clock::time_point total_end;

    if (threads_n == 0){
        std::queue<std::string>* chr;
        std::queue<std::string> queue;
        chr = &queue;

        find_start = get_current_time_fenced();
        try{
            extract_files(indir, chr);
        }
        catch(...){
            std::cerr << "Не знайдено файл чи директорію за заданим шляхом!" << std::endl;
            exit(INDEXING_PATH_ERROR);
        }
        find_end = get_current_time_fenced();

        total_start = get_current_time_fenced();

        read_start = get_current_time_fenced();
        read_files(chr);
        read_end = get_current_time_fenced();

        std::map<std::string, int> global;
        int amount = queue.size();

        for(int i=0; i<amount; ++i){
            const std::map<std::string, int>& local = split(&queue.front());

            merge(local, &global);
            queue.pop();
        }
        total_end = get_current_time_fenced();

        const std::vector<std::pair<std::string, int>>& sorted;
        const std::vector<std::pair<std::string, int>>& sorted_1;
        sorted = sort_by_func(global, 1);
        sorted_1 = sort_by_func(global, 0);

        write_start = get_current_time_fenced();
        write(out_path_a, sorted);
        write(out_path_n, sorted_1);
        write_end = get_current_time_fenced();

    }
    else{
        //TODO Multithreading
    }

    std::cout << "Total=" << to_us(total_end-total_start) << std::endl;
    std::cout << "Reading=" << to_us(read_end-read_start) << std::endl;
    std::cout << "Finding=" << to_us(find_end-find_start) << std::endl;
    std::cout << "Writing=" << to_us(write_end-write_start);

    return 0;
}
