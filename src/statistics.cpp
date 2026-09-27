#include "statistics.hpp"

#include <algorithm>
#include <numeric>

ColumnStatistics get_statistics(
    const NumericColumn &column)
{
    ColumnStatistics stats;

    std::vector<double> values;

    for (const auto &value : column.values)
    {
        if (value.has_value())
        {
            values.push_back(value.value());
        }
        else
        {
            ++stats.missing_count;
        }
    }

    stats.count = values.size();
    if (values.empty())
    {
        return stats;
    }

    auto min_max = std::minmax_element(values.begin(), values.end());

    stats.min = *min_max.first;
    stats.max = *min_max.second;

    double sum = std::accumulate(values.begin(), values.end(), 0.0);

    stats.mean = sum / values.size();

    std::sort(values.begin(), values.end());
    const std::size_t n = values.size();

    if (n % 2 == 1) // медиана
        stats.q50 = values[n / 2];
    else
        stats.q50 = (values[n / 2 - 1] + values[n / 2]) / 2.0;
    return stats;
}