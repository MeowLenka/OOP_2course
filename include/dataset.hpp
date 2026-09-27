#ifndef DATASET_HPP
#define DATASET_HPP

#include <string>
#include <vector>

struct NumericColumn
{
    std::vector<std::optional<double>> values;
};

struct CategoricalColumn
{
    std::vector<std::optional<std::string>> values;
};

using ColumnData =
    std::variant<NumericColumn, CategoricalColumn>;

struct Column
{
    std::string name;
    ColumnData data;
};

struct Dataset
{
    std::vector<Column> columns;
    std::size_t row_count = 0;
};

#endif