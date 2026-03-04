#include "dynamic_output.h"

#include <fstream>
#include <iomanip>
#include <ctime>
#include <limits>

static std::string nowString()
{
    std::time_t now = std::time(nullptr);
    std::tm* lt = std::localtime(&now);
    char buf[64]{};
    std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
                  1900 + lt->tm_year, 1 + lt->tm_mon, lt->tm_mday,
                  lt->tm_hour, lt->tm_min, lt->tm_sec);
    return std::string(buf);
}

bool DynamicOutput::canWriteToFile(const std::string& filename)
{
    std::ofstream test(filename, std::ios::out | std::ios::binary);
    return test.good();
}


void DynamicOutput::writeHeaderText(std::ofstream& file,
                                   const EngineParams& params,
                                   const DynamicExportInfo& info)
{
    file << "РЕЗУЛЬТАТЫ РАСЧЁТА ДИНАМИКИ КШМ\n";
    file << "Дата: " << nowString() << "\n\n";

    file << "ПАРАМЕТРЫ ДВИГАТЕЛЯ\n";
    file << " шаг альфа : " << params.step_alpha << " град\n";
    file << " предел альфа  : " << params.end_alpha  << " град\n";
    file << " радиус кривошипа  : " << params.radcrank   << " м\n";
    file << " лямбда    : " << params.lyambda    << "\n";
    file << " n          : " << params.n          << " об/мин\n";
    file << " угол развала      : " << params.gamma      << " град\n";
    file << " угол прицепного шатуна  : " << params.gammaPric  << " град\n";
    file << " дезаксиал   : " << params.dezaxial   << " м\n";
    file << " кол-во цилиндров  : " << params.countCyl   << "\n";
    file << " тактность   : " << params.taktnost   << "\n\n";

    file << "ПАРАМЕТРЫ ДИНАМИКИ\n";
    file << "# файл давления = " << info.pressure_file << "\n";
    file << " масса поршня   : " << info.mass_piston << " кг\n";
    file << " масса шатуна      : " << info.mass_rod << " кг\n";
    file << " диаметр         : " << info.bore << " м\n\n";
}

void DynamicOutput::writeVectorStats(std::ofstream& file,
                                    const std::string& title,
                                    const std::vector<double>& x,
                                    const std::vector<double>& alpha,
                                    const std::string& unit)
{
    if (x.empty() || alpha.empty()) return;

    double minV = std::numeric_limits<double>::infinity();
    double maxV = -std::numeric_limits<double>::infinity();
    size_t iMin = 0, iMax = 0;
    long double sum = 0.0;

    const size_t n = std::min(x.size(), alpha.size());
    for (size_t i = 0; i < n; ++i)
    {
        const double v = x[i];
        if (v < minV) { minV = v; iMin = i; }
        if (v > maxV) { maxV = v; iMax = i; }
        sum += v;
    }

    const double avg = static_cast<double>(sum / static_cast<long double>(n));

    file << title << "\n";
    file << "  мин = " << minV << " " << unit << "  при альфа =" << alpha[iMin] << " град\n";
    file << "  макс = " << maxV << " " << unit << "  при альфа =" << alpha[iMax] << " град\n";
    file << "  среднее значение = " << avg  << " " << unit << "\n\n";
}

static void writeBOM(std::ofstream& f)
{
    // UTF-8 BOM, чтобы Excel нормально открывал кириллицу
    f << "\xEF\xBB\xBF";
}

bool DynamicOutput::saveToCSVWide(const std::vector<double>& alpha,
                                 const DynamicResults& results,
                                 const EngineParams& params,
                                 const DynamicExportInfo& info,
                                 const std::string& filename)
{
    if (filename.empty()) return false;
    if (!canWriteToFile(filename)) return false;
    if (alpha.empty() || results.total_torque.empty()) return false;

    std::ofstream file(filename, std::ios::out | std::ios::binary);
    if (!file.is_open()) return false;

    writeBOM(file);

    // Шапка
    file << "# Результаты расчёта динамики КШМ\n";
    file << "# Дата: " << nowString() << "\n";
    file << "# файл давления = " << info.pressure_file << "\n";
    file << "# масса поршня   = " << info.mass_piston << " кг\n";
    file << "# масса шатуна     = " << info.mass_rod << " кг\n";
    file << "# диаметр          = " << info.bore << " м\n";
    file << "# площадь поршня   = " << results.piston_area << " м^2\n";
    file << "\n";

    // Заголовок таблицы
    file << "альфа[град];сум_момент[Н*м]";

    const size_t cylCount = results.cylinder_torque.size();
    for (size_t c = 0; c < cylCount; ++c)
    {
        file << ";цил" << (c+1) << "_сила_газ[Н]"
             << ";цил" << (c+1) << "_сила_инерц[Н]"
             << ";цил" << (c+1) << "_сум_сила[Н]"
             << ";цил" << (c+1) << "_шат_сила[Н]"
             << ";цил" << (c+1) << "_танг_сила[Н]"
             << ";цил" << (c+1) << "_рад_сила[Н]"
             << ";цил" << (c+1) << "_момент[Н*м]";
    }
    file << "\n";

    const size_t n = alpha.size();
    file << std::fixed << std::setprecision(6);

    for (size_t i = 0; i < n; ++i)
    {
        file << alpha[i] << ";" << (i < results.total_torque.size() ? results.total_torque[i] : 0.0);

        for (size_t c = 0; c < cylCount; ++c)
        {
            auto get = [&](const std::vector<std::vector<double>>& vv) -> double {
                if (c >= vv.size()) return 0.0;
                if (i >= vv[c].size()) return 0.0;
                return vv[c][i];
            };

            file << ";" << get(results.gas_force)
                 << ";" << get(results.inertia_force)
                 << ";" << get(results.total_force)
                 << ";" << get(results.rod_force)
                 << ";" << get(results.tangential_force)
                 << ";" << get(results.radial_force)
                 << ";" << get(results.cylinder_torque);
        }
        file << "\n";
    }

    return true;
}

