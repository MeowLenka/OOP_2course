#include "preprocessing.hpp"

#include <cmath>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>

namespace
{
    const FeatureInfo &find_feature_information(const DatasetInfo &information,
                                                const std::string &feature_name)
    {
        for (const FeatureInfo &feature : information.features)
        {
            if (feature.name == feature_name)
                return feature;
        }
        throw std::runtime_error("Не найдена статистика для признака: " + feature_name);
    }

    bool is_technical_column(const Column &column)
    {
        return column.name == "";
    }

    std::vector<std::string> get_categories_for_encoding(const CategoricalStatistics &statistics)
    {
        std::vector<std::string> categories = statistics.categories;

        if (statistics.missing_count > 0)
        {
            categories.push_back("__MISSING__");
        }

        return categories;
    }

    std::size_t ordinal_code(const std::string &value,
                             const std::vector<std::string> &categories)
    {
        for (std::size_t i = 0; i < categories.size(); ++i)
            if (categories[i] == value)
                return i;
        throw std::runtime_error("Категория не найдена при ordinal-кодировании: " + value);
    }

    double scale_standard(double value, const NumericStatistics &statistics)
    {
        if (!statistics.mean.has_value() || !statistics.var.has_value())
        {
            throw std::runtime_error("Недостаточно статистики для стандартизации признака");
        }

        const double mean = statistics.mean.value();
        const double var = statistics.var.value();

        if (var == 0.0)
            return 0.0;

        const double standard_deviation = std::sqrt(var);

        return (value - mean) / standard_deviation;
    }

    double scale_minmax(double value, const NumericStatistics &statistics)
    {
        if (!statistics.min.has_value() || !statistics.max.has_value())
            throw std::runtime_error("Недостаточно статистики для min-max масштабирования");

        const double min = statistics.min.value();
        const double max = statistics.max.value();

        if (max == min)
            return 0.0;

        return (value - min) / (max - min);
    }

    double scale_robust(double value, const NumericStatistics &statistics)
    {
        if (!statistics.q50.has_value() || !statistics.q25.has_value() || !statistics.q75.has_value())
            throw std::runtime_error("Недостаточно статистики для robust масштабирования");

        const double median = statistics.q50.value();
        const double iqr = statistics.q75.value() - statistics.q25.value();

        if (iqr == 0.0)
            return 0.0;

        return (value - median) / iqr;
    }

    double apply_scaling(double value, const NumericStatistics &statistics, Scaling scaling)
    {
        switch (scaling)
        {
        case Scaling::Standard:
            return scale_standard(value, statistics);
        case Scaling::MinMax:
            return scale_minmax(value, statistics);
        case Scaling::Robust:
            return scale_robust(value, statistics);
        }
        throw std::invalid_argument("Неизвестный способ масштабирования");
    }

} // namespace

void impute_missing_values(Dataset &dataset, const DatasetInfo &information,
                           NumericRefill strategy)
{
    for (Column &column : dataset.columns)
    {
        if (is_technical_column(column))
            continue;

        const FeatureInfo &feature_information =
            find_feature_information(information, column.name);

        std::visit(
            [&](auto &column_data)
            {
                using ColumnType = std::decay_t<decltype(column_data)>;

                if constexpr (std::is_same_v<ColumnType, NumericColumn>)
                {
                    const auto *numeric_statistics =
                        std::get_if<NumericStatistics>(&feature_information.statistics);

                    if (numeric_statistics == nullptr)
                        throw std::runtime_error("Тип статистики не соответствует числовому признаку: " + column.name);

                    double replacement = 0.0;

                    switch (strategy)
                    {
                    case NumericRefill::Median:
                        if (!numeric_statistics->q50.has_value())
                            throw std::runtime_error(
                                "Нет медианы для заполнения признака: " + column.name);
                        replacement = numeric_statistics->q50.value();
                        break;
                    case NumericRefill::Mean:
                        if (!numeric_statistics->mean.has_value())
                            throw std::runtime_error(
                                "Нет среднего для заполнения признака: " + column.name);
                        replacement = numeric_statistics->mean.value();
                        break;
                    case NumericRefill::Zero:
                        replacement = 0.0;
                        break;
                    }

                    for (std::optional<double> &value : column_data.values)
                        if (!value.has_value())
                            value = replacement;
                }
                else
                {
                    for (std::optional<std::string> &value : column_data.values)
                        if (!value.has_value())
                            value = "__MISSING__";
                }
            },
            column.data);
    }
}

