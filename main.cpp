/*  ВЫЗЫВАЕТ ИНТЕРФЕЙС (в случае необходимости закрыть в многострочный комментарий)*/
/*
#ifdef _WIN32
#include <windows.h>
#include "Interface/gui_win.h"

// точка входа GUI
int main()
{ // или int main(), если собираешь как консольное
    SetConsoleOutputCP(65001);
    return run_gui(); // полностью передаём управление GUI
}

#endif
*/

// ВЫВОД РЕЗУЛЬТАТОВ В КОМПИЛЯТОР БЕЗ ИНТЕРФЕЙСА , ДЛЯ ПРОВЕРКИ (в случае необходимости закрыть в "/* */")

// Для удобного форматирования

// main.cpp
// Консольный запуск всех модулей БЕЗ интерфейса.
// Печатает сводку и сохраняет результаты/графики/HTML туда,
// куда уже настроены ваши функции (здесь — в ./output).

// main.cpp — консольный вывод результатов расчётов (работает и с -mwindows)
// Собирается как с -mwindows, так и без него.
// g++ main.cpp input_data/input.cpp Calculations/*.cpp -o main.exe -std=c++17
// (для GUI exe не добавляй -mwindows — тогда консоль откроется автоматически)
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include <cstdio>
#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <cctype>
#include <cmath>

#include "input_data/input.h"
#include "Calculations/dynamic/ind_diagr.h"
#include "Calculations/dynamic/diag_palpha.h"
#include "Calculations/dynamic/forces_ksm.h"
#include "Calculations/dynamic/vds_crankpin.h"
#include "Calculations/dynamic/calc_mass_crankshaft.h"
#include "Calculations/dynamic/cw_counterweights.h"
#include "Calculations/dynamic/balance_inertia.h"
#include "Calculations/dynamic/flywheel_inertia.h"

// ──────────────────────────────────────────────────────────────
// Консоль для GUI-сборки (-mwindows)
static void attach_console_if_needed()
{
    if (GetConsoleWindow())
        return;
    if (!AttachConsole(ATTACH_PARENT_PROCESS))
    {
        if (!AllocConsole())
            return;
    }
    FILE *f;
    freopen_s(&f, "CONOUT$", "w", stdout);
    freopen_s(&f, "CONOUT$", "w", stderr);
    freopen_s(&f, "CONIN$", "r", stdin);
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
    std::ios::sync_with_stdio(false);
    std::wcout.sync_with_stdio(false);
}

// Печать «ключ: значение»
static void print_kv(const std::string &k, double v, int prec = 6, const char *unit = "")
{
    std::cout << "  " << std::left << std::setw(28) << k << ": "
              << std::fixed << std::setprecision(prec) << v
              << (unit[0] ? (" " + std::string(unit)) : "") << "\n";
}

// Быстрая проверка, что файл успешно создан
static void check_file(const std::string &path, const char *title)
{
    if (std::filesystem::exists(path))
        std::cout << "    [ok] " << title << ": " << path << "\n";
    else
        std::cout << "    [warn] Файл не найден: " << path
                  << "  (проверьте путь/кодировку/права)\n";
}

// Пытаемся вытащить I_z(total) из cw.message, где ты пишешь:
// "... I_z(total)=<число> kg·m^2"
static double extract_Iz_total_from_cw_message(const std::string &msg)
{
    std::string keys[2] = {"I_z(total)=", "I_z(total) ="};
    size_t pos = std::string::npos;
    for (auto &k : keys)
    {
        pos = msg.find(k);
        if (pos != std::string::npos)
        {
            pos += k.size();
            break;
        }
    }
    if (pos == std::string::npos)
        return std::numeric_limits<double>::quiet_NaN();

    // пропустим пробелы
    while (pos < msg.size() && std::isspace(static_cast<unsigned char>(msg[pos])))
        ++pos;

    // считываем число (включая e/E-экспоненту)
    size_t end = pos;
    while (end < msg.size())
    {
        char c = msg[end];
        if (std::isdigit(static_cast<unsigned char>(c)) || c == '.' || c == 'e' || c == 'E' || c == '+' || c == '-')
        {
            ++end;
        }
        else
            break;
    }
    try
    {
        return std::stod(msg.substr(pos, end - pos));
    }
    catch (...)
    {
        return std::numeric_limits<double>::quiet_NaN();
    }
}

// ──────────────────────────────────────────────────────────────

