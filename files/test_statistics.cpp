// Статистики и квантили на заранее известных массивах.

#undef NDEBUG
#include <cassert>

#include "statistics.hpp"

#include <cmath>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

namespace
{
    bool nearly_equal(double a, double b, double eps = 1e-12)
    {
        return std::fabs(a - b) < eps;
    }

    void test_quantiles()
    {
        const std::vector<double> values = {1, 2, 3, 4, 5};

        assert(nearly_equal(get_quantile(values, 0.00), 1.0));
        assert(nearly_equal(get_quantile(values, 0.05), 1.2));
        assert(nearly_equal(get_quantile(values, 0.25), 2.0));
        assert(nearly_equal(get_quantile(values, 0.50), 3.0));
        assert(nearly_equal(get_quantile(values, 0.75), 4.0));
        assert(nearly_equal(get_quantile(values, 0.95), 4.8));
        assert(nearly_equal(get_quantile(values, 1.00), 5.0));

        // Чётное число элементов: медиана - среднее двух центральных.
        const std::vector<double> even = {10, 20, 30, 40};
        assert(nearly_equal(get_quantile(even, 0.5), 25.0));

        const std::vector<double> single = {7};
        assert(nearly_equal(get_quantile(single, 0.5), 7.0));

        // Пустой массив.
        bool empty_thrown = false;
        try
        {
            (void)get_quantile(std::vector<double>{}, 0.5);
        }
        catch (const std::invalid_argument &)
        {
            empty_thrown = true;
        }
        assert(empty_thrown);

        // Вероятность вне [0, 1].
        bool above_thrown = false;
        try
        {
            (void)get_quantile(values, 1.5);
        }
        catch (const std::invalid_argument &)
        {
            above_thrown = true;
        }
        assert(above_thrown);

        bool below_thrown = false;
        try
        {
            (void)get_quantile(values, -0.1);
        }
        catch (const std::invalid_argument &)
        {
            below_thrown = true;
        }
        assert(below_thrown);
    }

    void test_numeric_statistics()
    {
        NumericColumn column;
        column.values = {2.0, 4.0, 4.0, 4.0, 5.0, 5.0, 7.0, 9.0, std::nullopt};

        const NumericStatistics stats = get_numeric_statistics(column, 4);

        assert(stats.count == 8);
        assert(stats.missing_count == 1); // пропуск не участвует в расчётах

        assert(nearly_equal(stats.min.value(), 2.0));
        assert(nearly_equal(stats.max.value(), 9.0));
        assert(nearly_equal(stats.mean.value(), 5.0));
        assert(nearly_equal(stats.var.value(), 4.0)); // дисперсия с делением на n

        assert(nearly_equal(stats.q05.value(), 2.7));
        assert(nearly_equal(stats.q25.value(), 4.0));
        assert(nearly_equal(stats.q50.value(), 4.5));
        assert(nearly_equal(stats.q75.value(), 5.5));
        assert(nearly_equal(stats.q95.value(), 8.3));

        assert(stats.histogram.size() == 4);
        std::size_t total = 0;
        for (const auto &bin : stats.histogram)
            total += bin.number;
        assert(total == stats.count);
    }

    void test_all_missing()
    {
        NumericColumn column;
        column.values = {std::nullopt, std::nullopt};

        const NumericStatistics stats = get_numeric_statistics(column);
        assert(stats.count == 0);
        assert(stats.missing_count == 2);
        assert(!stats.mean.has_value());
        assert(!stats.q50.has_value());
    }

    void test_categorical_statistics()
    {
        CategoricalColumn column;
        column.values = {std::string("b"), std::string("a"), std::string("b"),
                         std::nullopt, std::string("c"), std::string("b")};

        const CategoricalStatistics stats = get_categorical_statistics(column);

        assert(stats.count == 5);
        assert(stats.missing_count == 1);
        assert(stats.categories == (std::vector<std::string>{"a", "b", "c"}));

        assert(stats.histogram.size() == 3);
        for (const auto &bin : stats.histogram)
            if (bin.category == "b")
                assert(bin.number == 3);
    }

    void test_dataset_info()
    {
        Dataset dataset;
        dataset.row_count = 2;
        dataset.columns.push_back({"x", NumericColumn{{1.0, 2.0}}});
        dataset.columns.push_back({"c", CategoricalColumn{{std::string("u"), std::string("v")}}});

        const DatasetInfo info = get_dataset_info(dataset);

        assert(info.features.size() == 2);
        assert(info.features[0].name == "x");
        assert(std::holds_alternative<NumericStatistics>(info.features[0].statistics));
        assert(std::holds_alternative<CategoricalStatistics>(info.features[1].statistics));
    }
} // namespace

int main()
{
    test_quantiles();
    test_numeric_statistics();
    test_all_missing();
    test_categorical_statistics();
    test_dataset_info();
    std::cout << "test_statistics: OK\n";
    return 0;
}
