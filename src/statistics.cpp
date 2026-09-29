#include "statistics.hpp"

#include <algorithm>
#include <numeric>

double get_quantile(const std::vector<double> &values, double probability)
{
    if (values.empty())
    {
        throw std::invalid_argument("Невозможно вычислить квантиль для пустого массива");
    }

    if (probability > 1.0 || probability < 0.0)
    {
        throw std::invalid_argument("Вероятность квантиля должна быть от 0 до 1");
    }

    double position = probability * (values.size() - 1);
    std::size_t left_index = static_cast<std::size_t>(std::floor(position));
    std::size_t right_index = static_cast<std::size_t>(std::ceil(position));
    double fraction = position - left_index;

    return values[left_index] + fraction * (values[right_index] - values[left_index]);
}

ColumnStatistics get_statistics(const NumericColumn &column, std::size_t bin_count)
{
    ColumnStatistics stats;
    std::vector<double> values;
    for (const auto &value : column.values)
    {
        if (value.has_value())
            values.push_back(value.value());
        else
            ++stats.missing_count;
    }

    stats.count = values.size();
    if (values.empty())
        return stats;

    auto min_max = std::minmax_element(values.begin(), values.end());
    stats.min = *min_max.first;
    stats.max = *min_max.second;

    double sum = std::accumulate(values.begin(), values.end(), 0.0);
    stats.mean = sum / values.size();

    std::sort(values.begin(), values.end());
    stats.q05 = get_quantile(values, 0.05);
    stats.q25 = get_quantile(values, 0.25);
    stats.q50 = get_quantile(values, 0.50);
    stats.q75 = get_quantile(values, 0.75);
    stats.q95 = get_quantile(values, 0.95);

    double sum_sq_dev = 0.0;
    for (double value : values)
    {
        double difference = value - stats.mean.value();
        sum_sq_dev += difference * difference;
    }
    stats.var = sum_sq_dev / values.size();

    stats.histogram = get_histogram(values, bin_count);
    return stats;
}