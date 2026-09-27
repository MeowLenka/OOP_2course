#include "csv_loader.hpp"
#include "statistics.hpp"

#include <iostream>
#include <iomanip>
#include <variant>

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

            if (stats.median.has_value())
                std::cout << "медиана: " << stats.median.value() << '\n';
        }
    }

    return 0;
}