#ifndef MATRIX_HPP
#define MATRIX_HPP

#include <cstddef>
#include <filesystem>
#include <vector>

enum class Layout
{
    RowMajor,
    ColumnMajor
};

class Matrix
{
    std::vector<double> _data;
    std::size_t _rows;
    std::size_t _columns;
    Layout _layout;
    std::size_t index(std::size_t row, std::size_t column) const;

public:
    Matrix(std::size_t rows, std::size_t columns, Layout layout = Layout::RowMajor);

    double &operator()(std::size_t row, std::size_t column);
    const double &operator()(std::size_t row, std::size_t column) const;

    std::size_t rows() const;
    std::size_t columns() const;
    Layout layout() const;
    
    const std::vector<double> &data() const;
    std::vector<double> &data();

    void save(const std::filesystem::path &path) const; 
    static Matrix load(const std::filesystem::path &path);
};

#endif
