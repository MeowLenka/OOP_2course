#include <iostream>

char separator = ',';
std::string skip = "NA";

struct NumericColumn {
    std::vector<std::optional<double>> values;
};

struct CategoricalColumn {
    std::vector<std::optional<std::string>> values;
};

using Column =
    std::variant<NumericColumn, CategoricalColumn>;

struct Dataset
{
    /* data */
};

