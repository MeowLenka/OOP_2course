#include <iostream>
#include <fstream>
#include "include/dataset.hpp"

using namespace std;

int main()
{
    ifstream mainFile;
    string line;
    mainFile.open("data/dataset.csv");
    if (!mainFile.is_open())
    {
        std::cerr << "Не удалось открыть data/dataset.csv\n";
        return 1;
    }
    else
    {
        size_t current_location = 0;
        size_t separator_location = 0;

        getline(mainFile, line);
        cout << line << '\n';
        separator_location = line.find(separator);
        std::string first = line.substr(current_location, separator_location);
        cout << first << '\n';
        current_location = separator_location;
        return 0;
    }
}
