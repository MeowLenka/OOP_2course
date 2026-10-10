#include "csv_loader.hpp"
#include "dataset.hpp"
#include "matrix.hpp"
#include "preprocessing.hpp"
#include "statistics.hpp"

#include <algorithm>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <limits>
#include <type_traits>
#include <variant>

namespace
{
    constexpr const char *DEFAULT_CSV_PATH = "data/dataset.csv";
    constexpr const char *DEFAULT_BINARY_PATH = "data/preprocessed.bin";

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

    void print_menu()
    {
        std::cout << "\n";
        std::cout << "1. Загрузить CSV\n";
        std::cout << "2. Показать общую информацию\n";
        std::cout << "3. Посмотреть статистику признака\n";
        std::cout << "4. Выполнить preprocessing и сохранить\n";
        std::cout << "5. Загрузить бинарную матрицу\n";
        std::cout << "0. Выход\n";
    }

    void print_dataset_summary(const Dataset &dataset, const DatasetInfo &statistics)
    {
        std::cout << "\nИнформация по датасету: \n";
        std::cout << "Объектов: " << dataset.row_count << '\n';
        std::cout << "Признаков: " << dataset.columns.size() << '\n';

        std::cout << std::left << std::setw(26) << "#   Признак" << std::setw(19) << "Тип" << "Пропуски\n";
        for (std::size_t i = 0; i < dataset.columns.size(); ++i)
        {
            const Column &column = dataset.columns[i];
            std::size_t missing_count = 0;
            std::visit(
                [&](const auto &column_data)
                {
                    for (const auto &value : column_data.values)
                    {
                        if (!value.has_value())
                        {
                            ++missing_count;
                        }
                    }
                },
                column.data);

            std::string miss_percent = std::to_string(missing_count) + " (" + std::format("{:.1f}", 100.0 * static_cast<double>(missing_count) / dataset.row_count) + "%)";

            const char *type_name = std::holds_alternative<NumericColumn>(column.data) ? "числовой" : "категориальный";
            std::cout
                << std::setw(4) << i + 1 << std::left << std::setw(15)
                << column.name << std::left << std::setw(22) << type_name << "  " << miss_percent << "\n";
        }
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
        std::size_t max_bar_length = 30;
        for (const NumericHistogramBin &bin : stats.histogram)
            max_number = std::max(max_number, bin.number);
        for (const NumericHistogramBin &bin : stats.histogram)
        {
            std::size_t bar_length = 0;
            if (max_number > 0)
            {
                bar_length = static_cast<std::size_t>(ceil(static_cast<double>(bin.number) * max_bar_length / max_number));
            }
            std::string bar(bar_length, '#');
            std::string interval = "  [" + std::format("{:.1f}", bin.lower_bound) + ", " + std::format("{:.1f}", bin.upper_bound) + ")";

            std::cout << std::left << std::setw(18) << interval
                      << std::left << std::setw(static_cast<int>(max_bar_length)) << bar << "  " << bin.number << '\n';
        }
    }

    void print_categorical_statistics(const CategoricalStatistics &stats)
    {
        std::cout << "Тип: категориальный\n";
        std::cout << "Непустых значений: " << stats.count << '\n';
        std::cout << "Пропусков: " << stats.missing_count << "\n\n";

        std::size_t max_number = 0;
        std::size_t max_bar_length = 30;
        for (const CategoricalHistogramBin &bin : stats.histogram)
            max_number = std::max(max_number, bin.number);
        for (const CategoricalHistogramBin &bin : stats.histogram)
        {
            std::size_t bar_length = 0;
            if (max_number > 0)
            {
                bar_length = static_cast<std::size_t>(ceil(static_cast<double>(bin.number) * max_bar_length / max_number));
            }
            std::string bar(bar_length, '#');
            std::cout << std::left << std::setw(25) << bin.category
                      << std::left << std::setw(static_cast<int>(max_bar_length)) << bar << "  " << bin.number << '\n';
        }
    }

    void print_feature_statistics(const Dataset &dataset, const DatasetInfo &statistics)
    {
        std::cout << "\nДоступные признаки:\n";
        for (std::size_t i = 0; i < dataset.columns.size(); ++i)
        {
            std::cout << i + 1 << "  " << dataset.columns[i].name << '\n';
        }

        const auto selected = input_int("Введите номер признака (0 - вернуться назад): ");

        if (!selected.has_value() || selected.value() == 0)
            return;

        if (selected.value() < 1 || static_cast<std::size_t>(selected.value()) > dataset.columns.size())
        {
            std::cout << "Такого номера признака нет.\n";
            return;
        }

        const std::size_t index = static_cast<std::size_t>(selected.value() - 1);
        const std::string &name = dataset.columns[index].name;

        const FeatureInfo &feature = statistics.features.at(index);
        std::cout << "\nПризнак " << selected.value() << " : " << feature.name << "\n";

        if (std::holds_alternative<NumericStatistics>(feature.statistics))
        {
            print_numeric_statistics(std::get<NumericStatistics>(feature.statistics));
            return;
        }
        else
        {
            print_categorical_statistics(std::get<CategoricalStatistics>(feature.statistics));
            return;
        }
        std::cout << "Для признака нет статистики.\n";
    }

