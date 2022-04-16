//// This is a personal academic project. Dear PVS-Studio, please check it.
//// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#include <iostream>
#include <vector>
#include <deque>
#include <fstream>
#include <thread>
#include "includes/errors.h"
#include "includes/files.h"
#include "includes/mt_func.hpp"
#include "includes/parser.h"

#include "includes/my_mt_thread.hpp"


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
    size_t index_threads = obj.indexing_threads;

    std::chrono::high_resolution_clock::time_point find_start;
    std::chrono::high_resolution_clock::time_point find_end;
    std::chrono::high_resolution_clock::time_point read_start;
    std::chrono::high_resolution_clock::time_point read_end;
    std::chrono::high_resolution_clock::time_point write_start;
    std::chrono::high_resolution_clock::time_point write_end;
    std::chrono::high_resolution_clock::time_point total_start;
    std::chrono::high_resolution_clock::time_point total_end;

    if (index_threads == 0){
        std::deque<std::string>* chr;
        std::deque<std::string> deque;
        chr = &deque;

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
        int amount = deque.size();

        for(int i=0; i<amount; ++i){
            std::map<std::string, int> local = split(&deque.front());

            merge(local, &global);
            deque.pop_front();
        }
        total_end = get_current_time_fenced();

        std::vector<std::pair<std::string, int>> sorted;
        std::vector<std::pair<std::string, int>> sorted_1;
        sorted = sort_by_func(global, 1);
        sorted_1 = sort_by_func(global, 0);

        write_start = get_current_time_fenced();
        write(out_path_a, sorted);
        write(out_path_n, sorted_1);
        write_end = get_current_time_fenced();

    }
    else{
        std::vector<std::thread> index_worker;
        my_mt_thread<sys::path> filequ(1000);
        my_mt_thread<std::pair<sys::path, std::string>> stringqu(200);
        my_mt_map<std::string, int> global;

        find_start = get_current_time_fenced();
        std::thread file_searcher_worker(extract_files_mt, indir, &filequ);

        total_start = get_current_time_fenced();
        read_start = get_current_time_fenced();
        std::thread file_reader_worker(read_files_mt, &filequ, &stringqu);

        for (size_t i = 0; i < index_threads; i++) {
            index_worker.emplace_back(std::thread(index_work_mt, &stringqu, &global));
        }

        file_searcher_worker.join();
        find_end = get_current_time_fenced();
        file_reader_worker.join();
        read_end = get_current_time_fenced();


        for (std::thread & th : index_worker) {
            th.join();
        }

        total_end = get_current_time_fenced();

        std::vector<std::pair<std::string, int>> sorted;
        std::vector<std::pair<std::string, int>> sorted_1;
        sorted = sort_by_func(global.cast_to_map(), 1);
        sorted_1 = sort_by_func(global.cast_to_map(), 0);

        write_start = get_current_time_fenced();
        write(out_path_a, sorted);
        write(out_path_n, sorted_1);
        write_end = get_current_time_fenced();

    }

    std::cout << "Total=" << to_us(total_end-total_start) << std::endl;
    std::cout << "Reading=" << to_us(read_end-read_start) << std::endl;
    std::cout << "Finding=" << to_us(find_end-find_start) << std::endl;
    std::cout << "Writing=" << to_us(write_end-write_start) << std::endl;

    return 0;
}
