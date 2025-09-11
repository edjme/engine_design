#include "input.h"
#include <iostream>

int main()
{
    try
    {
        // Создать рядом с exe: kinematic_input.csv (если его ещё нет)
        bool created = kinio::EnsureDefaultInputCSV();
        if (created)
        {
            std::cout << "Default kinematic_input.csv created\n";
        }
        else
        {
            std::cout << "kinematic_input.csv already exists, left unchanged\n";
        }

        // Или принудительно пересоздать в указанной папке:
        // auto out = kinio::WriteDefaultInputCSV("data/");
        // std::cout << "Wrote: " << out << "\n";
    }
    catch (const std::exception &e)
    {
        std::cerr << "ERROR: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
