#include "matrix.hpp"
#include "test_utils.hpp"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>

namespace
{
    void print_first_rows(const Matrix &matrix, std::size_t max_rows = 5)
    {
        const std::size_t count = std::min(max_rows, matrix.rows());

        std::cout << "Первые " << count << " из " << matrix.rows()
                  << " строк (столбцов: " << matrix.columns() << "):\n"
                  << std::fixed << std::setprecision(4);

        for (std::size_t row = 0; row < count; ++row)
        {
            for (std::size_t column = 0; column < matrix.columns(); ++column)
                std::cout << std::setw(10) << matrix(row, column) << ' ';
            std::cout << '\n';
        }
        std::cout << std::defaultfloat;
    }

    // Размер файла: rows(8) + columns(8) + layout(1) + данные.
    constexpr std::uintmax_t header_size = 2 * sizeof(std::uint64_t) + sizeof(std::uint8_t);

    void check_roundtrip(std::size_t rows, std::size_t columns, Layout layout,
                         const std::filesystem::path &path)
    {
        Matrix original(rows, columns, layout);
        for (std::size_t r = 0; r < rows; ++r)
            for (std::size_t c = 0; c < columns; ++c)
                original(r, c) = static_cast<double>(r) / 3.0 + 0.1 * static_cast<double>(c);

        original.save(path);

        CHECK(std::filesystem::file_size(path) ==
              header_size + rows * columns * sizeof(double));

        const Matrix loaded = Matrix::load(path);

        print_first_rows(loaded);

        CHECK(loaded.rows() == original.rows());
        CHECK(loaded.columns() == original.columns());
        CHECK(loaded.layout() == original.layout());

        // Бинарный формат без потерь: сравниваем точно, без допуска.
        for (std::size_t r = 0; r < rows; ++r)
            for (std::size_t c = 0; c < columns; ++c)
                CHECK(loaded(r, c) == original(r, c));
    }
} // namespace

void run_all()
{
    const auto directory = std::filesystem::temp_directory_path();
    const auto path = directory / "lab1_matrix_roundtrip_test.bin";

    std::cout << "--- RowMajor 7x4 ---\n";
    check_roundtrip(7, 4, Layout::RowMajor, path);

    std::cout << "--- ColumnMajor 4x3 ---\n";
    check_roundtrip(4, 3, Layout::ColumnMajor, path);

    std::cout << "--- пустая матрица 0x0 ---\n";
    check_roundtrip(0, 0, Layout::RowMajor, path);

    // Файл не существует.
    CHECK_THROWS(Matrix::load(directory / "lab1_no_such_file.bin"));

    // Обрезанный файл (потеряны последние 8 байт данных).
    {
        Matrix matrix(2, 2);
        matrix.save(path);
        std::filesystem::resize_file(path, std::filesystem::file_size(path) - sizeof(double));
        CHECK_THROWS(Matrix::load(path));
    }

    // Файл короче заголовка.
    {
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        output << "abc";
    }
    CHECK_THROWS(Matrix::load(path));

    // Выход за границы матрицы.
    {
        Matrix matrix(2, 2);
        CHECK_THROWS(matrix(2, 0));
        CHECK_THROWS(matrix(0, 2));
    }

    std::filesystem::remove(path);
}

int main()
{
    run_test("test_matrix", run_all);
    return finish_tests("test_matrix");
}
