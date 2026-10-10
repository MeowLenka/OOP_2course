#include "matrix.hpp"

#include <cstdint>
#include <fstream>
#include <limits>
#include <stdexcept>

namespace
{
    std::uint8_t layout_to_binary(Layout layout)
    {
        switch (layout)
        {
        case Layout::RowMajor:
            return 0;
        case Layout::ColumnMajor:
            return 1;
        default:
            throw std::invalid_argument("неизвестный тип layout");
        }
    }

    Layout layout_from_binary(std::uint8_t value)
    {
        switch (value)
        {
        case 0:
            return Layout::RowMajor;
        case 1:
            return Layout::ColumnMajor;
        default:
            throw std::runtime_error("Некорректное значение layout в бинарном файле");
        }
    }

    std::uint64_t size_to_binary(std::size_t value)
    {
        if (
            static_cast<std::uint64_t>(value) >
            std::numeric_limits<std::uint64_t>::max())
        {
            throw std::overflow_error(
                "Размер матрицы не помещается "
                "в бинарный формат");
        }

        return static_cast<std::uint64_t>(value);
    }

    std::size_t size_from_binary(
        std::uint64_t value)
    {
        if (
            value > static_cast<std::uint64_t>(
                        std::numeric_limits<std::size_t>::max()))
        {
            throw std::runtime_error(
                "размер матрицы из файла не поддерживается");
        }
        return static_cast<std::size_t>(value);
    }

} // namespace

Matrix::Matrix(std::size_t rows, std::size_t columns, Layout layout)
    : _data(rows * columns, 0.0), _rows(rows), _columns(columns), _layout(layout)
{
}

std::size_t Matrix::index(std::size_t row, std::size_t column) const
{
    if (row >= _rows || column >= _columns)
        throw std::out_of_range("Индекс матрицы находится вне допустимого диапазона");

    if (_layout == Layout::RowMajor)
        return row * _columns + column;
    return column * _rows + row;
}

double &Matrix::operator()(std::size_t row, std::size_t column)
{
    return _data[index(row, column)];
}

const double &Matrix::operator()(std::size_t row, std::size_t column) const
{
    return _data[index(row, column)];
}

std::size_t Matrix::rows() const { return _rows; }

std::size_t Matrix::columns() const { return _columns; }

Layout Matrix::layout() const { return _layout; }

const std::vector<double> &Matrix::data() const { return _data; }

std::vector<double> &Matrix::data() { return _data; }

void Matrix::save(const std::filesystem::path &path) const
{
    std::ofstream output(path, std::ios::binary);

    if (!output)
    {
        throw std::runtime_error(
            "Не удалось открыть бинарный файл для записи: " + path.string());
    }

    const std::uint64_t rows = size_to_binary(_rows);
    const std::uint64_t columns = size_to_binary(_columns);
    const std::uint8_t layout = layout_to_binary(_layout);

    output.write(
        reinterpret_cast<const char *>(&rows),
        sizeof(rows));

    output.write(
        reinterpret_cast<const char *>(&columns),
        sizeof(columns));

    output.write(
        reinterpret_cast<const char *>(&layout),
        sizeof(layout));

    if (!output)
    {
        throw std::runtime_error(
            "Ошибка записи заголовка бинарного файла: " +
            path.string());
    }

    if (!_data.empty())
    {
        output.write(
            reinterpret_cast<const char *>(_data.data()),
            static_cast<std::streamsize>(_data.size() * sizeof(double)));
    }

    if (!output)
    {
        throw std::runtime_error(
            "Ошибка записи данных матрицы: " + path.string());
    }
}

Matrix Matrix::load(const std::filesystem::path &path)
{
    std::ifstream input(path, std::ios::binary);

    if (!input)
    {
        throw std::runtime_error(
            "Не удалось открыть бинарный файл на чтение: " +
            path.string());
    }

    std::uint64_t rows_binary = 0;
    std::uint64_t columns_binary = 0;
    std::uint8_t layout_binary = 0;

    input.read(
        reinterpret_cast<char *>(&rows_binary),
        sizeof(rows_binary));

    input.read(
        reinterpret_cast<char *>(&columns_binary),
        sizeof(columns_binary));

    input.read(
        reinterpret_cast<char *>(&layout_binary),
        sizeof(layout_binary));

    if (!input)
    {
        throw std::runtime_error(
            "Не удалось прочитать заголовок бинарного файла: " +
            path.string());
    }

    const std::size_t rows =
        size_from_binary(rows_binary);

    const std::size_t columns =
        size_from_binary(columns_binary);

    if (columns != 0 && rows > std::numeric_limits<std::size_t>::max() / columns)
    {
        throw std::runtime_error(
            "Размер матрицы из файла вызывает переполнение");
    }

    const Layout layout = layout_from_binary(layout_binary);

    Matrix matrix(rows, columns, layout);

    if (!matrix._data.empty())
    {
        input.read(
            reinterpret_cast<char *>(matrix._data.data()),
            static_cast<std::streamsize>(matrix._data.size() * sizeof(double)));
    }

    if (!input)
    {
        throw std::runtime_error(
            "Не удалось полностью прочитать данные матрицы из файла: " +
            path.string());
    }

    return matrix;
}
