#include <iostream>
#include <string>
#include "core/Diag_Palpha/Input_data/Palpha_input.h"
#include "core/Diag_Palpha/Output_data/Palpha_output.h"
#include "calc_palpha.h"
#include "ind_Palpha_path_manager.h"

#ifdef _WIN32
#include <windows.h>
#endif

using namespace std;

int main()
{
#ifdef _WIN32
    // Включаем корректный вывод кириллицы в консоль Windows
    SetConsoleOutputCP(CP_UTF8);
    system("chcp 65001 > nul");
#endif

    cout << "=============================================\n";
    cout << "   РАЗВЁРТКА ИНДИКАТОРНОЙ ДИАГРАММЫ (P–α)\n";
    cout << "=============================================\n\n";

    bool programIsOn = true;
    while (programIsOn)
    {
        // Проверяем/создаём рабочие директории
        if (!PathManagerIndUnwrap::ensureDirectoriesExist())
        {
            cerr << "❌ Не удалось создать рабочие папки. Завершение программы.\n";
            return 1;
        }

        // Загружаем параметры с меню выбора режима
        UnwrapParams params = IndUnwrapInput::loadParamsWithMenu();

        cout << "\n---------------------------------------------\n";
        cout << " Выполняется построение зависимости P(α)...\n";
        cout << "---------------------------------------------\n\n";

        // Основной расчёт (в зависимости от режима)
        PAlphaResults results = build_P_alpha(params);

        // Экспорт (CSV / HTML / оба)
        IndUnwrapOutput::exportWithMenu(results, params);

        // Вывод краткого отчёта
        cout << "\n---------------------------------------------\n";
        cout << " СВОДКА:\n";
        cout << results.summary << endl;
        cout << "---------------------------------------------\n";

        // Спросить, открыть ли диаграмму в браузере
        int open = 0;
        cout << "\nОткрыть диаграмму P(α) в браузере? (1 — Да, 2 — Нет): ";
        cin >> open;

#ifdef _WIN32
        if (open == 1)
        {
            string htmlPath = PathManagerIndUnwrap::getOutputDirectory() + "/palpha.html";
            ShellExecuteA(nullptr, "open", htmlPath.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        }
#else
        if (open == 1)
        {
            string htmlPath = PathManagerIndUnwrap::getOutputDirectory() + "/palpha.html";
            system(("xdg-open " + htmlPath).c_str());
        }
#endif

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
    cout << "\n✅ Работа программы завершена успешно.\n\n";
    return 0;
}
