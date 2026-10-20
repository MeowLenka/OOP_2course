// Статистики и квантили на заранее известных массивах.

#include "statistics.hpp"
#include "test_utils.hpp"

#include <numeric>
#include <string>
#include <vector>

namespace
{
    void test_quantiles()
    {
        const std::vector<double> values = {1, 2, 3, 4, 5};

        CHECK_NEAR(get_quantile(values, 0.00), 1.0, 1e-12);
        CHECK_NEAR(get_quantile(values, 0.05), 1.2, 1e-12);
        CHECK_NEAR(get_quantile(values, 0.25), 2.0, 1e-12);
        CHECK_NEAR(get_quantile(values, 0.50), 3.0, 1e-12);
        CHECK_NEAR(get_quantile(values, 0.75), 4.0, 1e-12);
        CHECK_NEAR(get_quantile(values, 0.95), 4.8, 1e-12);
        CHECK_NEAR(get_quantile(values, 1.00), 5.0, 1e-12);

        // Чётное число элементов: медиана - среднее двух центральных.
        const std::vector<double> even = {10, 20, 30, 40};
        CHECK_NEAR(get_quantile(even, 0.5), 25.0, 1e-12);

        const std::vector<double> single = {7};
        CHECK_NEAR(get_quantile(single, 0.5), 7.0, 1e-12);

        CHECK_THROWS(get_quantile(std::vector<double>{}, 0.5));
        CHECK_THROWS(get_quantile(values, 1.5));
        CHECK_THROWS(get_quantile(values, -0.1));
    }

    void test_numeric_statistics()
    {
        NumericColumn column;
        column.values = {2.0, 4.0, 4.0, 4.0, 5.0, 5.0, 7.0, 9.0, std::nullopt};

        const NumericStatistics stats = get_numeric_statistics(column, 4);

        CHECK(stats.count == 8);
        CHECK(stats.missing_count == 1); // пропуск не участвует в расчётах

        CHECK_NEAR(stats.min.value(), 2.0, 1e-12);
        CHECK_NEAR(stats.max.value(), 9.0, 1e-12);
        CHECK_NEAR(stats.mean.value(), 5.0, 1e-12);
        CHECK_NEAR(stats.var.value(), 4.0, 1e-12); // дисперсия с делением на n

        CHECK_NEAR(stats.q05.value(), 2.7, 1e-12);
        CHECK_NEAR(stats.q25.value(), 4.0, 1e-12);
        CHECK_NEAR(stats.q50.value(), 4.5, 1e-12);
        CHECK_NEAR(stats.q75.value(), 5.5, 1e-12);
        CHECK_NEAR(stats.q95.value(), 8.3, 1e-12);

        CHECK(stats.histogram.size() == 4);
        std::size_t total = 0;
        for (const auto &bin : stats.histogram)
            total += bin.number;
        CHECK(total == stats.count);
    }

    void test_all_missing()
    {
        NumericColumn column;
        column.values = {std::nullopt, std::nullopt};

        const NumericStatistics stats = get_numeric_statistics(column);
        CHECK(stats.count == 0);
        CHECK(stats.missing_count == 2);
        CHECK(!stats.mean.has_value());
        CHECK(!stats.q50.has_value());
    }

    void test_categorical_statistics()
    {
        CategoricalColumn column;
        column.values = {std::string("b"), std::string("a"), std::string("b"),
                         std::nullopt, std::string("c"), std::string("b")};

        const CategoricalStatistics stats = get_categorical_statistics(column);

        CHECK(stats.count == 5);
        CHECK(stats.missing_count == 1);
        CHECK(stats.categories == (std::vector<std::string>{"a", "b", "c"}));

        CHECK(stats.histogram.size() == 3);
        for (const auto &bin : stats.histogram)
            if (bin.category == "b")
                CHECK(bin.number == 3);
    }

    void test_dataset_info()
    {
        Dataset dataset;
        dataset.row_count = 2;
        dataset.columns.push_back({"x", NumericColumn{{1.0, 2.0}}});
        dataset.columns.push_back({"c", CategoricalColumn{{std::string("u"), std::string("v")}}});

        const DatasetInfo info = get_dataset_info(dataset);

        CHECK(info.features.size() == 2);
        CHECK(info.features[0].name == "x");
        CHECK(std::holds_alternative<NumericStatistics>(info.features[0].statistics));
        CHECK(std::holds_alternative<CategoricalStatistics>(info.features[1].statistics));
    }
} // namespace

int main()
{
    run_test("test_quantiles", test_quantiles);
    run_test("test_numeric_statistics", test_numeric_statistics);
    run_test("test_all_missing", test_all_missing);
    run_test("test_categorical_statistics", test_categorical_statistics);
    run_test("test_dataset_info", test_dataset_info);
    return finish_tests("test_statistics");
}
