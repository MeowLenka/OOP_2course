#ifndef STATISTICS_HPP
#define STATISTICS_HPP

#include "dataset.hpp"

#include <cstddef>
#include <optional>
#include <span>

struct NumericHistogramBin
{
    double lower_bound = 0.0;
    double upper_bound = 0.0;
    std::size_t number = 0;
};

struct CategoricalHistogramBin
{
    std::string category;
    std::size_t number = 0;
};

struct NumericStatistics
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

    std::vector<NumericHistogramBin> histogram;
};

struct CategoricalStatistics
{
    std::size_t count = 0;
    std::size_t missing_count = 0;

    std::vector<std::string> categories;
    std::vector<CategoricalHistogramBin> histogram;
};

using Statistics = std::variant<NumericStatistics, CategoricalStatistics>;

double get_quantile(std::span<const double> values, double probability);

CategoricalStatistics get_categorical_statistics(const CategoricalColumn &column);
NumericStatistics get_numeric_statistics(const NumericColumn &column, std::size_t bin_count = 7);

std::vector<NumericHistogramBin> get_numeric_histogram(std::span<const double> values, std::size_t bin_count = 7);
std::vector<CategoricalHistogramBin> get_categorical_histogram(const std::vector<std::string> &values);

struct FeatureInfo
{
    std::string name;
    Statistics statistics;
};

struct DatasetInfo 
{
    std::vector<FeatureInfo> features;
};

DatasetInfo get_dataset_info(const Dataset &dataset, std::size_t bin_count = 7);

#endif
