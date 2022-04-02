#ifndef QUEUE_PARSER_H
#define QUEUE_PARSER_H
#include <boost/program_options.hpp>

class config_options_t
{
public:
    config_options_t();
    config_options_t(char *argv[]);

    config_options_t(const config_options_t &) = default;
    config_options_t &operator=(const config_options_t &) = delete;
    config_options_t(config_options_t &&) = default;
    config_options_t &operator=(config_options_t &&) = delete;
    ~config_options_t() = default;

    void parse(char *argv[]);

    boost::program_options::variables_map vm;
    boost::program_options::options_description config;
    std::filesystem::path indir;
    std::filesystem::path out_by_a;
    std::filesystem::path out_by_n;
    int indexing_threads;
};

#endif // QUEUE_PARSER_H