    void run_preprocessing(const Dataset &dataset, const DatasetInfo &statistics, std::optional<TransformedDataset> &transformed, const std::string &csv_path)
    {
        try
        {
            TransformedDataset result =
                preprocess_dataset(dataset, statistics, NumericRefill::Median, Scaling::Standard, CategoricalEncoding::OneHot);

            std::cout << "\nPreprocessing завершён.\n";
            std::cout << "Строк в матрице: " << result.matrix.rows() << '\n';
            std::cout << "Столбцов в матрице: " << result.matrix.columns() << '\n';

            std::cout << "\nИтоговые признаки:\n";

            for (std::size_t i = 0; i < result.feature_names.size(); ++i)
                std::cout << i + 1 << "  " << result.feature_names[i] << '\n';

            const auto save_choice = input_int("\nСохранить матрицу в .bin? (1 — да, 0 — нет): ");

            if (!save_choice.has_value())
                return;

            if (save_choice.value() == 1)
            {
                const auto path = input_line("Файл для сохранения: ");

                if (!path.has_value())
                    return;

                const std::filesystem::path output_path = path.value();

                if (output_path.has_parent_path())
                {
                    std::filesystem::create_directories(output_path.parent_path());
                }
                result.matrix.save(output_path);
                std::cout << "Матрица сохранена: " << output_path.string() << '\n';
                auto file_size = std::filesystem::file_size(output_path);
                std::cout << "Размер .bin: " << file_size << " байт\n";

                const std::filesystem::path csv = csv_path;
                file_size = std::filesystem::file_size(csv);
                std::cout << "Размер .csv: " << file_size << " байт\n";

            }
            else if (save_choice.value() != 0)
            {
                std::cout << "Неизвестный вариант. Матрица не сохранена.\n";
            }

            transformed = std::move(result);
        }
        catch (const std::exception &error)
        {
            std::cout << "Ошибка preprocessing: " << error.what() << '\n';
        }
    }

    void import_binary_matrix(std::optional<Matrix> &loaded_matrix)
    {
        const auto path = input_line(
            "Файл для импорта: ");

        if (!path.has_value())
            return;

        const std::filesystem::path binary_path = path.value();
        try
        {
            Matrix matrix = Matrix::load(binary_path);

            std::cout << "\nМатрица загружена.\n";
            std::cout << "Объектов: " << matrix.rows() << '\n';
            std::cout << "Столбцов: " << matrix.columns() << '\n';
            loaded_matrix = std::move(matrix);
        }
        catch (const std::exception &error)
        {
            std::cout << "Ошибка загрузки .bin файла: " << error.what() << '\n';
        }
    }

} // namespace

int main()
{
    std::optional<Dataset> dataset;
    std::optional<DatasetInfo> statistics;
    std::optional<TransformedDataset> transformed;
    std::optional<Matrix> loaded_matrix;

    bool running = true;

    while (running)
    {
        print_menu();

        const auto choice = input_int("Выберите пункт меню: ");

        if (!choice.has_value())
        {
            if (std::cin.eof())
                break;
            continue;
        }

        try
        {
            switch (choice.value())
            {
            case 1:
            {
                const auto path = input_line("Путь к .csv файлу: ");

                if (!path.has_value())
                {
                    running = false;
                    break;
                }

                const std::string csv_path = path.value();

                CsvLoadResult result = load_csv(csv_path);

                if (!result.success)
                {
                    std::cout << "Не удалось загрузить .csv файл";
                    break;
                }

                DatasetInfo new_statistics = get_dataset_info(result.dataset);

                dataset = std::move(result.dataset);
                statistics = std::move(new_statistics);

                transformed.reset();
                loaded_matrix.reset();

                std::cout << "\nCSV загружен.\n";

                break;
            }

            case 2:
                if (dataset.has_value() && statistics.has_value())
                {
                    print_dataset_summary(dataset.value(), statistics.value());
                }
                else
                    std::cout << "Сначала загрузите CSV (пункт 1).\n";
                break;

            case 3:
                if (dataset.has_value() && statistics.has_value())
                {
                    print_feature_statistics(dataset.value(), statistics.value());
                }
                else
                    std::cout << "Сначала загрузите CSV (пункт 1).\n";
                break;

            case 4:
                if (dataset.has_value() && statistics.has_value())
                {
                    run_preprocessing(dataset.value(), statistics.value(), transformed, dataset->path);
                }
                else
                    std::cout << "Сначала загрузите CSV (пункт 1).\n";
                break;

            case 5:
                import_binary_matrix(loaded_matrix);
                break;

            case 0:
                running = false;
                break;

            default:
                std::cout << "Ошибка. Введите число от 0 до 6\n";
                break;
            }
        }
        catch (const std::exception &error)
        {
            std::cout << "Ошибка: " << error.what() << '\n';
        }
    }

    return 0;
}
