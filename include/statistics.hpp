#ifndef STATISTICS_HPP
#define STATISTICS_HPP

#include "dataset.hpp"

#include <cstddef>
#include <optional>

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
};

ColumnStatistics get_statistics(const NumericColumn &column);
double get_quantile(const std::vector<double> &values, double probability);

#endif
