#include "ind_diag_output.h"
#include "ind_path_manager.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace std;

string IndDiagramOutput::generateFilename(const string &prefix)
{
    time_t now = time(0);
    tm *localTime = localtime(&now);

    stringstream filename;
    filename << prefix << "_"
             << setw(2) << setfill('0') << localTime->tm_hour << "-"
             << setw(2) << setfill('0') << localTime->tm_min << "-"
             << setw(2) << setfill('0') << localTime->tm_sec << "_"
             << setw(2) << setfill('0') << localTime->tm_mday << "."
             << setw(2) << setfill('0') << (localTime->tm_mon + 1) << "."
             << (1900 + localTime->tm_year);

    return filename.str();
}

void IndDiagramOutput::writeHeader(ofstream &file, const IndParams &params)
{
    time_t now = time(0);

    file << "# Результаты расчёта индикаторной диаграммы P–V\n";
    file << "# Дата: " << ctime(&now);
    file << "# --------------------------------------------\n";
    file << "# Параметры расчёта:\n";
    file << "# Радиус кривошипа r = " << params.radcrank << " м\n";
    file << "# Диаметр цилиндра D = " << params.diam_cyl << " м\n";
    file << "# Степень сжатия ε = " << params.epsilent << "\n";
    file << "# λ = " << params.lyambda << "\n";
    file << "# n1 = " << params.n_1 << ", n2 = " << params.n_2 << "\n";
    file << "# Давления: Pa=" << params.p_a << " Па, Pr=" << params.p_r << " Па\n";
    file << "# --------------------------------------------\n";
}

bool IndDiagramOutput::saveToCSV(const IndicatorResults &results,
                                 const IndParams &params,
                                 const string &filename)
{
    IndPathManager::ensureDirectoriesExist();

    string outputFilename = filename.empty()
                                ? IndPathManager::getOutputDirectory() + "/" +
                                      generateFilename("indicator_results") + ".csv"
                                : filename;

#ifdef _WIN32
    wstring widePath = IndPathManager::utf8ToWide(outputFilename);
    ofstream file(widePath.c_str(), ios::binary);
#else
    ofstream file(outputFilename, ios::binary);
#endif

    if (!file.is_open())
    {
        cerr << "Ошибка: не удалось создать файл " << outputFilename << endl;
        return false;
    }

    file << "\xEF\xBB\xBF"; // BOM для UTF-8
    writeHeader(file, params);
    file << "\nV[м^3];P[Па]\n";

    file << fixed << setprecision(6);
    for (size_t i = 0; i < results.V_path.size(); ++i)
    {
        file << results.V_path[i] << ";" << results.P_path[i] << "\n";
    }

    file.close();
    cout << "✅ CSV файл сохранён: " << outputFilename << endl;
    return true;
}

bool IndDiagramOutput::saveSummary(const IndicatorResults &results,
                                   const IndParams &params,
                                   const string &filename)
{
    IndPathManager::ensureDirectoriesExist();

    string outputFilename = filename.empty()
                                ? IndPathManager::getOutputDirectory() + "/" +
                                      generateFilename("indicator_summary") + ".txt"
                                : filename;

#ifdef _WIN32
    wstring widePath = IndPathManager::utf8ToWide(outputFilename);
    ofstream file(widePath.c_str());
#else
    ofstream file(outputFilename);
#endif

    if (!file.is_open())
    {
        cerr << "Ошибка создания файла: " << outputFilename << endl;
        return false;
    }

    writeHeader(file, params);
    file << "\n";

    file << "Основные результаты:\n";
    file << "-------------------------------------------\n";
    file << "Индикаторная работа за цикл: " << results.A_cycle << " Дж\n";
    file << "Подвод теплоты: " << results.Q_in << " Дж\n";
    file << "η индикаторный: " << results.eta << "\n";
    file << "Pc = " << results.Pc << " Па\n";
    file << "Pz = " << results.Pz << " Па\n";
    file << "Pb = " << results.Pb << " Па\n";
    file << "-------------------------------------------\n";
    file << results.summary << "\n";

    file.close();
    cout << "📄 Итоговый отчёт сохранён: " << outputFilename << endl;
    return true;
}

void IndDiagramOutput::exportWithMenu(const IndicatorResults &results,
                                      const IndParams &params)
{
    int choice;
    string filename;

    cout << "\n=== ЭКСПОРТ РЕЗУЛЬТАТОВ ===\n";
    cout << "1 - Сохранить CSV с точками P–V\n";
    cout << "2 - Сохранить сводку (основные параметры)\n";
    cout << "3 - Сохранить всё\n";
    cout << "4 - Не сохранять\n";
    cout << "-----------------------------\n";
    cout << "Ваш выбор: ";
    cin >> choice;

    if (choice == 4)
        return;

    switch (choice)
    {
    case 1:
        saveToCSV(results, params);
        break;
    case 2:
        saveSummary(results, params);
        break;
    case 3:
        saveToCSV(results, params);
        saveSummary(results, params);
        break;
    default:
        cout << "⚠️ Неверный ввод.\n";
        return;
    }

    // === После экспорта спрашиваем про открытие графика ===
    string html_path = IndPathManager::getOutputDirectory() + "/indicator_PV.html";

    cout << "\nХотите открыть график индикаторной диаграммы в браузере? (1 - Да / 2 - Нет): ";
    int openChoice;
    cin >> openChoice;

    if (openChoice == 1)
    {
#ifdef _WIN32
        ShellExecuteA(nullptr, "open", html_path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
#else
        string cmd = "xdg-open \"" + html_path + "\"";
        system(cmd.c_str());
#endif
        cout << "\n📈 Диаграмма открыта: " << html_path << endl;
    }
    else
    {
        cout << "\nДиаграмма сохранена по пути: " << html_path << endl;
    }
}