TransformedDataset transform_dataset(const Dataset &dataset,
                                     const DatasetInfo &information, Scaling scaling,
                                     CategoricalEncoding encoding)
{
    std::size_t output_columns = 0;

    for (const Column &column : dataset.columns)
    {
        if (is_technical_column(column))
            continue;

        const FeatureInfo &feature_information =
            find_feature_information(information, column.name);

        if (std::holds_alternative<NumericColumn>(column.data))
        {
            output_columns += 1;
        }
        else
        {
            const auto *categorical_statistics =
                std::get_if<CategoricalStatistics>(&feature_information.statistics);

            if (categorical_statistics == nullptr)
                throw std::runtime_error(
                    "Тип статистики не соответствует категориальному признаку: " + column.name);

            const auto categories = get_categories_for_encoding(*categorical_statistics);

            if (encoding == CategoricalEncoding::OneHot)
                output_columns += categories.size();
            else
                output_columns += 1; // ordinal
        }
    }

    Matrix matrix(dataset.row_count, output_columns, Layout::RowMajor);

    std::vector<std::string> feature_names;
    feature_names.reserve(output_columns);
    std::size_t output_column = 0;

    for (const Column &column : dataset.columns)
    {
        if (is_technical_column(column))
            continue;

        const FeatureInfo &feature_information =
            find_feature_information(information, column.name);

        if (const auto *numeric_column = std::get_if<NumericColumn>(&column.data))
        {
            const auto *numeric_statistics =
                std::get_if<NumericStatistics>(&feature_information.statistics);

            if (numeric_statistics == nullptr)
                throw std::runtime_error(
                    "Тип статистики не соответствует числовому признаку: " + column.name);

            feature_names.push_back(column.name);

            for (std::size_t row = 0; row < dataset.row_count; ++row)
            {
                if (!numeric_column->values[row].has_value())
                    throw std::runtime_error(
                        "После заполнения пропусков найден пустой числовой элемент: " + column.name);

                matrix(row, output_column) =
                    apply_scaling(numeric_column->values[row].value(), *numeric_statistics, scaling);
            }
            ++output_column;
            continue;
        }

        const auto *categorical_column = std::get_if<CategoricalColumn>(&column.data);
        const auto *categorical_statistics =
            std::get_if<CategoricalStatistics>(&feature_information.statistics);

        if (categorical_column == nullptr || categorical_statistics == nullptr)
            throw std::runtime_error(
                "Тип данных или статистики не соответствует категориальному признаку: " + column.name);

        const std::vector<std::string> categories =
            get_categories_for_encoding(*categorical_statistics);

        if (encoding == CategoricalEncoding::OneHot)
        {
            for (const std::string &category : categories)
            {
                feature_names.push_back(column.name + "_" + category);

                for (std::size_t row = 0; row < dataset.row_count; ++row)
                {
                    if (!categorical_column->values[row].has_value())
                        throw std::runtime_error(
                            "Пустой категориальный элемент после заполнения: " + column.name);

                    matrix(row, output_column) =
                        categorical_column->values[row].value() == category ? 1.0 : 0.0;
                }
                ++output_column;
            }
        }
        else // ordinal
        {
            feature_names.push_back(column.name);

            for (std::size_t row = 0; row < dataset.row_count; ++row)
            {
                if (!categorical_column->values[row].has_value())
                    throw std::runtime_error(
                        "Пустой категориальный элемент после заполнения: " + column.name);

                const std::size_t code =
                    ordinal_code(categorical_column->values[row].value(), categories);

                matrix(row, output_column) = static_cast<double>(code);
            }
            ++output_column;
        }
    }

    return TransformedDataset{std::move(matrix), std::move(feature_names)};
}

TransformedDataset preprocess_dataset(const Dataset &dataset, const DatasetInfo &information,
                                      NumericRefill refill, Scaling scaling,
                                      CategoricalEncoding encoding) 
{
    Dataset preprocessed_dataset = dataset;

    impute_missing_values(preprocessed_dataset, information, refill);

    return transform_dataset(preprocessed_dataset, information, scaling, encoding);
}