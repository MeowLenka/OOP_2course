#include "csv_loader.hpp"
#include "dataset.hpp"
#include "matrix.hpp"
#include "preprocessing.hpp"
#include "statistics.hpp"
#include "cxxopts.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cmath>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>
#include <variant>

NumericRefill parse_refill(const std::string &s)
{
    if (s == "median")
        return NumericRefill::Median;
    if (s == "mean")
        return NumericRefill::Mean;
    if (s == "zero")
        return NumericRefill::Zero;
    throw std::invalid_argument(
        "Неизвестная стратегия заполнения: " + s + " Допустимо: median, mean, zero");
}

Scaling parse_scaling(const std::string &s)
{
    if (s == "standard")
        return Scaling::Standard;
    if (s == "minmax")
        return Scaling::MinMax;
    if (s == "robust")
        return Scaling::Robust;
    throw std::invalid_argument(
        "Неизвестный способ масштабирования: " + s + " Допустимо: standard, minmax, robust");
}

CategoricalEncoding parse_encoding(const std::string &s)
{
    if (s == "onehot")
        return CategoricalEncoding::OneHot;
    if (s == "ordinal")
        return CategoricalEncoding::Ordinal;
    throw std::invalid_argument(
        "Неизвестный способ кодирования: " + s + " Допустимо: onehot, ordinal");
}

std::optional<int> input_int(const std::string &message)
{
    std::cout << message;
    int value = 0;
    std::cin >> value;
    if (std::cin.peek() == '\n')
        return value;

    if (std::cin.eof())
        return std::nullopt;

    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cout << "введите целое число\n";
    return std::nullopt;
}

std::optional<std::string> input_line(const std::string &message)
{
    std::cout << message;
    std::string value;
    if (std::getline(std::cin >> std::ws, value))
        return value;
    return std::nullopt;
}

void print_dataset_summary(const Dataset &dataset, const DatasetInfo &statistics)
{
    std::cout << "\nИнформация по датасету: \n";
    std::cout << "Объектов: " << dataset.row_count << '\n';
    std::cout << "Признаков: " << dataset.columns.size() << '\n';

    std::cout << std::left << std::setw(26) << "#   Признак"
              << std::setw(19) << "Тип" << "Пропуски\n";

    std::size_t total_missing = 0;
    std::size_t features_with_missing = 0;

    for (std::size_t i = 0; i < dataset.columns.size(); ++i)
    {
        const Column &column = dataset.columns[i];
        std::size_t missing_count = 0;
        std::visit(
            [&](const auto &column_data)
            {
                for (const auto &value : column_data.values)
                    if (!value.has_value())
                        ++missing_count;
            },
            column.data);

        total_missing += missing_count;
        if (missing_count > 0)
            ++features_with_missing;

        std::string miss_percent = std::to_string(missing_count) + " (" +
                                   std::format("{:.1f}", 100.0 * static_cast<double>(missing_count) / dataset.row_count) + "%)";

        const char *type_name = std::holds_alternative<NumericColumn>(column.data)
                                    ? "числовой"
                                    : "категориальный";
        std::cout << std::setw(4) << i + 1 << std::left << std::setw(15)
                  << column.name << std::left << std::setw(22) << type_name
                  << "  " << miss_percent << "\n";
    }

    std::cout << "\nВсего пропусков: " << total_missing
              << " в " << features_with_missing << " признаках\n";
}

void print_numeric_statistics(const NumericStatistics &stats)
{
    std::cout << "Тип: числовой\n";
    std::cout << "Непустых значений: " << stats.count << '\n';
    std::cout << "Пропусков: " << stats.missing_count << "\n\n";

    std::cout << std::fixed << std::setprecision(3);

    if (stats.min.has_value())
        std::cout << "min: " << stats.min.value() << '\n';
    if (stats.max.has_value())
        std::cout << "max: " << stats.max.value() << '\n';
    if (stats.mean.has_value())
        std::cout << "mean: " << stats.mean.value() << '\n';
    if (stats.var.has_value())
        std::cout << "var: " << stats.var.value() << "\n\n";

    if (stats.q05.has_value())
        std::cout << "q05: " << stats.q05.value() << '\n';
    if (stats.q25.has_value())
        std::cout << "q25: " << stats.q25.value() << '\n';
    if (stats.q50.has_value())
        std::cout << "q50: " << stats.q50.value() << '\n';
    if (stats.q75.has_value())
        std::cout << "q75: " << stats.q75.value() << '\n';
    if (stats.q95.has_value())
        std::cout << "q95: " << stats.q95.value() << "\n\n";

    std::size_t max_number = 0;
    const std::size_t max_bar_length = 30;
    for (const NumericHistogramBin &bin : stats.histogram)
        max_number = std::max(max_number, bin.number);

    for (const NumericHistogramBin &bin : stats.histogram)
    {
        std::size_t bar_length = 0;
        if (max_number > 0)
            bar_length = static_cast<std::size_t>(
                std::ceil(static_cast<double>(bin.number) * max_bar_length / max_number));

        std::string bar(bar_length, '#');
        std::string interval = "  [" + std::format("{:.1f}", bin.lower_bound) +
                               ", " + std::format("{:.1f}", bin.upper_bound) + ")";

        std::cout << std::left << std::setw(18) << interval
                  << std::left << std::setw(static_cast<int>(max_bar_length))
                  << bar << "  " << bin.number << '\n';
    }
}

