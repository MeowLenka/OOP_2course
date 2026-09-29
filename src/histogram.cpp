#include "statistics.hpp"

#include <algorithm>
#include <stdexcept>
#include <vector>

std::vector<HistogramBin> get_histogram(const std::vector<double> &values, std::size_t bin_count)
{
    if (bin_count == 0)
        throw std::invalid_argument("Количество интервалов должно быть больше 0");

    std::vector<HistogramBin> histogram;

    if (values.empty())
        return histogram;
    auto min_max = std::minmax_element(values.begin(), values.end());

    double min_value = *min_max.first;
    double max_value = *min_max.second;

    if (min_value == max_value)
    {
        for (std::size_t i = 0; i < bin_count; ++i)
        {
            HistogramBin bin;

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
        HistogramBin bin;
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
