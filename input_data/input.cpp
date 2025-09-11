#include "input.h"

#include <fstream>
#include <sstream>
#include <locale>
#include <iomanip>
#include <stdexcept>

namespace kinio
{

    namespace
    {

        // Определяем, является ли path папкой.
        static bool IsDirectoryPath(const std::filesystem::path &p)
        {
            if (p.empty())
                return true;
            if (std::filesystem::exists(p))
                return std::filesystem::is_directory(p);
            // Если не существует — эвристика: есть ли расширение у последнего элемента?
            return p.has_filename() && !p.has_extension();
        }

        // Возвращает итоговый полный путь к файлу CSV.
        static std::filesystem::path ResolveCSVPath(const std::filesystem::path &path,
                                                    const Defaults &def)
        {
            if (path.empty())
            {
                return std::filesystem::current_path() / def.filename;
            }
            if (IsDirectoryPath(path))
            {
                std::filesystem::path dir = path.empty() ? std::filesystem::current_path() : path;
                return dir / def.filename;
            }
            return path; // уже полный путь до файла
        }

        // Пишем строку CSV с гарантированной "C" локалью и десятичной точкой.
        template <typename... Ts>
        void WriteCSVRow(std::ofstream &ofs, const Ts &...fields)
        {
            std::ostringstream line;
            line.imbue(std::locale::classic());
            bool first = true;
            auto writeField = [&](const auto &v)
            {
                if (!first)
                    line << ',';
                first = false;
                // Для строк — как есть; для чисел — с надёжным форматом
                if constexpr (std::is_arithmetic_v<std::decay_t<decltype(v)>>)
                {
                    // Без научной нотации, достаточная точность
                    line << std::setprecision(12) << std::fixed << v;
                }
                else
                {
                    // Экранирование запятых при необходимости (наши идентификаторы без запятых)
                    line << v;
                }
            };
            (writeField(fields), ...);
            ofs << line.str() << "\n";
        }

    } // namespace

    std::vector<double> MakeAlphaGrid(double start_deg, double stop_deg, double step_deg)
    {
        if (step_deg <= 0.0)
            throw std::invalid_argument("alpha_step must be > 0");
        if (stop_deg < start_deg)
            std::swap(start_deg, stop_deg);
        std::vector<double> a;
        double x = start_deg;
        // Защита от накопления ошибки: считаем количество шагов заранее
        const double span = stop_deg - start_deg;
        const std::size_t n_steps = static_cast<std::size_t>(span / step_deg + 0.5);
        a.reserve(n_steps + 1);
        for (std::size_t i = 0;; ++i)
        {
            double val = start_deg + i * step_deg;
            if (val > stop_deg + 1e-9)
                break;
            if (val > stop_deg && val < stop_deg + 1e-9)
                val = stop_deg; // выровнять последний
            a.push_back(val);
            if (val >= stop_deg - 1e-12)
                break;
        }
        return a;
    }

    std::filesystem::path WriteDefaultInputCSV(const std::filesystem::path &path,
                                               const Defaults &def)
    {
        Defaults d = def;
        if (d.alpha_stop <= 0.0)
            d.alpha_stop = static_cast<double>(d.cycle_deg);

        const auto full = ResolveCSVPath(path, d);
        const auto parent = full.parent_path();
        if (!parent.empty() && !std::filesystem::exists(parent))
        {
            std::filesystem::create_directories(parent);
        }

        std::ofstream ofs(full, std::ios::binary);
        if (!ofs)
            throw std::runtime_error("cannot open file for write: " + full.string());

        ofs.imbue(std::locale::classic());

        // Подготовим сетку alpha
        const auto alpha = MakeAlphaGrid(d.alpha_start, d.alpha_stop, d.alpha_step);

        // --------- Формат CSV (матрица) ----------
        // Строка 0: маяки в (0;0) и (0;1). Колонки ≥2 оставляем пустыми на входе.
        // Скалярные параметры кладём как пары строк: [метка], [значение] в колонке 0.
        //
        // r0:  "diam_cyl", "alpha"
        WriteCSVRow(ofs, "diam_cyl", "alpha");
        // r1:   value(diam_cyl), alpha[0]
        WriteCSVRow(ofs, d.diam_cyl, alpha.empty() ? 0.0 : alpha.front());

        // Далее: сетка alpha продолжается вниз по колонке 1 (строки 2..N)
        for (std::size_t i = 1; i < alpha.size(); ++i)
        {
            // кол0 пуст, кол1 = alpha[i]
            WriteCSVRow(ofs, "", alpha[i]);
        }

        // Теперь продолжаем внизу добавлять пары [метка],[значение] в колонке 0.
        // Порядок и "координаты-маяки" согласованы: stroke на следующей чётной строке первого столбца и т.д.
        WriteCSVRow(ofs, "stroke", "");
        WriteCSVRow(ofs, d.stroke, "");

        WriteCSVRow(ofs, "conrod_len", "");
        WriteCSVRow(ofs, d.conrod_len, "");

        WriteCSVRow(ofs, "pin_offset", "");
        WriteCSVRow(ofs, d.pin_offset, "");

        WriteCSVRow(ofs, "rpm", "");
        WriteCSVRow(ofs, d.rpm, "");

        WriteCSVRow(ofs, "cycle_deg", "");
        WriteCSVRow(ofs, d.cycle_deg, "");

        WriteCSVRow(ofs, "phase_tdc", "");
        WriteCSVRow(ofs, d.phase_tdc, "");

        WriteCSVRow(ofs, "direction", "");
        WriteCSVRow(ofs, d.direction, "");

        WriteCSVRow(ofs, "mass_piston", "");
        WriteCSVRow(ofs, d.mass_piston, "");

        WriteCSVRow(ofs, "mass_conrod", "");
        WriteCSVRow(ofs, d.mass_conrod, "");

        // L/R (lgr) можно сразу положить рассчитанным по текущим дефолтам
        const double R = d.stroke * 0.5;
        const double lgr = (R > 0.0) ? (d.conrod_len / R) : 0.0;
        WriteCSVRow(ofs, "lgr", "");
        WriteCSVRow(ofs, lgr, "");

        ofs.flush();
        return full;
    }

    bool EnsureDefaultInputCSV(const std::filesystem::path &path)
    {
        Defaults d;
        const auto full = ResolveCSVPath(path, d);
        if (std::filesystem::exists(full))
            return false; // уже есть — ничего не делаем
        WriteDefaultInputCSV(path, d);
        return true;
    }

} // namespace kinio
