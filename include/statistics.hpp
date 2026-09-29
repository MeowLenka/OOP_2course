#ifndef STATISTICS_HPP
#define STATISTICS_HPP

#include "dataset.hpp"

#include <cstddef>
#include <optional>


struct HistogramBin
{
    double lower_bound = 0.0;
    double upper_bound = 0.0;
    std::size_t number = 0;
};


struct ColumnStatistics
{
    std::size_t count = 0;
    std::size_t missing_count = 0;

    std::optional<double> min;
    std::optional<double> max;
    std::optional<double> mean;
    std::optional<double> var;

    std::optional<double> q05;
    std::optional<double> q25;
    std::optional<double> q50;
    std::optional<double> q75;
    std::optional<double> q95;

    std::vector<HistogramBin> histogram;
};

ColumnStatistics get_statistics(const NumericColumn& column, std::size_t bin_count = 5);
double get_quantile(const std::vector<double> &values, double probability);
std::vector<HistogramBin> get_histogram(const std::vector<double>& values, std::size_t bin_count = 5);

#endif
