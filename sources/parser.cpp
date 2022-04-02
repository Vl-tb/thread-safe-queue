#include <boost/program_options.hpp>
#include <iostream>
#include <fstream>
#include <iterator>
#include "../includes/errors.h"
#include "../includes/parser.h"

namespace po = boost::program_options;
namespace sys = std::filesystem;

config_options_t::config_options_t() {
    config.add_options()
            ("indir", po::value<sys::path>()->default_value(""),
             "indir")
            ("out_by_a", po::value<sys::path>()->default_value(""),
             "out_by_a")
            ("out_by_n", po::value<sys::path>()->default_value(""),
             "out_by_n")
            ("indexing_threads", po::value<int>()->default_value(2),
             "indexing_threads");
}

config_options_t::config_options_t(char *argv[]):
        config_options_t()
{
    parse(argv);
}

void config_options_t::parse(char *argv[]) {
    std::string config_file = argv[1];
    std::ifstream file(config_file.c_str());
    if (!file)
    {
        std::cerr << "Не вдалося відкрити конфігураційний файл!" << std::endl;
        exit(CFG_FILE_OPEN_ERROR);
    }
    try {
        po::options_description config_file_options;
        config_file_options.add(config);
        store(parse_config_file(file, config_file_options), vm);
        notify(vm);
        indir = vm["indir"].as<sys::path>();
        out_by_a = vm["out_by_a"].as<sys::path>();
        out_by_n = vm["out_by_n"].as<sys::path>();
        indexing_threads = vm["indexing_threads"].as<int>();

    }
    catch (std::exception &e) {
        {
            std::cerr << e.what() << std::endl;
            exit(CFG_FILE_READ_ERROR);
        }
    }
}
