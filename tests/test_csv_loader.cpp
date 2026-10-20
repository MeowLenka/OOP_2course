// Разбор CSV: пропуски, кавычки, определение типа признака, несогласованный файл.

#include "csv_loader.hpp"
#include "test_utils.hpp"

#include <filesystem>
#include <fstream>
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
            "\"\",\"city\",\"height\",\"flag\"\r\n"
            "1,\"Moscow\",170.5,\"a\"\r\n"
            "2,\"Kazan\",NA,\"b\"\r\n"
            "3,NA,181,\"a\"\r\n"
            "4,\"Saint Petersburg, RU\",160,\"b\"\r\n"
            "5,\"say \"\"hi\"\"\",165,\"a\"\r\n");

        const CsvLoadResult result = load_csv(path.string());
        CHECK(result.success);

        const Dataset &dataset = result.dataset;
        CHECK(dataset.row_count == 5);
        CHECK(dataset.columns.size() == 4);

        CHECK(dataset.columns[0].name.empty()); // кавычки сняты
        CHECK(dataset.columns[1].name == "city");

        // Пропуск -> std::nullopt, а не особое значение.
        const auto &city = std::get<CategoricalColumn>(dataset.columns[1].data);
        CHECK(city.values[0] == "Moscow");
        CHECK(!city.values[2].has_value());
        CHECK(city.values[3] == "Saint Petersburg, RU"); // запятая внутри кавычек
        CHECK(city.values[4] == "say \"hi\"");           // "" -> "

        const auto &height = std::get<NumericColumn>(dataset.columns[2].data);
        CHECK(height.values[0].has_value());
        CHECK_NEAR(height.values[0].value(), 170.5, 1e-12);
        CHECK(!height.values[1].has_value());

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

        CHECK(is_numeric(dataset.columns[0]));  // только числа
        CHECK(!is_numeric(dataset.columns[1])); // строки
        CHECK(is_numeric(dataset.columns[2]));  // числа + пропуск
        CHECK(!is_numeric(dataset.columns[3])); // числа и слово -> категориальный

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
            CHECK(std::string(error.what()).find('3') != std::string::npos);
        }
        CHECK(thrown);

        std::filesystem::remove(path);
    }

    void test_bad_files()
    {
        CHECK_THROWS(load_csv("/definitely/not/existing/file.csv"));

        const auto empty = write_temp("lab1_csv_empty.csv", "");
        CHECK_THROWS(load_csv(empty.string()));
        std::filesystem::remove(empty);
    }
} // namespace

int main()
{
    run_test("test_missing_and_quotes", test_missing_and_quotes);
    run_test("test_type_detection", test_type_detection);
    run_test("test_inconsistent_csv", test_inconsistent_csv);
    run_test("test_bad_files", test_bad_files);
    return finish_tests("test_csv_loader");
}