void print_categorical_statistics(const CategoricalStatistics &stats)
{
    std::cout << "Тип: категориальный\n";
    std::cout << "Непустых значений: " << stats.count << '\n';
    std::cout << "Пропусков: " << stats.missing_count << "\n\n";

    std::size_t max_number = 0;
    const std::size_t max_bar_length = 30;
    for (const CategoricalHistogramBin &bin : stats.histogram)
        max_number = std::max(max_number, bin.number);

    for (const CategoricalHistogramBin &bin : stats.histogram)
    {
        std::size_t bar_length = 0;
        if (max_number > 0)
            bar_length = static_cast<std::size_t>(
                std::ceil(static_cast<double>(bin.number) * max_bar_length / max_number));

        std::string bar(bar_length, '#');
        std::cout << std::left << std::setw(25) << bin.category
                  << std::left << std::setw(static_cast<int>(max_bar_length))
                  << bar << "  " << bin.number << '\n';
    }
}

// Возвращает false, если нужно выйти из цикла просмотра
bool print_feature_statistics(const Dataset &dataset, const DatasetInfo &statistics)
{
    const auto selected = input_int(
        "\nВведите номер признака (0 - завершить просмотр): ");

    if (!selected.has_value())
    {
        std::cout << "Некорректный ввод. Введите целое число.\n";
        return true;
    }
    if (selected.value() == 0)
        return false;

    if (selected.value() < 1 ||
        static_cast<std::size_t>(selected.value()) > dataset.columns.size())
    {
        std::cout << "Такого номера признака нет. Допустимо от 1 до "
                  << dataset.columns.size() << ".\n";
        return true; // остаёмся в цикле
    }

    const std::size_t index = static_cast<std::size_t>(selected.value() - 1);
    const FeatureInfo &feature = statistics.features.at(index);

    std::cout << "\nПризнак " << selected.value() << " : " << feature.name << "\n";

    if (std::holds_alternative<NumericStatistics>(feature.statistics))
        print_numeric_statistics(std::get<NumericStatistics>(feature.statistics));
    else
        print_categorical_statistics(std::get<CategoricalStatistics>(feature.statistics));

    return true;
}

bool ask_yes_no(const std::string &message)
{
    while (true)
    {
        std::cout << message;
        std::string line;
        if (!std::getline(std::cin >> std::ws, line))
            return false;

        if (line == "1" || line == "y" || line == "Y" || line == "yes" || line == "да")
            return true;
        if (line == "0" || line == "n" || line == "N" || line == "no" || line == "нет")
            return false;

        std::cout << "Введите 1 (да) или 0 (нет).\n";
    }
}

void run_preprocessing(const Dataset &dataset, const DatasetInfo &statistics,
                       NumericRefill refill, Scaling scaling, CategoricalEncoding encoding)
{
    try
    {
        TransformedDataset result = preprocess_dataset(dataset, statistics, refill, scaling, encoding);

        std::cout << "\nPreprocessing завершён.\n";
        std::cout << "Строк в матрице: " << result.matrix.rows() << '\n';
        std::cout << "Столбцов в матрице: " << result.matrix.columns() << '\n';

        std::cout << "\nИтоговые признаки:\n";
        for (std::size_t i = 0; i < result.feature_names.size(); ++i)
            std::cout << i + 1 << "  " << result.feature_names[i] << '\n';

        if (!ask_yes_no("\nСохранить матрицу в .bin? (1 - да, 0 - нет): "))
        {
            std::cout << "Матрица не сохранена.\n";
            return;
        }

        std::optional<std::string> path;
        while (true)
        {
            path = input_line("Файл для сохранения: ");
            if (!path.has_value())
            {
                std::cout << "Не удалось прочитать путь. Матрица не сохранена.\n";
                return;
            }
            if (!path->empty())
                break;
            std::cout << "Путь не может быть пустым. Попробуйте ещё раз.\n";
        }

        const std::filesystem::path output_path = path.value();
        if (output_path.has_parent_path())
            std::filesystem::create_directories(output_path.parent_path());

        result.matrix.save(output_path);

        const auto csv_size = std::filesystem::file_size(dataset.path);
        const auto bin_size = std::filesystem::file_size(output_path);

        std::cout << "\nМатрица сохранена: " << output_path.string() << '\n';
        std::cout << "CSV:    " << csv_size << " байт\n";
        std::cout << "Binary: " << bin_size << " байт\n";
    }
    catch (const std::exception &error)
    {
        std::cout << "Ошибка preprocessing: " << error.what() << '\n';
    }
}

