#include "csv_loader.hpp"
#include "statistics.hpp"

#include <iostream>
#include <iomanip>
#include <variant>

void print_histogram(const std::vector<HistogramBin> &histogram, std::size_t max_bar_length = 30)
{
    if (histogram.empty())
    {
        std::cout << "Гистограмма пуста.\n";
        return;
    }

    std::size_t max_number = 0;

    for (const HistogramBin &bin : histogram)
        max_number = std::max(max_number, bin.number);

    for (const HistogramBin &bin : histogram)
    {
        std::size_t bar_length = 0;
        if (max_number > 0)
        {
            bar_length = static_cast<std::size_t>(ceil(static_cast<double>(bin.number) * max_bar_length / max_number));
        }
        std::string bar(bar_length, '#');
        std::string interval = "  [" + std::format("{:.1f}", bin.lower_bound) + ", " + std::format("{:.1f}", bin.upper_bound) + ")";
        
        std::cout << std::left << std::setw(18) << interval 
        << std::left << std::setw(static_cast<int>(max_bar_length)) << bar << "  " << bin.number << '\n';
    }
}

int main()
{
    CsvLoadResult result = load_csv("data/dataset.csv");

    if (!result.success)
    {
        return 1;
    }

    std::cout << "Датасет успешно загружен.\n";
    std::cout << "Количество строк: " << result.dataset.row_count << '\n';
    std::cout << "Количество столбцов: " << result.dataset.columns.size() << '\n';

    std::cout << "\nСтолбцы:\n";

    for (const Column &column : result.dataset.columns)
    {
        std::cout << "- " << column.name << ": ";

        if (std::holds_alternative<NumericColumn>(column.data))
            std::cout << "числовой";
        else
            std::cout << "категориальный";

        std::cout << '\n';
    }

    for (const Column &column : result.dataset.columns)
    {
        if (std::holds_alternative<NumericColumn>(column.data))
        {
            const NumericColumn &numeric_column = std::get<NumericColumn>(column.data);

            ColumnStatistics stats = get_statistics(numeric_column);

            std::cout << "\n- " << column.name << '\n';
            std::cout << "количество значений: " << stats.count << '\n';
            std::cout << "количество пропусков: " << stats.missing_count << '\n';

            std::cout << std::fixed << std::setprecision(2);

            if (stats.min.has_value())
                std::cout << "минимум: " << stats.min.value() << '\n';

            if (stats.max.has_value())
                std::cout << "максимум: " << stats.max.value() << '\n';

            if (stats.mean.has_value())
                std::cout << "среднее: " << stats.mean.value() << '\n';

            if (stats.q05.has_value())
                std::cout << "q05: " << stats.q05.value() << ' ';
            if (stats.q25.has_value())
                std::cout << "q25: " << stats.q25.value() << ' ';
            if (stats.q50.has_value())
                std::cout << "q50: " << stats.q50.value() << ' ';
            if (stats.q75.has_value())
                std::cout << "q75: " << stats.q75.value() << ' ';
            if (stats.q95.has_value())
                std::cout << "q95: " << stats.q95.value() << '\n';

            if (stats.var.has_value())
                std::cout << "дисперсия: " << stats.var.value() << '\n';

            print_histogram(stats.histogram);
        }
    }

    return 0;
}