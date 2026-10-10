#include "statistics.hpp"

#include <map>
#include <string>
#include <vector>

std::vector<NumericHistogramBin> get_numeric_histogram(std::span<const double> values, std::size_t bin_count)
{
    if (bin_count == 0)
        throw std::invalid_argument("Количество интервалов должно быть больше 0");

    std::vector<NumericHistogramBin> histogram;

    if (values.empty())
        return histogram;
    auto min_max = std::minmax_element(values.begin(), values.end());

    double min_value = *min_max.first;
    double max_value = *min_max.second;

    if (min_value == max_value)
    {
        for (std::size_t i = 0; i < bin_count; ++i)
        {
            NumericHistogramBin bin;

            bin.lower_bound = min_value;
            bin.upper_bound = max_value;
            bin.number = (i == 0) ? values.size() : 0;

            histogram.push_back(bin);
        }
        return histogram;
    }

    double width = (max_value - min_value) / bin_count;

    for (std::size_t i = 0; i < bin_count; ++i)
    {
        NumericHistogramBin bin;
        bin.lower_bound = min_value + i * width;
        bin.upper_bound = min_value + (i + 1) * width;
        bin.number = 0;
        histogram.push_back(bin);
    }

    for (double value : values)
    {
        std::size_t bin_index = static_cast<std::size_t>((value - min_value) / width);
        if (bin_index >= bin_count)
            bin_index = bin_count - 1;
        ++histogram[bin_index].number;
    }

    return histogram;
}

std::vector<CategoricalHistogramBin> get_categorical_histogram(const std::vector<std::string> &values)
{
    std::map<std::string, std::size_t> numbers;
    for (const std::string &value : values)
        ++numbers[value];

    std::vector<CategoricalHistogramBin> histogram;

    for (const auto &[category, num] : numbers)
    {
        CategoricalHistogramBin bin;
        bin.category = category;
        bin.number = num;
        histogram.push_back(bin);
    }
    return histogram;
}
