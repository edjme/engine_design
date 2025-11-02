#include "Palpha_output.h"
#include "ind_Palpha_path_manager.h"
#include <iostream>
#include <fstream>
#include <iomanip>

using namespace std;

void IndUnwrapOutput::exportWithMenu(const PAlphaResults &results, const UnwrapParams &params)
{
    cout << "\n=== ЭКСПОРТ РЕЗУЛЬТАТОВ РАЗВЁРТКИ ===\n";
    cout << "1 — Сохранить в CSV\n";
    cout << "2 — Сохранить только HTML-график\n";
    cout << "3 — Сохранить во все форматы\n";
    cout << "0 — Не сохранять\n";
    cout << "Ваш выбор: ";

    int choice;
    cin >> choice;
    if (choice == 0)
        return;

    string csvPath = PathManagerIndUnwrap::getOutputDirectory() + "/palpha.csv";
    string htmlPath = PathManagerIndUnwrap::getOutputDirectory() + "/palpha.html";

    if (choice == 1 || choice == 3)
    {
        ofstream f(csvPath);
        if (f.is_open())
        {
            f << "alpha_deg;P_Pa\n";
            for (size_t i = 0; i < results.alpha_deg.size(); ++i)
            {
                f << fixed << setprecision(4)
                  << results.alpha_deg[i] << ";" << results.P_alpha[i] << "\n";
            }
            cout << "✅ CSV сохранён: " << csvPath << endl;
        }
        else
            cerr << "❌ Ошибка записи CSV.\n";
    }

    if (choice == 2 || choice == 3)
    {
        ofstream f(htmlPath);
        if (f.is_open())
        {
            f << results.htmlContent;
            cout << "✅ HTML сохранён: " << htmlPath << endl;
        }
        else
            cerr << "❌ Ошибка записи HTML.\n";
    }
}