int main()
{
    attach_console_if_needed();

    try
    {
        std::cout << u8"== Запуск консольного расчёта ==\n\n";

        // 0) Папка результатов (ASCII-имя)
        std::filesystem::create_directories("output");
        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm tm{};
        localtime_s(&tm, &t);
        std::ostringstream ts;
        ts << std::put_time(&tm, "%d-%m-%Y_%H-%M-%S");
        std::string outdir = "output/results_" + ts.str();
        std::filesystem::create_directories(outdir);
        auto J = [&](const std::string &fn)
        { return outdir + "/" + fn; };
        std::cout << u8"[0] Папка результатов: " << outdir << "\n";
        {
            std::ofstream r(J("README.txt"));
            r << "Calculation outputs for run at " << ts.str() << "\n";
        }

        // 1) Входные данные
        Params p = input();
        std::cout << u8"[1] Входные данные загружены из input_data/input.csv\n\n";

        // 2) Индикаторная P–V
        auto ind = build_indicator_PV(p, 400, J("ind_diag.csv"), J("ind_diag.html"), false);
        std::cout << u8"[2] Индикаторная диаграмма (PV): ок\n";
        check_file(J("ind_diag.csv"), "ind_diag.csv");
        check_file(J("ind_diag.html"), "ind_diag.html");

        // 3) Развёртка P(α)
        auto pal = build_P_alpha(p, ind, 0.5, J("palpha.csv"), J("palpha.html"), false);
        std::cout << u8"[3] Развёртка P(α): ок\n";
        check_file(J("palpha.csv"), "palpha.csv");
        check_file(J("palpha.html"), "palpha.html");

        // 4) Силы/моменты КШМ
        auto fr = build_forces_ksm(p, pal, J("forces_ksm.csv"), J("forces_ksm.html"), false);
        std::cout << u8"[4] Силы/момент КШМ: ок\n";
        check_file(J("forces_ksm.csv"), "forces_ksm.csv");
        check_file(J("forces_ksm.html"), "forces_ksm.html");

        // 5) ВДС шатунной шейки
        auto vds = build_vds_crankpin(p, fr, J("vds.csv"), J("vds.html"), false);
        std::cout << u8"[5] ВДС шейки: ок\n";
        check_file(J("vds.csv"), "vds.csv");
        check_file(J("vds.html"), "vds.html");

        // 6) Масса/ЦТ колена
        auto cm = calc_mass_crankshaft(p);
        std::cout << u8"[6] Масса/ЦТ колена: ок\n";
        std::cout << u8"— Масса и ЦТ колена:\n";
        print_kv("Масса колена, кг", cm.total_mass);
        print_kv("y_CG от оси коренной, м", cm.y_cg);
        print_kv("m_root_reduce, кг/м^2", cm.m_root_reduce);
        print_kv("m_rotating, кг/м^2", cm.m_rotating);
        print_kv("axis_p, м", cm.axis_p);
        print_kv("web_p, м", cm.web_p);
        print_kv("axis_n, м", cm.axis_n);
        print_kv("web_n, м", cm.web_n);
        std::cout << "\n";

        // 7) Баланс сил/моментов инерции
        auto bi = build_balance_inertia(p, cm, J("balance_inertia.csv"), J("balance_inertia.html"), false);
        std::cout << u8"[7] Баланс сил/моментов инерции: ок\n";
        print_kv("S_prot1 (1-й порядок), кг·м", bi.S_prot1);
        print_kv("S_prot2 (2-й порядок), кг·м", bi.S_prot2);
        check_file(J("balance_inertia.csv"), "balance_inertia.csv");
        check_file(J("balance_inertia.html"), "balance_inertia.html");
        std::cout << "\n";

        // 8) Противовесы (STL и расчёт S_prot/α)
        CWVariant var = CWVariant::FullSupport_V1;
        const int prot_cfg = static_cast<int>(p.config_prot);
        if (prot_cfg == 2)
            var = CWVariant::FullSupport_V2;
        else if (prot_cfg == 3)
            var = CWVariant::SemiSupport;

        auto cw = build_counterweights_and_export(p, cm, var, J("crank_with_cw_mm.stl"), 128, true);
        std::cout << u8"[8] Противовесы: ок\n";
        print_kv("S_prot, кг·м", cw.S_prot);
        print_kv("α сектора, град", cw.alpha_deg, 3);
        if (!cw.message.empty())
            std::cout << u8"  Примечание: " << cw.message << "\n";
        check_file(J("crank_with_cw_mm.stl"), "crank_with_cw_mm.stl");
        std::cout << "\n";

        // 9) Геометрия Ланчестера
        const bool semi = (static_cast<int>(p.config_crankshaft) == 2);
        double c_lanch = semi
                             ? (cm.web_n + p.depth_web + p.length_root_neck)
                             : (cm.web_p + p.depth_web + p.length_root_neck);
        std::cout << u8"[9] Геометрия Ланчестера: c_lanch = "
                  << std::fixed << std::setprecision(6) << c_lanch << " м\n\n";

        // 10) Моменты инерции моторных масс и маховика

        // Берём I_kv_cw (колено+CW) из результата CW напрямую.
        double I_kv_cw = cw.Iz_total;
        const int cylinders = static_cast<int>(std::lround(p.count_cyl));
        const double delta = p.delta; // степень неравномерности вращения — читаем из Params

        auto fi = compute_flywheel_inertia(
            p, fr, I_kv_cw, cylinders, delta,
            J("flywheel.csv"), J("flywheel.html"), true);

        std::cout << u8"[10] Моторные массы и маховик: ок\n";
        print_kv("I_kv+CW (ось коренной), кг·м^2", fi.I_kv_cw);
        print_kv("I_mm (1 цилиндр), кг·м^2", fi.I_mm_one);
        print_kv("Средний момент Ms, Н·м", fi.Ms);
        print_kv("ΔA (размах избыточной работы), Дж", fi.dA);
        print_kv("I0 требуемый, кг·м^2", fi.I0_needed);
        print_kv("Число цилиндров, i", (double)fi.cylinders, 0);
        print_kv("I_fly (маховик, треб.), кг·м^2", fi.I_fly);
        check_file(J("flywheel.csv"), "flywheel.csv");
        check_file(J("flywheel.html"), "flywheel.html");
        std::cout << "\n";

        std::cout << u8"== Готово ==\n";
        std::cout << u8"Файлы сохранены в: " << outdir << "\n";
    }
    catch (const std::exception &e)
    {
        std::cerr << u8"[ОШИБКА std::exception] " << e.what() << "\n";
    }
    catch (...)
    {
        std::cerr << u8"[ОШИБКА] Неизвестное исключение\n";
    }

    std::cout << u8"\nНажмите Enter для выхода...";
    std::cin.get();
    return 0;
}
