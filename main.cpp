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

    // typedefs
    typedef std::pair<std::vector<std::pair<std::string, int>>, std::vector<std::pair<std::string, int>>> sorted_v_type;



    // Graph nodes
    tbb::flow::function_node<sys::path, std::pair<sys::path, std::string>> reader_node(g, 1, [](const sys::path& filename) -> std::pair<sys::path, std::string> {
        try{
            auto str = read_binary_file(filename);
            return std::make_pair(filename, str);
        }
        catch (...) {
            std::cout << "Cannot read file " << filename << std::endl;
            return std::make_pair(filename, "");
        }
    });

    tbb::flow::function_node<std::pair<sys::path, std::string>, std::map<std::string, int>> indexing_nodes(g, index_threads, [loc](const std::pair<sys::path, std::string>& text_pair) -> std::map<std::string, int> {

        if (text_pair.first.extension().string() == ".txt") {
            return split(&text_pair.second, loc);
        } else if (text_pair.first.extension().string() == ".zip") {
            auto data = extract_archive_files(text_pair.second);
            return split(&data, loc);
        }
    });

    tbb::flow::function_node<std::map<std::string, int>, std::map<std::string, int>> merging_nodes(g, index_threads, [](std::map<std::string, int> dictionaries) -> std::map<std::string, int> {

    });

    tbb::flow::function_node<std::map<std::string, int>, sorted_v_type> sort_node(g, 1, [](std::map<std::string, int> answer) -> sorted_v_type {

    });

    // Graph edges
    tbb::flow::make_edge(reader_node, indexing_nodes);
    tbb::flow::make_edge(indexing_nodes, merging_nodes);
    tbb::flow::make_edge(merging_nodes, sort_node);


    // Run graph with first func
    for (const auto & file : sys::recursive_directory_iterator(indir)) {
        if ((file.path().extension().string() == ".txt" || file.path().extension().string() == ".zip") && (sys::file_size(file.path()) > 1) && (sys::file_size(file.path()) < max_file_size)){ //< 100kb - skip
            reader_node.try_put(file.path().string());
        }
    }
    g.wait_for_all();
    // Save vector into files
}
