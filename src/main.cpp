#include "csv_loader.hpp"
#include "statistics.hpp"

#include <iostream>
#include <iomanip>
#include <variant>

void print_numeric_stats(const NumericStatistics &stats, std::size_t max_bar_length = 30)
{
    std::cout << "(числовой)\n";
    std::cout << "всего значений: " << stats.count << '\n';
    std::cout << "пропусков: " << stats.missing_count << '\n';

    std::cout << std::fixed << std::setprecision(2);

    if (stats.min.has_value())
        std::cout << "min: " << stats.min.value() << '\n';

    if (stats.max.has_value())
        std::cout << "max: " << stats.max.value() << '\n';

    if (stats.mean.has_value())
        std::cout << "mean: " << stats.mean.value() << '\n';

    if (stats.var.has_value())
        std::cout << "var: " << stats.var.value() << '\n';

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

    const std::vector<NumericHistogramBin> &histogram = stats.histogram;

    if (histogram.empty())
    {
        std::cout << "гистограмма пуста.\n";
        return;
    }
    std::size_t max_number = 0;

    for (const NumericHistogramBin &bin : histogram)
        max_number = std::max(max_number, bin.number);

    for (const NumericHistogramBin &bin : histogram)
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

void print_categorical_stats(const CategoricalStatistics &stats, std::size_t max_bar_length = 30)
{
    std::cout << "(категориальный)\n";
    std::cout << "всего значений: " << stats.count << '\n';
    std::cout << "пропусков: " << stats.missing_count << '\n';
    std::cout << "список категорий:\n";

    std::size_t max_cat_num = 10;
    for (const std::string &category : stats.categories)
    {
        std::cout << "  -> " << category << '\n';
        if (--max_cat_num == 0)
            break;
    }
    std::cout << '\n';

    const std::vector<CategoricalHistogramBin> &histogram = stats.histogram;
    if (histogram.empty())
    {
        std::cout << "гистограмма пуста.\n";
        return;
    }
    std::size_t max_number = 0;

    for (const CategoricalHistogramBin &bin : histogram)
        max_number = std::max(max_number, bin.number);

    for (const CategoricalHistogramBin &bin : histogram)
    {
        std::size_t bar_length = 0;
        if (max_number > 0)
        {
            bar_length = static_cast<std::size_t>(ceil(static_cast<double>(bin.number) * max_bar_length / max_number));
        }
        std::string bar(bar_length, '#');
        std::cout << "  " << std::left << std::setw(25) << bin.category
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
        if (std::holds_alternative<NumericColumn>(column.data))
        {
            std::cout << "\n - " << column.name ;
            const NumericColumn &numeric_column = std::get<NumericColumn>(column.data);
            const NumericStatistics &numeric_stats = get_numeric_statistics(numeric_column);
            print_numeric_stats(numeric_stats);
        }
        else
        {
            if (column.name == "\"\"")
                continue;
            else
            {
                std::cout << "\n - " << column.name ;
                const CategoricalColumn &categorical_column = std::get<CategoricalColumn>(column.data);
                const CategoricalStatistics &categorical_stats = get_categorical_statistics(categorical_column);
                print_categorical_stats(categorical_stats);
            }
        }
    }
    return 0;
}
