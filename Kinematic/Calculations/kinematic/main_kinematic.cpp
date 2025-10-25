#include <iostream>
#include <cmath>
#include <vector>
#include "Kinematic/input_data/kinematic_input.h"
#include "Kinematic/output_data/kinematic_output.h"
#include "common/common_types.h"
#include "kinematic_formulas.h"
#ifdef _WIN32
#include <windows.h>
#endif

using namespace std;

int main()
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    system("chcp 65001 > nul");
#endif

    bool programIsOn = true;

    while (programIsOn)
    {
        // Загружаем параметры и выводим их
        EngineParams params = KinematicInput::loadParamsWithMenu();
        KinematicInput::printParams(params);


        // Расчет
        CalculationResults results = calcCylinderKinematics(params);

        // Экспорт результатов
        KinematicOutput::exportWithMenu(results, params);

        // ---------------- Меню повторного расчета ----------------
        int otvet = 0;
        cout << "\nСделать еще один расчет?\n"
             << "------------------\n"
             << "1 - Да\n"
             << "2 - Нет\n"
             << "------------------\n"
             << "Ваш ответ: ";
        cin >> otvet;

        if (otvet == 2)
        {
            cout << "\nДо свидания!\n";
            programIsOn = false;
        }
        else
        {
            cout << "\nПерехожу к следующему расчету...\n\n";
        }
    }

    return 0;
}