bool DynamicOutput::saveToCSVLong(const std::vector<double>& alpha,
                                 const DynamicResults& results,
                                 const EngineParams& params,
                                 const DynamicExportInfo& info,
                                 const std::string& filename)
{
    (void)params;
    if (filename.empty()) return false;
    if (!canWriteToFile(filename)) return false;
    if (alpha.empty() || results.total_torque.empty()) return false;

    std::ofstream file(filename, std::ios::out | std::ios::binary);
    if (!file.is_open()) return false;

    writeBOM(file);

    file << "# Dynamic export (long)\n";
    file << "# Дата: " << nowString() << "\n";
    file << "# файл давления = " << info.pressure_file << "\n";
    file << "\n";

    file << "альфа[град];цил;сила_газ[Н];сила_инерц[Н];сум_сила[Н];шат_сила[Н];танг_сила[Н];рад_сила[Н]; момент_цил[Н*м];сум_момент[Н*м]\n";

    const size_t cylCount = results.cylinder_torque.size();
    const size_t n = alpha.size();

    file << std::fixed << std::setprecision(6);

    for (size_t i = 0; i < n; ++i)
    {
        for (size_t c = 0; c < cylCount; ++c)
        {
            auto get = [&](const std::vector<std::vector<double>>& vv) -> double {
                if (c >= vv.size()) return 0.0;
                if (i >= vv[c].size()) return 0.0;
                return vv[c][i];
            };

            const double totalT = (i < results.total_torque.size() ? results.total_torque[i] : 0.0);

            file << alpha[i] << ";" << (c+1)
                 << ";" << get(results.gas_force)
                 << ";" << get(results.inertia_force)
                 << ";" << get(results.total_force)
                 << ";" << get(results.rod_force)
                 << ";" << get(results.tangential_force)
                 << ";" << get(results.radial_force)
                 << ";" << get(results.cylinder_torque)
                 << ";" << totalT
                 << "\n";
        }
    }

    return true;
}

bool DynamicOutput::saveToFormattedText(const std::vector<double>& alpha,
                                       const DynamicResults& results,
                                       const EngineParams& params,
                                       const DynamicExportInfo& info,
                                       const std::string& filename)
{
    if (filename.empty()) return false;
    if (!canWriteToFile(filename)) return false;
    if (alpha.empty() || results.total_torque.empty()) return false;

    std::ofstream file(filename, std::ios::out | std::ios::binary);
if (!file.is_open()) return false;

writeBOM(file); // <-- важно для Windows, чтобы русский читался корректно

file << std::fixed << std::setprecision(6);
writeHeaderText(file, params, info);

    file << "СВОДКА\n";
    writeVectorStats(file, "Суммарный момент двигателя", results.total_torque, alpha, "Н*м");

    const size_t cylCount = results.cylinder_torque.size();
    for (size_t c = 0; c < cylCount; ++c)
    {
        if (c < results.cylinder_torque.size())
        {
            writeVectorStats(file,
                             "Момент цилиндра: цил " + std::to_string(c + 1),
                             results.cylinder_torque[c],
                             alpha,
                             "Н*М");
        }
    }

    file << "ТАБЛИЦА (первый цилиндр в таблице)\n";
    file << "альфа[град]  сила_газ[Н]  сила_инерц[Н]  сум_сила[Н]  танг_сила[Н]  рад_сила[Н]  момент_цил[Н*м]  сум_момент[Н*м]\n";

    const size_t n = alpha.size();
    for (size_t i = 0; i < n; ++i)
    {
        const double gas = (i < results.gas_force_cyl0.size() ? results.gas_force_cyl0[i] : 0.0);
        const double inr = (i < results.inertia_force_cyl0.size() ? results.inertia_force_cyl0[i] : 0.0);
        const double tot = (i < results.total_force_cyl0.size() ? results.total_force_cyl0[i] : 0.0);
        const double tan = (i < results.tangential_force_cyl0.size() ? results.tangential_force_cyl0[i] : 0.0);
        const double rad = (i < results.radial_force_cyl0.size() ? results.radial_force_cyl0[i] : 0.0);
        const double ctq = (i < results.cylinder_torque_cyl0.size() ? results.cylinder_torque_cyl0[i] : 0.0);
        const double ttq = (i < results.total_torque.size() ? results.total_torque[i] : 0.0);

        file << std::setw(10) << alpha[i] << "  "
             << std::setw(10) << gas << "  "
             << std::setw(12) << inr << "  "
             << std::setw(10) << tot << "  "
             << std::setw(10) << tan << "  "
             << std::setw(10) << rad << "  "
             << std::setw(16) << ctq << "  "
             << std::setw(16) << ttq << "\n";
    }

    return true;
}