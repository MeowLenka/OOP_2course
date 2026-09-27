#ifndef CSV_LOADER_HPP
#define CSV_LOADER_HPP

#include "dataset.hpp"

struct CsvConfig
{
    char separator = ',';
    std::string skip_value = "NA";
    // std::string skip_value2 = "";
    bool has_header = true;
};

struct CsvLoadResult
{
    Dataset dataset;
    bool success = false;
};

CsvLoadResult load_csv(const std::string &path, const CsvConfig &config = {});

#endif