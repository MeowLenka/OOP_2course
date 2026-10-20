#pragma once

#include "dataset.hpp"
#include "statistics.hpp"
#include "matrix.hpp"

#include <string>
#include <vector>

enum class NumericRefill
{
    Median,
    Mean,
    Zero
};

enum class Scaling
{
    Standard,
    MinMax,
    Robust
};

enum class CategoricalEncoding
{
    OneHot,
    Ordinal
};

struct TransformedDataset
{
    Matrix matrix;
    std::vector<std::string> feature_names;
};

void impute_missing_values(Dataset &dataset, const DatasetInfo &information, NumericRefill strategy);

TransformedDataset transform_dataset(const Dataset &dataset,
                                     const DatasetInfo &information, Scaling scaling,
                                     CategoricalEncoding encoding);

TransformedDataset preprocess_dataset(const Dataset &dataset, const DatasetInfo &information,
                                      NumericRefill refill, Scaling scaling,
                                      CategoricalEncoding encoding);