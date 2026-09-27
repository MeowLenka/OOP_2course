#include "csv_loader.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <utility>


static std::vector<std::string> parse_csv_line(const std::string &line, char separator)
{
    std::vector<std::string> fields;
    std::string field;
    bool inside_quotes = false;

    for (std::size_t i = 0; i < line.size(); ++i)
    {
        char ch = line[i];

        if (ch == separator)
        {
            fields.push_back(field);
            field.clear();
        }
        else
        {
            field += ch;
        }
    }

    fields.push_back(field);
    return fields;
}

static bool is_number(const std::string &value)
{
    if (value.empty())
    {
        return false;
    }

    try
    {
        std::size_t pos = 0;
        std::stod(value, &pos);
        return pos == value.size();
    }
    catch (const std::exception &)
    {
        return false;
    }
}

static double parse_number(const std::string &value)
{
    std::size_t pos = 0;
    double result = std::stod(value, &pos);

    if (pos != value.size())
    {
        throw std::invalid_argument(
            "Некорректное числовое значение: " + value);
    }
    return result;
}

CsvLoadResult load_csv(const std::string &path, const CsvConfig &config)
{
    CsvLoadResult result;

    std::ifstream file(path);

    if (!file.is_open())
    {
        throw std::runtime_error("Не удалось открыть файл: " + path);
        return result;
    }

    std::vector<std::vector<std::string>> rows;
    std::string line;

    while (std::getline(file, line))
    {
        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }

        if (line.empty())
        {
            continue;
        }

        rows.push_back(parse_csv_line(line, config.separator));
    }

    if (rows.empty())
    {
        throw std::runtime_error("CSV-файл пуст");
        return result;
    }

    std::vector<std::string> headers;
    std::size_t first_data_row = 0;

    if (config.has_header)
    {
        headers = rows[0];
        first_data_row = 1;
    }
    else
    {
        for (std::size_t i = 0; i < rows[0].size(); ++i)
        {
            headers.push_back("Column_" + std::to_string(i + 1));
        }
    }

    const std::size_t column_count = headers.size();

    if (column_count == 0)
    {
        throw std::runtime_error("CSV-файл пуст");
        return result;
    }

    for (std::size_t i = first_data_row; i < rows.size(); ++i)
    {
        if (rows[i].size() != column_count)
        {
            throw std::runtime_error("Неверное количество полей в строке " + std::to_string(i + 1));
            return result;
        }
    }

    for (std::size_t col = 0; col < column_count; ++col)
    {
        bool numeric = true;
        bool has_value = false;

        for (std::size_t row = first_data_row; row < rows.size(); ++row)
        {
            std::string value = rows[row][col];

            if (value == config.skip_value || value.empty())
            {
                continue;
            }

            has_value = true;

            if (!is_number(value))
            {
                numeric = false;
                break;
            }
        }
        if (!has_value)
        {
            numeric = false;
        }

        Column column;
        column.name = headers[col];

        if (numeric)
        {
            NumericColumn numeric_column;

            for (std::size_t row = first_data_row;
                 row < rows.size(); ++row)
            {
                std::string value = rows[row][col];

                if (value == config.skip_value || value.empty())
                {
                    numeric_column.values.push_back(std::nullopt);
                }
                else
                {
                    numeric_column.values.push_back(parse_number(value));
                }
            }

            column.data = std::move(numeric_column);
        }
        else
        {
            CategoricalColumn categorical_column;

            for (std::size_t row = first_data_row;
                 row < rows.size(); ++row)
            {
                std::string value = rows[row][col];

                if (value == config.skip_value || value.empty())
                {
                    categorical_column.values.push_back(std::nullopt);
                }
                else
                {
                    categorical_column.values.push_back(value);
                }
            }

            column.data = std::move(categorical_column);
        }

        result.dataset.columns.push_back(std::move(column));
    }

    result.dataset.row_count = rows.size() - first_data_row;
    result.success = true;

    return result;
}
