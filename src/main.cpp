#include "csv_loader.hpp"

#include <iostream>
#include <variant>

int main()
{
    CsvLoadResult result = load_csv("data/dataset.csv");

    if (!result.success)
    {
        return 1;
    }

    std::cout << "Датасет успешно загружен.\n";
    std::cout << "Количество строк: "
              << result.dataset.row_count << '\n';
    std::cout << "Количество столбцов: "
              << result.dataset.columns.size() << '\n';

    std::cout << "\nСтолбцы:\n";

    for (const Column &column : result.dataset.columns)
    {
        std::cout << "- " << column.name << ": ";

        if (std::holds_alternative<NumericColumn>(column.data))
        {
            std::cout << "числовой";
        }
        else
        {
            std::cout << "категориальный";
        }

        std::cout << '\n';
    }

    return 0;
}