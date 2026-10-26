// Заполнение пропусков, масштабирование и кодирование категорий.

#undef NDEBUG
#include <cassert>

#include "preprocessing.hpp"

#include <cmath>
#include <iostream>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace
{
    bool nearly_equal(double a, double b, double eps = 1e-12)
    {
        return std::fabs(a - b) < eps;
    }

    // Технический столбец "" + категориальный "color" (с пропуском) + числовой "x" (с пропуском).
    Dataset make_dataset()
    {
        Dataset dataset;
        dataset.row_count = 4;
        dataset.columns.push_back({"", NumericColumn{{1.0, 2.0, 3.0, 4.0}}});
        dataset.columns.push_back(
            {"color", CategoricalColumn{{std::string("b"), std::string("a"),
                                         std::nullopt, std::string("b")}}});
        dataset.columns.push_back({"x", NumericColumn{{0.0, 10.0, std::nullopt, 50.0}}});
        return dataset;
    }

    // Статистика x по исходным данным {0, 10, 50}:
    // mean = 20, median = 10, var = 1400/3, q25 = 5, q75 = 30, min = 0, max = 50.

    void test_one_hot()
    {
        const Dataset dataset = make_dataset();
        const DatasetInfo info = get_dataset_info(dataset);

        const TransformedDataset result = preprocess_dataset(
            dataset, info, NumericRefill::Median, Scaling::MinMax, CategoricalEncoding::OneHot);

        // Технический столбец пропущен; a, b, __MISSING__ + x.
        const std::vector<std::string> expected_names = {
            "color_a", "color_b", "color___MISSING__", "x"};
        assert(result.feature_names == expected_names);
        assert(result.matrix.rows() == 4);
        assert(result.matrix.columns() == 4);

        const double expected[4][3] = {
            {0, 1, 0}, // b
            {1, 0, 0}, // a
            {0, 0, 1}, // пропуск -> __MISSING__
            {0, 1, 0}, // b
        };
        for (std::size_t r = 0; r < 4; ++r)
            for (std::size_t c = 0; c < 3; ++c)
                assert(nearly_equal(result.matrix(r, c), expected[r][c], 1e-12));

        // x: пропуск заменён медианой 10, затем min-max: (v - 0) / 50.
        assert(nearly_equal(result.matrix(0, 3), 0.0, 1e-12));
        assert(nearly_equal(result.matrix(1, 3), 0.2, 1e-12));
        assert(nearly_equal(result.matrix(2, 3), 0.2, 1e-12));
        assert(nearly_equal(result.matrix(3, 3), 1.0, 1e-12));
    }

    void test_ordinal()
    {
        const Dataset dataset = make_dataset();
        const DatasetInfo info = get_dataset_info(dataset);

        const TransformedDataset result = preprocess_dataset(
            dataset, info, NumericRefill::Median, Scaling::MinMax, CategoricalEncoding::Ordinal);

        assert(result.feature_names == (std::vector<std::string>{"color", "x"}));
        assert(result.matrix.columns() == 2);

        // Порядок категорий детерминирован: a = 0, b = 1, __MISSING__ = 2.
        assert(nearly_equal(result.matrix(0, 0), 1.0, 1e-12));
        assert(nearly_equal(result.matrix(1, 0), 0.0, 1e-12));
        assert(nearly_equal(result.matrix(2, 0), 2.0, 1e-12));
        assert(nearly_equal(result.matrix(3, 0), 1.0, 1e-12));
    }

    void test_scaling_uses_original_statistics()
    {
        const Dataset dataset = make_dataset();
        const DatasetInfo info = get_dataset_info(dataset);
        const std::size_t x = 3; // индекс столбца x в one-hot матрице

        const auto standard = preprocess_dataset(
            dataset, info, NumericRefill::Median, Scaling::Standard, CategoricalEncoding::OneHot);
        const double sigma = std::sqrt(1400.0 / 3.0);
        assert(nearly_equal(standard.matrix(0, x), -20.0 / sigma, 1e-9));
        assert(nearly_equal(standard.matrix(1, x), -10.0 / sigma, 1e-9));
        assert(nearly_equal(standard.matrix(3, x), 30.0 / sigma, 1e-9));

        // Robust: (v - 10) / (30 - 5); заполненное медианой значение даёт 0.
        const auto robust = preprocess_dataset(
            dataset, info, NumericRefill::Median, Scaling::Robust, CategoricalEncoding::OneHot);
        assert(nearly_equal(robust.matrix(0, x), -0.4, 1e-12));
        assert(nearly_equal(robust.matrix(2, x), 0.0, 1e-12));
        assert(nearly_equal(robust.matrix(3, x), 1.6, 1e-12));
    }

    void test_refill_strategies()
    {
        const Dataset original = make_dataset();
        const DatasetInfo info = get_dataset_info(original);

        auto filled_value = [&](NumericRefill strategy)
        {
            Dataset copy = original;
            impute_missing_values(copy, info, strategy);
            return std::get<NumericColumn>(copy.columns[2].data).values[2].value();
        };

        assert(nearly_equal(filled_value(NumericRefill::Median), 10.0, 1e-12));
        assert(nearly_equal(filled_value(NumericRefill::Mean), 20.0, 1e-12));
        assert(nearly_equal(filled_value(NumericRefill::Zero), 0.0, 1e-12));
    }

    void test_original_data_and_info_untouched()
    {
        const Dataset dataset = make_dataset();
        const DatasetInfo info = get_dataset_info(dataset);

        (void)preprocess_dataset(dataset, info, NumericRefill::Median,
                                 Scaling::Standard, CategoricalEncoding::OneHot);

        // Исходные данные по-прежнему содержат пропуски.
        assert(!std::get<CategoricalColumn>(dataset.columns[1].data).values[2].has_value());
        assert(!std::get<NumericColumn>(dataset.columns[2].data).values[2].has_value());

        // В статистике нет служебной категории: исходный набор категорий сохранён.
        const auto &stats = std::get<CategoricalStatistics>(info.features[1].statistics);
        assert(stats.categories == (std::vector<std::string>{"a", "b"}));
        assert(stats.missing_count == 1);
    }
} // namespace

int main()
{
    test_one_hot();
    test_ordinal();
    test_scaling_uses_original_statistics();
    test_refill_strategies();
    test_original_data_and_info_untouched();
    std::cout << "test_preprocessing: OK\n";
    return 0;
}
