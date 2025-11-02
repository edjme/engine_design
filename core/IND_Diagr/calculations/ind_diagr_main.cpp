#include <iostream>
#include <cmath>
#include <vector>
#include "core/IND_Diagr/input_data/ind_diag_input.h"
#include "core/IND_Diagr/output_data/ind_diag_output.h"
#include "core/common/ind_diag_common_types.h"
#include "core/IND_Diagr/calculations/calc_ind_diagr.h"
#include "ind_path_manager.h"
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
        cout << "\n=========================================\n";
        cout << "   РАСЧЁТ ИНДИКАТОРНОЙ ДИАГРАММЫ P–V\n";
        cout << "=========================================\n";

        // Убедимся, что папки есть
        IndPathManager::ensureDirectoriesExist();

        // Загружаем параметры
        IndParams params = IndInput::IndloadParamsWithMenu();
        IndInput::IndprintParams(params);

        cout << "\nВыполняется расчёт индикаторной диаграммы...\n";

        // Пути к результатам
        string csv_path = IndPathManager::getOutputDirectory() + "/indicator_PV.csv";
        string html_path = IndPathManager::getOutputDirectory() + "/indicator_PV.html";

        // Расчёт с построением HTML
        IndicatorResults results = build_indicator_PV(
            params,
            400,       // число точек
            csv_path,  // CSV
            html_path, // HTML
            false);

        cout << "\n✅ Расчёт завершён успешно.\n";
        cout << "HTML-график: " << html_path << endl;

        // Экспорт данных в нужный формат
        IndDiagramOutput::exportWithMenu(results, params);

        // ---------------- Меню повторного расчета ----------------
        int otvet = 0;
        cout << "\nСделать ещё один расчёт?\n"
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
            cout << "\nПерехожу к следующему расчёту...\n\n";
        }
    }

    return 0;
}
