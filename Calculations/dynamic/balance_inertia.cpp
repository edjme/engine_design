#include "Calculations/dynamic/balance_inertia.h"
#include <cmath>
#include <fstream>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#endif

namespace
{
    constexpr double PI = 3.1415926535897932384626433832795;

    static inline double area_piston(double D) { return PI * D * D / 4.0; }

    // c_lanch по ТЗ: Расстояниие между противовесами для Ланчестера
    //  - неполноопорный: c_lanch = web_n + depth_web + length_root_neck
    //  - полноопорный:   c_lanch = web_p + depth_web + length_root_neck
    static double calc_c_lanch(bool nonfull,
                               const CrankshaftMassResults &cm,
                               const Params &p,
                               std::string &note)
    {
        if (nonfull)
        {
            note = "c_lanch = web_n + depth_web + length_root_neck";
            return cm.web_n + p.depth_web + p.length_root_neck;
        }
        else
        {
            note = "c_lanch = web_p + depth_web + length_root_neck";
            return cm.web_p + p.depth_web + p.length_root_neck;
        }
    }

    static void save_csv(const std::string &path, const BalanceResults &R)
    {
        if (path.empty())
            return;
        std::ofstream f(path);
        if (!f)
            return;
        f << "ok,mode,message,"
             "Fp,M_pd,R,lambda,omega,tau,n_cyl,gamma_deg,"
             "m_root_reduce,m_rotating,"
             "axis_p,web_p,axis_n,web_n,c_lanch,c_lanch_note,"
             "S_prot1,S1_label,S_prot2,S2_label\n";
        f << (R.ok ? 1 : 0) << ","
          << "\"" << R.mode << "\","
          << "\"" << R.message << "\","
          << R.Fp << "," << R.M_pd << "," << R.R << "," << R.lambda << "," << R.omega << ","
          << R.tau << "," << R.n_cyl << "," << R.gamma_deg << ","
          << R.m_root_reduce << "," << R.m_rotating << ","
          << R.axis_p << "," << R.web_p << "," << R.axis_n << "," << R.web_n << ","
          << R.c_lanch << ",\"" << R.c_lanch_note << "\","
          << R.S_prot1 << ",\"" << R.S1_label << "\"," << R.S_prot2 << ",\"" << R.S2_label << "\"\n";
    }

    static void save_html(const std::string &path, const BalanceResults &R)
    {
        if (path.empty())
            return;
        std::ofstream f(path);
        if (!f)
            return;

        f <<
            R"(<!doctype html><html lang="ru"><head><meta charset="utf-8">
<title>Уравновешивание инерционных воздействий</title>
<style>
body{font-family:system-ui,-apple-system,Segoe UI,Roboto,Arial,sans-serif;margin:0;padding:16px;background:#0b0d0f;color:#e8e8e8;}
.card{max-width:1050px;margin:auto;background:#121417;border:1px solid #23262d;border-radius:12px;padding:16px}
h1{margin:0 0 12px 0;font-size:20px}
table{width:100%;border-collapse:collapse;margin-top:12px}
td,th{border:1px solid #23262d;padding:8px;font-size:13px}
th{background:#0e1114;text-align:left}
.note{color:#aab;font-size:12px}
.ok{color:#34d399}.err{color:#f87171}
</style></head><body>
<div class="card">
<h1>Уравновешивание инерционных воздействий</h1>
<div class="note">Схема: )"
          << R.mode << R"(</div>)";

        if (!R.message.empty())
            f << R"(<div class="note" style="color:#f87171">)" << R.message << "</div>\n";

        f <<
            R"(<table>
<tr><th>База</th><th>Значение</th></tr>
<tr><td>Fₚ = πD²/4</td><td>)"
          << R.Fp << R"( м²</td></tr>
<tr><td>M_pd = m_pd·Fₚ</td><td>)"
          << R.M_pd << R"( кг</td></tr>
<tr><td>r</td><td>)"
          << R.R << R"( м</td></tr>
<tr><td>λ</td><td>)"
          << R.lambda << R"(</td></tr>
<tr><td>ω</td><td>)"
          << R.omega << R"( рад/с</td></tr>
<tr><td>τ (тактность)</td><td>)"
          << R.tau << R"(</td></tr>
<tr><td>n_cyl</td><td>)"
          << R.n_cyl << R"(</td></tr>
<tr><td>γ (град)</td><td>)"
          << R.gamma_deg << R"(</td></tr>
<tr><td>m_root_reduce</td><td>)"
          << R.m_root_reduce << R"( кг/м²</td></tr>
<tr><td>m_rotating</td><td>)"
          << R.m_rotating << R"( кг/м²</td></tr>
</table>

<table>
<tr><th>Плечи/расстояния</th><th>Значение</th></tr>
<tr><td>axis_p</td><td>)"
          << R.axis_p << R"( м</td></tr>
<tr><td>web_p</td><td>)"
          << R.web_p << R"( м</td></tr>
