#include "input.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <algorithm>
#include <cctype>

namespace
{
    using Str = std::string;
    static const Str CSV_PATH = "input_data/input.csv";

    // ────────────────────────── ВСПОМОГАТЕЛЬНЫЕ ──────────────────────────

    // trim по месту
    inline void ltrim(Str &s)
    {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch)
                                        { return !std::isspace(ch); }));
    }
    inline void rtrim(Str &s)
    {
        s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch)
                             { return !std::isspace(ch); })
                    .base(),
                s.end());
    }
    inline void trim(Str &s)
    {
        ltrim(s);
        rtrim(s);
    }

    inline bool starts_with(const Str &s, const Str &p)
    {
        return s.size() >= p.size() && std::equal(p.begin(), p.end(), s.begin());
    }

    // Безопасное извлечение double из строки (локаль по умолчанию — точка)
    inline bool parse_double(const Str &t, double &out)
    {
        try
        {
            size_t idx = 0;
            out = std::stod(t, &idx);
            return idx > 0;
        }
        catch (...)
        {
            return false;
        }
    }

    // ────────────────────────── ДЕФОЛТНЫЙ CSV ──────────────────────────
    // Создаёт файл с дефолтными значениями, если он отсутствует.
    void createDefaultCSV()
    {
        std::ofstream out(CSV_PATH);
        out << "param,value\n";
        out << "diam_cyl,0.12\n";         // диаметр цилиндра, м
        out << "stroke,0.1\n";            // ход поршня, м
        out << "epsilent,12\n";           // степень сжатия ε
        out << "p_a,90000\n";             // давление на впуске, Па
        out << "p_r,110000\n";            // давление на выпуске, Па
        out << "n_1,1.38\n";              // показатель политропы сжатия
        out << "n_2,1.22\n";              // показатель политропы расширения
        out << "lymbda_z,2\n";            // степень повышения давления (Pz/Pc)
        out << "ro,1.35\n";               // степень предварительного расширения (Vz'/Vz)
        out << "lyambda,0.3333333\n";     // геом. характеристика КШМ λ = R/L
        out << "n,3700\n";                // частота вращения, об/мин
        out << "m_pd,120\n";              // масса поступ. частей, кг/м^2
        out << "r,0.05\n";                // радиус кривошипа, м
        out << "leng_rod,0.167\n";        // длина шатуна, м
        out << "m_rod,120\n";             // масса шатуна (к Fп), кг/м^2
        out << "m_2,80\n";                // масса шатуна приведённая к оси шейки (к Fп), кг/м^2
        out << "w,387.463\n";             // угловая скорость, рад/с (если в CSV будет 0 — возьмём из n)
        out << "tau,4\n";                 // тактность (2 или 4)
        out << "count_cyl,2\n";           // число цилиндров
        out << "gamma,0\n";               // угол развала
        out << "diam_root_neck,0.09\n";   // диаметр коренной шейки, м
        out << "diam_rod_neck,0.075\n";   // диаметр шатунной шейки, м
        out << "length_rod_neck,0.04\n";  // длина шатунной шейки, м
        out << "length_root_neck,0.06\n"; // длина коренной шейки, м
        out << "depth_web,0.04\n";        // толщина щеки (по Z), м
        out << "fillet_rad,0.004\n";      // радиус галтели, м
        out << "width_web,0.098\n";       // ширина щеки (по Y), м
        out << "dist_axes,0.179\n";       // межосевое расстояние, м (при необходимости)
        out << "dist_web,0.26525\n";      // расстояние между щеками, м (при необходимости)
        out << "rho_material,7810\n";     // плотность материала, кг/м^3
        out << "config_crankshaft,1\n";   // 1 — полноопорный, 2 — неполноопорный
        out << "config_prot,1\n";         // 1 — V1, 2 — V2, 3 — SemiSupport
        out << "r_prot1,0.049\n";         // внутренний радиус противовеса, м
        out << "r_prot2,0.091\n";         // внешний радиус противовеса, м
        out << "depth_prot,0.04\n";       // толщина противовеса (по Z), м
        out.close();
        std::cout << "Создан файл " << CSV_PATH << " с дефолтными значениями.\n";
    }

    // ────────────────────────── ЗАГРУЗКА CSV ──────────────────────────
    // Читает файл вида "param,value"; игнорирует пустые строки и комментарии (#, //).
    std::map<Str, double> loadCSVToMap()
    {
        std::ifstream file(CSV_PATH);
        if (!file.is_open())
        {
            std::cerr << "⚠ CSV-файл не найден. Создаю новый с дефолтными значениями...\n";
            createDefaultCSV();
            file.open(CSV_PATH);
        }

        std::map<Str, double> values;
        Str line;

        // попытаться пропустить заголовок
        if (std::getline(file, line))
        {
            Str head = line;
            trim(head);
            if (!(starts_with(head, "param") || starts_with(head, "key")))
            {
                // первая строка могла быть уже данными — разберём её
                file.clear();
                file.seekg(0, std::ios::beg);
            }
        }
        else
        {
            // пустой файл — перезапишем дефолтами
            file.close();
            createDefaultCSV();
            file.open(CSV_PATH);
            std::getline(file, line); // пропустить заголовок
        }

        while (std::getline(file, line))
        {
            Str raw = line;
            // отрезаем комментарии
            auto pos_sharp = raw.find('#');
            if (pos_sharp != Str::npos)
                raw.erase(pos_sharp);
            auto pos_dbl = raw.find("//");
            if (pos_dbl != Str::npos)
                raw.erase(pos_dbl);

            trim(raw);
            if (raw.empty())
                continue;

            // разбор "key,value"
            Str key, valStr;
            std::stringstream ss(raw);
            if (!std::getline(ss, key, ','))
                continue;
            if (!std::getline(ss, valStr))
                continue;
            trim(key);
            trim(valStr);

            double val = 0.0;
            if (!parse_double(valStr, val))
            {
                std::cerr << "⚠ Не удалось прочитать число для параметра '" << key << "': '" << valStr << "'\n";
                continue;
            }
            values[key] = val;
        }
        file.close();
        return values;
    }

    // Достаёт значение из map с fallback-ом на дефолт.
    // Заодно логируем, если параметр отсутствует в CSV.
    double getOrDefault(const std::map<Str, double> &m, const Str &k, double def)
    {
        auto it = m.find(k);
        if (it != m.end())
            return it->second;
        std::cerr << "ℹ Параметр '" << k << "' отсутствует в CSV — берём дефолт: " << def << "\n";
        return def;
    }

    constexpr double PI = 3.1415926535897932384626433832795;
} // namespace

