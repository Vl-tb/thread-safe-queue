#include <iostream>
#include <deque>
#include <fstream>
#include <thread>
#include <boost/locale.hpp>
#include <tbb/flow_graph.h>

#include "includes/errors.h"
#include "includes/files.h"

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
    std::locale loc = boost::locale::generator().generate("en_US.UTF-8");

    std::chrono::high_resolution_clock::time_point find_start;
    std::chrono::high_resolution_clock::time_point find_end;
    std::chrono::high_resolution_clock::time_point read_start;
    std::chrono::high_resolution_clock::time_point read_end;
    std::chrono::high_resolution_clock::time_point write_start;
    std::chrono::high_resolution_clock::time_point write_end;
    std::chrono::high_resolution_clock::time_point total_start;
    std::chrono::high_resolution_clock::time_point total_end;


    std::vector<std::map<std::string, int>> global;
    std::vector<sys::path> pathes;

    for (const auto & file : sys::recursive_directory_iterator(indir)) {
        if ((file.path().extension().string() == ".txt" || file.path().extension().string() == ".zip") && (sys::file_size(file.path()) > 1) && (sys::file_size(file.path()) < max_file_size)){ //< 100kb - skip
            pathes.push_back(file.path().string());
        }
    }
    size_t curr_file = 0, num_of_files = pathes.size();

    // typedefs
    typedef std::pair<std::vector<std::pair<std::string, int>>, std::vector<std::pair<std::string, int>>> sorted_v_type;


    // Graph nodes
    tbb::flow::input_node<sys::path> start_node(g, [&](tbb::flow_control& fc) -> sys::path {
        if (curr_file < num_of_files) {
            ++curr_file;
            return pathes[curr_file];
        } else {
            fc.stop();
            return "";
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

    tbb::flow::function_node<std::pair<sys::path, std::string>, std::map<std::string, int>> indexing_nodes(g, index_threads, [&](const std::pair<sys::path, std::string>& text_pair) -> std::map<std::string, int> {

        if (text_pair.first.extension().string() == ".txt") {
            text_l.decrementer().try_put(tbb::flow::continue_msg());
            return split(&text_pair.second, loc);
        } else if (text_pair.first.extension().string() == ".zip") {
            auto data = extract_archive_files(text_pair.second);
            text_l.decrementer().try_put(tbb::flow::continue_msg());
            return split(&data, loc);
        }
        std::map<std::string, int> mp;
        mp[""] = 0;
        text_l.decrementer().try_put(tbb::flow::continue_msg());
        return mp;
    });

    tbb::flow::limiter_node<std::map<std::string, int>> index_l(g, dictionaries_queue_size);

//    tbb::flow::function_node<std::map<std::string, int>, std::map<std::string, int>> merging_nodes(g, merging_threads, [&](std::map<std::string, int> dictionary) -> std::map<std::string, int> {
//        index_l.decrementer().try_put(tbb::flow::continue_msg());
//    });
    tbb::flow::function_node<std::map<std::string, int>, std::vector<std::map<std::string, int>>> merging_nodes(g, 1, [&](std::map<std::string, int> dictionary) -> std::vector<std::map<std::string, int>> {
        global.push_back(dictionary);
        return global;
    });

//    tbb::flow::function_node<std::map<std::string, int>, sorted_v_type> sort_node(g, 1, [](std::map<std::string, int> answer) -> sorted_v_type {

//    });

    // Graph edges
    tbb::flow::make_edge(start_node, path_l);
    tbb::flow::make_edge(path_l, reader_node);
    tbb::flow::make_edge(reader_node, text_l);
    tbb::flow::make_edge(text_l, indexing_nodes);
    tbb::flow::make_edge(indexing_nodes, index_l);
    tbb::flow::make_edge(index_l, merging_nodes);
//    tbb::flow::make_edge(merging_nodes, sort_node);


    start_node.activate();
    g.wait_for_all();

    for(const auto& mp : global)
    {
        for(const auto& elem: mp) {
            std::cout << elem.first << " " << elem.second << std::endl;
        }
    }
    // Save vector into files
}

