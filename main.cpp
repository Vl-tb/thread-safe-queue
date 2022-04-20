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
#include <boost/locale.hpp>

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
    size_t merging_threads = obj.merging_threads;

    #ifdef DEBUG
    int max_file_size = obj.max_file_size;
    int file_names_queue_max_size = obj.file_names_queue_max_size;
    int raw_files_queue_size = obj.raw_files_queue_size;
    int dictionaries_queue_size = obj.dictionaries_queue_size;
    std::cout << merging_threads << max_file_size << dictionaries_queue_size << std::endl;
    #endif

    if (!std::filesystem::exists(indir)) {
        std::cerr << "Input dir does not exist" << std::endl;
        exit(INDEXING_PATH_ERROR);
    }
     if (!std::filesystem::exists(out_path_a.parent_path())) {
         std::cerr << "Output path 'a' does not exist" << std::endl;
         exit(RESULT_FILE_OPEN_ERROR);
     }
     if (!std::filesystem::exists(out_path_n.parent_path())) {
         std::cerr << "Output path 'n' does not exist" << std::endl;
         exit(RESULT_FILE_OPEN_ERROR);
     }

    std::locale loc = boost::locale::generator().generate("en_US.UTF-8");

    std::chrono::high_resolution_clock::time_point find_start;
    std::chrono::high_resolution_clock::time_point find_end;
    std::chrono::high_resolution_clock::time_point read_start;
    std::chrono::high_resolution_clock::time_point read_end;
    std::chrono::high_resolution_clock::time_point write_start;
    std::chrono::high_resolution_clock::time_point write_end;
    std::chrono::high_resolution_clock::time_point total_start;
    std::chrono::high_resolution_clock::time_point total_end;

    if (index_threads == 0){
        std::deque<sys::path> deque;
        std::deque<std::pair<sys::path, std::string>> text_deque;

        find_start = get_current_time_fenced();
        try{
            extract_files(indir, &deque);
        }
        catch(...){
            std::cerr << "Не знайдено файл чи директорію за заданим шляхом!" << std::endl;
        }
        find_end = get_current_time_fenced();

        total_start = get_current_time_fenced();

        read_start = get_current_time_fenced();
        read_files(&deque, &text_deque);
        read_end = get_current_time_fenced();

        std::map<std::string, int> global;


        for(auto& elem : text_deque){
            std::map<std::string, int> local = split(&elem.second, loc);

            #ifdef DEBUG
            std::fstream outfile;
               outfile.open("./results/maps_0.txt", std::fstream::app);
            outfile << "FILE: " << elem.first << std::endl;
            for(const auto& e : local)
            {
               outfile << e.first << " " << e.second << std::endl;
            }
            outfile << "END OF FILE " << elem.first << std::endl << std::endl;
            outfile.close();
            #endif

            merge(local, &global);
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
        std::vector<std::thread> merging_worker;
        my_mt_thread<sys::path> filequ(1000);
        my_mt_thread<std::pair<sys::path, std::string>> stringqu(200);
        my_mt_thread<std::map<std::string, int>> dictionaries(1000);


        find_start = get_current_time_fenced();
        std::thread file_searcher_worker(extract_files_mt, indir, &filequ);

        total_start = get_current_time_fenced();
        read_start = get_current_time_fenced();
        std::thread file_reader_worker(read_files_mt, &filequ, &stringqu);

        for (size_t i = 0; i < index_threads; i++) {
            index_worker.emplace_back(std::thread(index_work_mt, &stringqu, &dictionaries, loc));
        }

        for (size_t i = 0; i < merging_threads; i++) {
            merging_worker.emplace_back(std::thread(merge_work_mt, &dictionaries));
        }

        file_searcher_worker.join();
        find_end = get_current_time_fenced();
        file_reader_worker.join();
        read_end = get_current_time_fenced();


        for (std::thread & th : index_worker) {
            th.join();
        }

        for (std::thread & th : merging_worker) {
            th.join();
        }

        total_end = get_current_time_fenced();

        std::map<std::string, int> global = dictionaries.pop_front();
        std::vector<std::pair<std::string, int>> sorted;
        std::vector<std::pair<std::string, int>> sorted_1;
        sorted = sort_by_func(global, 1);
        sorted_1 = sort_by_func(global, 0);

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