// ────────────────────────── ПУБЛИЧНОЕ API ──────────────────────────
Params input()
{
    const auto values = loadCSVToMap();
    Params p;

    // Геометрия цилиндра
    p.diam_cyl = getOrDefault(values, "diam_cyl", 0.11);
    p.stroke = getOrDefault(values, "stroke", 0.12);
    p.epsilent = getOrDefault(values, "epsilent", 12.0);

    // Давления
    p.p_a = getOrDefault(values, "p_a", 90000.0);
    p.p_r = getOrDefault(values, "p_r", 110000.0);

    // Политропы и диаграмма
    p.n_1 = getOrDefault(values, "n_1", 1.38);
    p.n_2 = getOrDefault(values, "n_2", 1.22);
    p.lymbda_z = getOrDefault(values, "lymbda_z", 2.0);
    p.ro = getOrDefault(values, "ro", 1.35);

    // КШМ геометрия
    p.lyambda = getOrDefault(values, "lyambda", 0.3333333);
    p.r = getOrDefault(values, "r", 0.06);
    p.leng_rod = getOrDefault(values, "leng_rod", 0.18);

    // Массы (удельные)
    p.m_pd = getOrDefault(values, "m_pd", 110.0);
    p.m_rod = getOrDefault(values, "m_rod", 110.0);
    p.m_2 = getOrDefault(values, "m_2", 73.33);

    // Режим
    p.n = getOrDefault(values, "n", 3900.0);
    // Если w в CSV отсутствует — вычислим из n (рад/с). Если есть, используем как есть.
    if (values.find("w") != values.end())
    {
        p.w = values.at("w");
    }
    else
    {
        p.w = 2.0 * PI * p.n / 60.0;
        std::cerr << "ℹ Параметр 'w' отсутствует — вычислен из n: w = " << p.w << " рад/с\n";
    }
    p.tau = getOrDefault(values, "tau", 4.0);
    p.count_cyl = getOrDefault(values, "count_cyl", 2.0);
    p.gamma = getOrDefault(values, "gamma", 0.0);

    // Вал: шейки/щёки
    p.diam_root_neck = getOrDefault(values, "diam_root_neck", 0.07997);
    p.diam_rod_neck = getOrDefault(values, "diam_rod_neck", 0.0715);
    p.length_rod_neck = getOrDefault(values, "length_rod_neck", 0.036465);
    p.length_root_neck = getOrDefault(values, "length_root_neck", 0.05278);
    p.depth_web = getOrDefault(values, "depth_web", 0.044);
    p.fillet_rad = getOrDefault(values, "fillet_rad", 0.00352);
    p.width_web = getOrDefault(values, "width_web", 0.08701);
    p.dist_axes = getOrDefault(values, "dist_axes", 0.17725);
    p.dist_web = getOrDefault(values, "dist_web", 0.26525);

    // Материал и конфигурация
    p.rho_material = getOrDefault(values, "rho_material", 7850.0);
    p.config_crankshaft = getOrDefault(values, "config_crankshaft", 1.0);
    p.config_prot = getOrDefault(values, "config_prot", 1.0);

    // Противовесы
    p.depth_prot = getOrDefault(values, "depth_prot", 0.044);
    p.r_prot1 = getOrDefault(values, "r_prot1", 0.043505);
    p.r_prot2 = getOrDefault(values, "r_prot2", 0.103505);

    // Базовые проверки (не жёсткие, только предупреждения)
    if (p.diam_cyl <= 0 || p.stroke <= 0)
        std::cerr << "⚠ diam_cyl/stroke выглядят некорректно (<=0)\n";
    if (p.epsilent <= 1.0)
        std::cerr << "⚠ epsilent <= 1 — степень сжатия должна быть > 1\n";
    if (p.r <= 0 || p.leng_rod <= 0)
        std::cerr << "⚠ r/leng_rod выглядят некорректно (<=0)\n";
    if (p.r_prot2 <= p.r_prot1)
        std::cerr << "⚠ r_prot2 <= r_prot1 — внешний радиус должен быть больше внутреннего\n";

    return p;
}