<tr><td>axis_n</td><td>)"
          << R.axis_n << R"( м</td></tr>
<tr><td>web_n</td><td>)"
          << R.web_n << R"( м</td></tr>
<tr><td>c_lanch</td><td>)"
          << R.c_lanch << R"( м</td></tr>
<tr><td>формула c_lanch</td><td>)"
          << R.c_lanch_note << R"(</td></tr>
</table>

<table>
<tr><th>Результаты</th><th>Значение</th></tr>
<tr><td>S_prot1 — )"
          << R.S1_label << R"(</td><td>)" << R.S_prot1 << R"( кг·м</td></tr>
<tr><td>S_prot2 — )"
          << R.S2_label << R"(</td><td>)" << R.S_prot2 << R"( кг·м</td></tr>
<tr><td>Статус</td><td>)"
          << (R.ok ? "<span class='ok'>OK</span>" : "<span class='err'>Ошибка/не доступно</span>") << R"(</td></tr>
</table>
</div></body></html>)";
    }
} // namespace

BalanceResults build_balance_inertia(const Params &p,
                                     const CrankshaftMassResults &cm,
                                     const std::string &out_csv,
                                     const std::string &out_html,
                                     bool auto_open_html)
{
    BalanceResults R;

    // База
    R.Fp = area_piston(p.diam_cyl);
    R.M_pd = p.m_pd * R.Fp; // кг
    R.R = p.r;
    R.lambda = p.lyambda;
    R.omega = (p.w != 0.0 ? p.w : 2.0 * PI * p.n / 60.0);
    R.tau = static_cast<long>(std::llround(p.tau));
    R.n_cyl = static_cast<long>(std::llround(p.count_cyl));
    R.gamma_deg = p.gamma;

    // Инфо из блока масс (для отчёта)
    R.m_root_reduce = cm.m_root_reduce;
    R.m_rotating = cm.m_rotating;

    // Плечи из блока масс
    R.axis_p = cm.axis_p; // Для полноопорного расстояние между осями цилиндров
    R.web_p = cm.web_p;   // Для полноопорного расстояние между щеками
    R.axis_n = cm.axis_n; // Для неполноопорного расстояние между осями цилиндров
    R.web_n = cm.web_n;   // Для неполноопорного расстояние между щеками

    const bool nonfull = (std::lround(p.config_crankshaft) == 2); // nonfull - неполноопорный вал, иначе - полноопорный
    //---------Нахождение статических моментов для уравновешивания сил инерции 1го и 2го порядка и их моментов
    if (R.n_cyl == 1)
    {
        // ---------------- 1 цилиндр (стандартный) ----------------
        // Метод Ланчестера: уравновешивание сил.
        // S_prot1 = 0.5 * M_pd * r
        // S_prot2 = 0.125 * M_pd * λ * r
        R.S_prot1 = 0.5 * R.M_pd * R.R;
        R.S_prot2 = 0.125 * R.M_pd * R.lambda * R.R;
        R.S1_label = "от силы 1-го порядка";
        R.S2_label = "от силы 2-го порядка";
        R.mode = "1 цилиндр (силовая схема)";
        R.ok = true;
    }
    //--------------2 цилиндра --------------------------
    else if (R.n_cyl == 2)
    {
        //--------4 такта--------------
        if (R.tau == 4)
        {
            if (std::fabs(R.gamma_deg) < 1e-6) // Угол развала равен 0
            {
                // -------- 2 цилиндра, 4Т, γ=0° --------
                if (nonfull) // Проверка на неполноопорный вал
                {
                    R.ok = false;
                    R.mode = "2 цилиндра, 4Т, γ=0°";
                    R.message = "Нельзя использовать неполноопорный коленчатый вал.";
                }
                else
                {
                    // S_prot1 = M_pd * r
                    // S_prot2 = 0.25 * M_pd * λ * r
                    R.S_prot1 = R.M_pd * R.R;
                    R.S_prot2 = 0.25 * R.M_pd * R.lambda * R.R;
                    R.S1_label = "от силы 1-го порядка";
                    R.S2_label = "от силы 2-го порядка";
                    R.mode = "2 цилиндра, 4Т, γ=0° (силовая схема, только полноопорный)";
                    R.ok = true;
                }
            }
            else if (std::fabs(std::fabs(R.gamma_deg) - 180.0) < 1e-6) // Угол развала равен 180 (оппозитный двигатель)
            {
                // -------- 2 цилиндра, 4Т, γ=180° (оппозит), моментная схема --------
                R.c_lanch = calc_c_lanch(nonfull, cm, p, R.c_lanch_note); // Для моментов необходимо использовать расстояние между противовесами
                if (nonfull)                                              // Если неполноопорный
                {
                    // c_lanch уже посчитан по web_n
                    // S_prot1 = 0.5*M_pd * r * (axis_n) / c_lanch
                    // S_prot2 = 0.125 * M_pd * λ * r * (axis_n) / c_lanch
                    R.S_prot1 = 0.5 * (R.M_pd * R.R) * (R.axis_n / R.c_lanch);
                    R.S_prot2 = (0.125 * R.M_pd * R.lambda * R.R) * (R.axis_n / R.c_lanch);
                }
                else
                {
                    // полноопорный
                    // S_prot1 = 0.5*M_pd * r * (axis_p) / c_lanch
                    // S_prot2 = 0.125 * M_pd * λ * r * (axis_p) / c_lanch
                    R.S_prot1 = 0.5 * (R.M_pd * R.R) * (R.axis_p / R.c_lanch);
                    R.S_prot2 = (0.125 * R.M_pd * R.lambda * R.R) * (R.axis_p / R.c_lanch);
                }
                R.S1_label = "от момента 1-го порядка";
                R.S2_label = "от момента 2-го порядка";
                R.mode = std::string("2 цилиндра, 4Т, γ=180° (") + (nonfull ? "неполноопорный" : "полноопорный") + ", моментная схема)";
                R.ok = true;
            }
            else
            { // Для промежуточного угла развала пока недоступно
                R.ok = false;
                R.mode = "2 цилиндра, 4Т, γ≠0°,180°";
                R.message = "Пока недоступно.";
            }
        }
        else if (R.tau == 2)
        {                                      // для 2 тактов
            if (std::fabs(R.gamma_deg) < 1e-6) // угол развала 0
            {
                // -------- 2 цилиндра, 2Т, γ=0° --------
                R.c_lanch = calc_c_lanch(nonfull, cm, p, R.c_lanch_note);
                if (nonfull) // Неполноопорный вал
                {
                    // S_prot1 = M_pd * r * (axis_n) / (2 * c_lanch)
                    // S_prot2 = 0.125 * M_pd * λ * r
                    R.S_prot1 = (R.M_pd * R.R) * (R.axis_n / (2.0 * R.c_lanch));
                    R.S_prot2 = 0.125 * R.M_pd * R.lambda * R.R;
                }
                else // Полноопорный вал
                {
                    // S_prot1 = M_pd * r * (axis_p) / (2 * c_lanch)
                    // S_prot2 = 0.125 * M_pd * λ * r
                    R.S_prot1 = (R.M_pd * R.R) * (R.axis_p / (2.0 * R.c_lanch));
                    R.S_prot2 = 0.125 * R.M_pd * R.lambda * R.R;
                }
                R.S1_label = "от момента 1-го порядка";
                R.S2_label = "от силы 2-го порядка";
                R.mode = std::string("2 цилиндра, 2Т, γ=0° (") + (nonfull ? "неполноопорный" : "полноопорный") + ")";
                R.ok = true;
            }
            else
            { // Для 2х тактов пока досутпно только для угла развала 0
                R.ok = false;
                R.mode = "2 цилиндра, 2Т, γ≠0°";
                R.message = "Пока недоступно.";
            }
        }
        else // Другие тактности не считаем
        {
            R.ok = false;
            R.mode = "2 цилиндра, неизвестная тактность";
            R.message = "Пока недоступно.";
        }
    } // 3 цилиндра
    else if (R.n_cyl == 3)
    {
        R.ok = false;
        R.mode = "3 цилиндра";
        R.message = "Пока недоступно.";
    }
    else
    {
        R.ok = false;
        R.mode = "Конфигурация не реализована";
        R.message = "Добавьте нужную ветку алгоритма.";
    }

    // SUMMARY в консоль
    {
        std::ostringstream ss;
        ss << "=== Уравновешивание инерции ===\n"
           << R.mode << "\n"
           << "Fp=" << R.Fp << " м^2,  M_pd=" << R.M_pd << " кг,  r=" << R.R
           << " м,  λ=" << R.lambda << ",  ω=" << R.omega << " рад/с,  τ=" << R.tau
           << ",  n_cyl=" << R.n_cyl << ",  γ=" << R.gamma_deg << "°\n"
           << "Плечи (из масс): axis_p=" << R.axis_p << " м, web_p=" << R.web_p
           << " м, axis_n=" << R.axis_n << " м, web_n=" << R.web_n << " м\n";
        if (R.c_lanch > 0.0)
            ss << "c_lanch=" << R.c_lanch << " м (" << R.c_lanch_note << ")\n";
        ss << "S_prot1=" << R.S_prot1 << " кг·м — " << R.S1_label << "\n"
           << "S_prot2=" << R.S_prot2 << " кг·м — " << R.S2_label << "\n";
        if (!R.message.empty())
            ss << "Примечание: " << R.message << "\n";
        R.summary = ss.str();
    }

    // файлы
    save_csv(out_csv, R);
    save_html(out_html, R);
#ifdef _WIN32
    if (auto_open_html && !out_html.empty())
    {
        ShellExecuteA(nullptr, "open", out_html.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    }
#endif
    return R;
}
