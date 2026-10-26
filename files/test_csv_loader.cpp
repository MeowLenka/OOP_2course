// Разбор CSV: пропуски, кавычки, определение типа признака, несогласованный файл.

#undef NDEBUG
#include <cassert>

#include "csv_loader.hpp"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <variant>

namespace
{
    std::filesystem::path write_temp(const std::string &name, const std::string &text)
    {
        const auto path = std::filesystem::temp_directory_path() / name;
        std::ofstream output(path, std::ios::binary);
        output << text;
        return path;
    }

    bool is_numeric(const Column &column)
    {
        return std::holds_alternative<NumericColumn>(column.data);
    }

    void test_missing_and_quotes()
    {
        // Формат как у выбранного датасета: значения в кавычках, CRLF, пропуск NA,
        // первый столбец с пустым именем.
        const auto path = write_temp(
            "lab1_csv_quotes.csv",
            ",city,height,flag\r\n"
            "1,\"Moscow\",170.5,\"a\"\r\n"
            "2,\"Kazan\",NA,\"b\"\r\n"
            "3,NA,181,\"a\"\r\n"
            "4,\"Saint Petersburg, RU\",160,\"b\"\r\n"
            "5,\"say \"\"hi\"\"\",165,\"a\"\r\n");

        const CsvLoadResult result = load_csv(path.string());
        assert(result.success);

        const Dataset &dataset = result.dataset;
        assert(dataset.row_count == 5);
        assert(dataset.columns.size() == 4);

        assert(dataset.columns[0].name.empty());
        assert(dataset.columns[1].name == "city");

        // Кавычки вокруг значения сняты; пропуск -> std::nullopt.
        const auto &city = std::get<CategoricalColumn>(dataset.columns[1].data);
        assert(city.values[0] == "Moscow");
        assert(!city.values[2].has_value());
        assert(city.values[3] == "Saint Petersburg, RU"); // запятая внутри кавычек
        assert(city.values[4] == "say \"hi\"");           // "" -> "

        const auto &flag = std::get<CategoricalColumn>(dataset.columns[3].data);
        assert(flag.values[0] == "a");

        const auto &height = std::get<NumericColumn>(dataset.columns[2].data);
        assert(height.values[0].has_value());
        assert(std::fabs(height.values[0].value() - 170.5) < 1e-12);
        assert(!height.values[1].has_value());

        std::filesystem::remove(path);
    }

    void test_type_detection()
    {
        const auto path = write_temp(
            "lab1_csv_types.csv",
            "a,b,c,d\n"
            "1,x,1.5,10\n"
            "2,y,NA,abc\n"
            "3,x,2.5,30\n");

        const Dataset dataset = load_csv(path.string()).dataset;

        assert(is_numeric(dataset.columns[0]));  // только числа
        assert(!is_numeric(dataset.columns[1])); // строки
        assert(is_numeric(dataset.columns[2]));  // числа + пропуск
        assert(!is_numeric(dataset.columns[3])); // числа и слово -> категориальный

        std::filesystem::remove(path);
    }

    void test_inconsistent_csv()
    {
        const auto path = write_temp(
            "lab1_csv_bad.csv",
            "a,b,c\n"
            "1,2,3\n"
            "4,5\n"
            "7,8,9\n");

        bool thrown = false;
        try
        {
            (void)load_csv(path.string());
        }
        catch (const std::runtime_error &error)
        {
            thrown = true;
            // В сообщении должен быть номер проблемной строки (3).
            assert(std::string(error.what()).find('3') != std::string::npos);
        }
        assert(thrown);

        std::filesystem::remove(path);
    }

    void test_bad_files()
    {
        bool missing_thrown = false;
        try
        {
            (void)load_csv("/definitely/not/existing/file.csv");
        }
        catch (const std::runtime_error &)
        {
            missing_thrown = true;
        }
        assert(missing_thrown);

        const auto empty = write_temp("lab1_csv_empty.csv", "");
        bool empty_thrown = false;
        try
        {
            (void)load_csv(empty.string());
        }
        catch (const std::runtime_error &)
        {
            empty_thrown = true;
        }
        assert(empty_thrown);
        std::filesystem::remove(empty);
    }
} // namespace

int main()
{
    test_missing_and_quotes();
    test_type_detection();
    test_inconsistent_csv();
    test_bad_files();
    std::cout << "test_csv_loader: OK\n";
    return 0;
}