int main(int argc, char **argv)
{
    try
    {
        cxxopts::Options options("lab1");

        options.add_options()
            ("h,help", "Показать справку")
            ("csv", "Путь к входному CSV-файлу",
                cxxopts::value<std::string>())
            ("o,output", "Путь для сохранения .bin (если задан — preprocessing выполняется автоматически)",
                cxxopts::value<std::string>())
            ("bins", "Число интервалов гистограммы",
                cxxopts::value<std::size_t>()->default_value("7"))
            ("refill", "Стратегия заполнения числовых пропусков: median, mean, zero",
                cxxopts::value<std::string>()->default_value("median"))
            ("scaling", "Масштабирование: standard, minmax, robust",
                cxxopts::value<std::string>()->default_value("standard"))
            ("encoding", "Кодирование категорий: onehot, ordinal",
                cxxopts::value<std::string>()->default_value("onehot"));
        cxxopts::ParseResult args;
        try
        {
            args = options.parse(argc, argv);
        }
        catch (const cxxopts::exceptions::exception &e)
        {
            std::cerr << "Ошибка параметров: " << e.what() << "\n\n";
            std::cerr << options.help() << '\n';
            return 1;
        }

        if (args.count("help"))
        {
            std::cout << options.help() << '\n';
            return 0;
        }

        NumericRefill refill;
        Scaling scaling;
        CategoricalEncoding encoding;
        try
        {
            refill = parse_refill(args["refill"].as<std::string>());
            scaling = parse_scaling(args["scaling"].as<std::string>());
            encoding = parse_encoding(args["encoding"].as<std::string>());
        }
        catch (const std::invalid_argument &e)
        {
            std::cerr << "Ошибка: " << e.what() << '\n';
            return 1;
        }

        const std::size_t bin_count = args["bins"].as<std::size_t>();
        if (bin_count == 0)
        {
            std::cerr << "Ошибка: --bins должно быть больше 0\n";
            return 1;
        }

        std::string csv_path;
        if (args.count("csv"))
        {
            csv_path = args["csv"].as<std::string>();
        }
        else
        {
            while (true)
            {
                const auto path = input_line("Путь к .csv файлу: ");
                if (!path.has_value())
                {
                    std::cout << "Не удалось прочитать путь. Завершение.\n";
                    return 0;
                }
                if (!path->empty())
                {
                    csv_path = path.value();
                    break;
                }
                std::cout << "Путь не может быть пустым. Попробуйте ещё раз.\n";
            }
        }

        CsvLoadResult load_result;
        try
        {
            load_result = load_csv(csv_path);
        }
        catch (const std::exception &error)
        {
            std::cerr << "Ошибка загрузки CSV: " << error.what() << '\n';
            return 1;
        }

        if (!load_result.success)
        {
            std::cerr << "Не удалось загрузить .csv файл\n";
            return 1;
        }

        Dataset dataset = std::move(load_result.dataset);
        DatasetInfo statistics = get_dataset_info(dataset, bin_count);
        print_dataset_summary(dataset, statistics);

        while (true)
        {
            if (!print_feature_statistics(dataset, statistics))
                break;
        }

        if (args.count("output"))
        {
            const std::filesystem::path output_path = args["output"].as<std::string>();

            try
            {
                TransformedDataset result =
                    preprocess_dataset(dataset, statistics, refill, scaling, encoding);

                if (output_path.has_parent_path())
                    std::filesystem::create_directories(output_path.parent_path());

                result.matrix.save(output_path);

                const auto csv_size = std::filesystem::file_size(dataset.path);
                const auto bin_size = std::filesystem::file_size(output_path);

                std::cout << "\nМатрица сохранена: " << output_path.string() << '\n';
                std::cout << "Строк: " << result.matrix.rows()
                          << ", столбцов: " << result.matrix.columns() << '\n';
                std::cout << "CSV:    " << csv_size << " байт\n";
                std::cout << "Binary: " << bin_size << " байт\n";
            }
            catch (const std::exception &error)
            {
                std::cerr << "Ошибка preprocessing: " << error.what() << '\n';
                return 1;
            }
        }
        else
        {
            if (ask_yes_no("\nВыполнить предобработку и сохранить результат? (1 — да, 0 — нет): "))
                run_preprocessing(dataset, statistics, refill, scaling, encoding);
        }
        return 0;
    }
    catch (const std::exception &error) // todo: прчитать про раскрутку стека
    {
        std::cerr << "Непредвиденная ошибка: " << error.what() << '\n';
        return 1;
    }
}