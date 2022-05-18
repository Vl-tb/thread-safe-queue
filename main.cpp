#include <iostream>
#include <deque>
#include <fstream>
#include <thread>
#include <boost/locale.hpp>
#include <tbb/flow_graph.h>
#include <tbb/concurrent_hash_map.h>

#include "includes/errors.h"
#include "includes/files.h"

namespace sys = std::filesystem;


#ifdef DEBUGMODE
void write_map(std::string pth, std::map<std::string, int> map) {
    std::ofstream file_1(pth);
    for (auto& w : map) {
        file_1 << w.first << " " << w.second << std::endl;
    }
    file_1.close();
}
#endif

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
    size_t max_file_size = obj.max_file_size;
    int file_names_queue_max_size = obj.file_names_queue_max_size;
    int raw_files_queue_size = obj.raw_files_queue_size;
    int dictionaries_queue_size = obj.dictionaries_queue_size;


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


    tbb::flow::graph g;
    tbb::concurrent_hash_map<std::string, int> global;
    std::locale loc = boost::locale::generator().generate("en_US.UTF-8");

    std::chrono::high_resolution_clock::time_point find_start;
    std::chrono::high_resolution_clock::time_point find_end;
    std::chrono::high_resolution_clock::time_point read_start;
    std::chrono::high_resolution_clock::time_point read_end;
    std::chrono::high_resolution_clock::time_point write_start;
    std::chrono::high_resolution_clock::time_point write_end;
    std::chrono::high_resolution_clock::time_point total_start;
    std::chrono::high_resolution_clock::time_point total_end;


    std::vector<sys::path> pathes;

    for (const auto & file : sys::recursive_directory_iterator(indir)) {
        if ((file.path().extension().string() == ".txt" || file.path().extension().string() == ".zip") && (sys::file_size(file.path()) > 1) && (sys::file_size(file.path()) < max_file_size)){ //< 100kb - skip
            pathes.push_back(file.path().string());
        }
    }
    size_t curr_file = 0, num_of_files = pathes.size();


//     Graph nodes
    tbb::flow::input_node<sys::path> start_node(g, [&](tbb::flow_control& fc) -> sys::path {
        if (curr_file < num_of_files) {
            sys::path pth = pathes[curr_file];
            ++curr_file;
            return pth;
        } else {
            fc.stop();
            return std::string();
        }
    });

    tbb::flow::limiter_node<sys::path> path_l(g, file_names_queue_max_size);

    tbb::flow::function_node<sys::path, std::pair<sys::path, std::string>> reader_node(g, 1, [&](const sys::path& filename) -> std::pair<sys::path, std::string> {
        try{
            auto str = read_binary_file(filename);
            path_l.decrementer().try_put(tbb::flow::continue_msg());
            return std::make_pair(filename, str);
        }
        catch (...) {
            std::cout << "Cannot read file " << filename << std::endl;
            path_l.decrementer().try_put(tbb::flow::continue_msg());
            return std::make_pair(filename, "");
        }
    });

    tbb::flow::limiter_node<std::pair<sys::path, std::string>> text_l(g, raw_files_queue_size);

    tbb::flow::function_node< std::pair<sys::path, std::string>, std::pair<sys::path, std::map<std::string, int>> > indexing_nodes(g, index_threads, [&](const std::pair<sys::path, std::string>& text_pair) -> std::pair<sys::path, std::map<std::string, int>> {

        if (text_pair.first.extension().string() == ".txt") {
            std::map<std::string, int> local = split(&text_pair.second, loc);
            text_l.decrementer().try_put(tbb::flow::continue_msg());
            return std::make_pair(text_pair.first, local);
        } else if (text_pair.first.extension().string() == ".zip") {
            auto data = extract_archive_files(text_pair.second);
            std::map<std::string, int> local = split(&data, loc);
            text_l.decrementer().try_put(tbb::flow::continue_msg());
            return std::make_pair(text_pair.first, local);
        }
        std::map<std::string, int> mp;
        mp[""] = 0;
        text_l.decrementer().try_put(tbb::flow::continue_msg());
        return std::make_pair(text_pair.first, mp);
    });

    tbb::flow::limiter_node<std::pair<sys::path, std::map<std::string, int>>> index_l(g, dictionaries_queue_size);

    tbb::flow::function_node<std::pair<sys::path, std::map<std::string, int>>, int> merging_nodes(g, merging_threads, [&](std::pair<sys::path, std::map<std::string, int>> local) -> int {

    #ifdef DEBUGMODE
        write_map("./results/" + local.first.filename().string(), local.second);
    #endif

        for (auto& word : local.second) {
            if (!word.first.empty()) {
                tbb::concurrent_hash_map<std::string, int>::accessor a;
                global.insert(a, word);
//                a->second += 1;
            }
        }
        index_l.decrementer().try_put(tbb::flow::continue_msg());
        return 0;
    });


    // Graph edges
    tbb::flow::make_edge(start_node, path_l);
    tbb::flow::make_edge(path_l, reader_node);
    tbb::flow::make_edge(reader_node, text_l);
    tbb::flow::make_edge(text_l, indexing_nodes);
    tbb::flow::make_edge(indexing_nodes, index_l);
    tbb::flow::make_edge(index_l, merging_nodes);


    start_node.activate();
    g.wait_for_all();

    std::vector<std::pair<std::string, int>> sorted;
    std::vector<std::pair<std::string, int>> sorted_1;
    sorted = sort_by_func_mt(global, 1);
    sorted_1 = sort_by_func_mt(global, 0);



    write_start = get_current_time_fenced();
    write(out_path_a, sorted);
    write(out_path_n, sorted_1);
    write_end = get_current_time_fenced();


    std::cout << "Total=" << to_us(total_end - total_start) << std::endl;
    std::cout << "Reading=" << to_us(read_end - read_start) << std::endl;
    std::cout << "Finding=" << to_us(find_end - find_start) << std::endl;
    std::cout << "Writing=" << to_us(write_end - write_start) << std::endl;
}
