#include "matrix.hpp"

#include <cassert>
#include <cmath>
#include <filesystem>

void test_matrix_binary_roundtrip()
{
    const std::filesystem::path path =
        "matrix_roundtrip_test.bin";

    Matrix original(2, 3, Layout::RowMajor);

    original(0, 0) = 1.0;
    original(0, 1) = 2.0;
    original(0, 2) = 3.0;

    original(1, 0) = 4.0;
    original(1, 1) = 5.0;
    original(1, 2) = 6.0;

    original.save(path);

    Matrix loaded =
        Matrix::load(path);

    assert(
        loaded.rows() ==
        original.rows());

    assert(
        loaded.columns() ==
        original.columns());

    assert(
        loaded.layout() ==
        original.layout());

    for (
        std::size_t row = 0;
        row < original.rows();
        ++row)
    {
        for (
            std::size_t column = 0;
            column < original.columns();
            ++column)
        {
            assert(
                std::abs(
                    loaded(row, column) -
                    original(row, column)) < 1e-12);
        }
    }
    std::filesystem::remove(path);
}

int main()
{
    test_matrix_binary_roundtrip();

    return 0;
}

