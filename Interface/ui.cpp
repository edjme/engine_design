#include "Interface/ui.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <map>
#include <sstream>
#ifdef _WIN32
#include <windows.h>
#endif
#include <cstdio>
#include <cstdlib>
#include <cmath>
using std::llround;

static void open_file(const std::string &path)
{
#ifdef _WIN32
    ShellExecuteA(nullptr, "open", path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
#else
#ifdef __APPLE__
    std::string cmd = "open \"" + path + "\"";
#else
    std::string cmd = "xdg-open \"" + path + "\"";
#endif
    std::system(cmd.c_str());
#endif
}

static void save_params_to_csv(const Params &p, const std::string &csv = "input_data/input.csv")
{
    std::ofstream out(csv);
    if (!out)
    {
        std::cerr << "Не удалось записать " << csv << "\n";
        return;
    }
    out << "param,value\n";
    out << std::fixed << std::setprecision(8);
    out << "diam_cyl," << p.diam_cyl << "\n";
    out << "stroke," << p.stroke << "\n";
    out << "epsilent," << p.epsilent << "\n";
    out << "p_a," << p.p_a << "\n";
    out << "p_r," << p.p_r << "\n";
    out << "n_1," << p.n_1 << "\n";
    out << "n_2," << p.n_2 << "\n";
    out << "lymbda_z," << p.lymbda_z << "\n";
    out << "ro," << p.ro << "\n";
    out << "lyambda," << p.lyambda << "\n";
    out << "n," << p.n << "\n";
    out << "m_pd," << p.m_pd << "\n";
    out << "r," << p.r << "\n";
    out << "leng_rod," << p.leng_rod << "\n";
    out << "m_rod," << p.m_rod << "\n";
    out << "m_2," << p.m_2 << "\n";
    out << "w," << p.w << "\n";
    out << "tau," << p.tau << "\n";
    out << "count_cyl," << p.count_cyl << "\n";
    out << "gamma," << p.gamma << "\n";
    out << "diam_root_neck," << p.diam_root_neck << "\n";
    out << "diam_rod_neck," << p.diam_rod_neck << "\n";
    out << "length_rod_neck," << p.length_rod_neck << "\n";
    out << "length_root_neck," << p.length_root_neck << "\n";
    out << "depth_web," << p.depth_web << "\n";
    out << "fillet_rad," << p.fillet_rad << "\n";
    out << "width_web," << p.width_web << "\n";
    out << "dist_axes," << p.dist_axes << "\n";
    out << "dist_web," << p.dist_web << "\n";
    out << "rho_material," << p.rho_material << "\n";
    out << "config_crankshaft," << p.config_crankshaft << "\n";
    out << "config_prot," << p.config_prot << "\n";
    out << "depth_prot," << p.depth_prot << "\n";
    out << "r_prot1," << p.r_prot1 << "\n";
    out << "r_prot2," << p.r_prot2 << "\n";
}

static double read_or_keep(const std::string &prompt, double cur)
{
    std::cout << prompt << " [" << std::setprecision(8) << cur << "]: ";
    std::string s;
    std::getline(std::cin, s);
    if (s.empty())
        return cur;
    try
    {
        return std::stod(s);
    }
    catch (...)
    {
        std::cout << "  (некорректно, оставляю текущее)\n";
        return cur;
    }
}

PipelineResults run_full_pipeline(const Params &p, bool auto_open_html)
{
    PipelineResults res; // файлы — в дефолтах
    // 1) индикаторная
    res.ind = build_indicator_PV(p, 400, res.files.pv_csv, res.files.pv_html, auto_open_html);
    std::cout << res.ind.summary << "\n";
    // 2) P(α)
    res.pal = build_P_alpha(p, res.ind, 0.5, res.files.pa_csv, res.files.pa_html, auto_open_html);
    std::cout << res.pal.summary << "\n";
    // 3) Силы КШМ
    res.fr = build_forces_ksm(p, res.pal, res.files.f_csv, res.files.f_html, auto_open_html);
    std::cout << res.fr.summary << "\n";
    // 4) ВДС шатунной
    res.vds = build_vds_crankpin(p, res.fr, res.files.vds_csv, res.files.vds_html, auto_open_html);
    std::cout << res.vds.summary << "\n";
    // 5) масса/ЦТ колена
    res.cm = calc_mass_crankshaft(p);
    std::cout << "Масса колена=" << res.cm.total_mass << " кг, y_CG=" << res.cm.y_cg
              << " м (от оси коренной), m_root_reduce=" << res.cm.m_root_reduce
              << " кг/м^2, m_rotating=" << res.cm.m_rotating << " кг/м^2\n";
    // 6) STL колена
    if (export_crank_STL_mm(p, res.files.crank_stl, 128))
        std::cout << "STL (колено): " << res.files.crank_stl << "\n";
    // 7) Противовесы и STL
    CWVariant var = CWVariant::FullSupport_V1;
    const long vsel = (long)std::llround(p.config_prot);
    if (vsel == 2)
        var = CWVariant::FullSupport_V2;
    else if (vsel == 3)
        var = CWVariant::SemiSupport;
    res.cw = build_counterweights_and_export(p, res.cm, var, res.files.crank_cw_stl, 128, true);
    std::cout << "S_prot=" << res.cw.S_prot << " кг·м, alpha=" << res.cw.alpha_deg << "°  (" << res.cw.message << ")\n";
    return res;
}

static void menu_inputs()
{
    Params p = input();
    std::cout << "\n— Редактирование входных данных —\n(Enter — оставить текущее)\n";
    p.diam_cyl = read_or_keep("diam_cyl (м)", p.diam_cyl);
    p.stroke = read_or_keep("stroke (м)", p.stroke);
    p.epsilent = read_or_keep("epsilent", p.epsilent);
    p.p_a = read_or_keep("p_a (Па)", p.p_a);
    p.p_r = read_or_keep("p_r (Па)", p.p_r);
    p.n_1 = read_or_keep("n_1", p.n_1);
    p.n_2 = read_or_keep("n_2", p.n_2);
    p.lymbda_z = read_or_keep("lymbda_z", p.lymbda_z);
    p.ro = read_or_keep("ro", p.ro);
    p.lyambda = read_or_keep("lyambda", p.lyambda);
    p.n = read_or_keep("n (об/мин)", p.n);
    p.m_pd = read_or_keep("m_pd (кг/м^2)", p.m_pd);
    p.r = read_or_keep("r (м)", p.r);
    p.leng_rod = read_or_keep("leng_rod (м)", p.leng_rod);
    p.m_rod = read_or_keep("m_rod (кг/м^2)", p.m_rod);
    p.m_2 = read_or_keep("m_2 (кг/м^2)", p.m_2);
    p.w = read_or_keep("w (рад/с)", p.w);
    p.tau = read_or_keep("tau (2 или 4)", p.tau);
    p.count_cyl = read_or_keep("count_cyl", p.count_cyl);
    p.gamma = read_or_keep("gamma (град)", p.gamma);
    p.diam_root_neck = read_or_keep("diam_root_neck (м)", p.diam_root_neck);
    p.diam_rod_neck = read_or_keep("diam_rod_neck (м)", p.diam_rod_neck);
    p.length_rod_neck = read_or_keep("length_rod_neck (м)", p.length_rod_neck);
    p.length_root_neck = read_or_keep("length_root_neck (м)", p.length_root_neck);
    p.depth_web = read_or_keep("depth_web (м)", p.depth_web);
    p.fillet_rad = read_or_keep("fillet_rad (м)", p.fillet_rad);
    p.width_web = read_or_keep("width_web (м)", p.width_web);
    p.dist_axes = read_or_keep("dist_axes (м)", p.dist_axes);
    p.dist_web = read_or_keep("dist_web (м)", p.dist_web);
    p.rho_material = read_or_keep("rho_material (кг/м^3)", p.rho_material);
    p.config_crankshaft = read_or_keep("config_crankshaft (1 полноопорный, 2 неполнооп.)", p.config_crankshaft);
    p.config_prot = read_or_keep("config_prot (1 FS_V1, 2 FS_V2, 3 Semi)", p.config_prot);
    p.depth_prot = read_or_keep("depth_prot (м)", p.depth_prot);
    p.r_prot1 = read_or_keep("r_prot1 (м)", p.r_prot1);
    p.r_prot2 = read_or_keep("r_prot2 (м)", p.r_prot2);

    save_params_to_csv(p);
    std::cout << "Сохранено в input_data/input.csv\n";
}

static void menu_outputs(const RunArtifacts &f)
{
    while (true)
    {
        std::cout << "\n— Вывод —\n"
                     " 1) Показать индикаторную диаграмму\n"
                     " 2) Показать развёртку P(α)\n"
                     " 3) Показать силы в КШМ\n"
                     " 4) Показать ВДС шатунной шейки\n"
                     " 5) Показать 3D колена (STL)\n"
                     " 6) Показать 3D колена с противовесами (STL)\n"
                     " 7) Назад\n> ";
        std::string c;
        std::getline(std::cin, c);
        if (c == "1")
            open_file(f.pv_html);
        else if (c == "2")
            open_file(f.pa_html);
        else if (c == "3")
            open_file(f.f_html);
        else if (c == "4")
            open_file(f.vds_html);
        else if (c == "5")
            open_file(f.crank_stl);
        else if (c == "6")
            open_file(f.crank_cw_stl);
        else if (c == "7" || c.empty())
            break;
    }
}

void run_cli()
{
#ifdef _WIN32
    system("chcp 65001 > nul");
#endif
    PipelineResults last{};
    bool have_results = false;

    while (true)
    {
        std::cout << "\n================ ENGINE DESIGN ================\n"
                     "1) Входные данные (просмотр/редактирование)\n"
                     "2) Расчёт (выполнить все этапы)\n"
                     "3) Вывод (открыть графики/модели)\n"
                     "4) Печать S_prot и пояснений\n"
                     "0) Выход\n> ";
        std::string s;
        std::getline(std::cin, s);
        if (s == "1")
        {
            menu_inputs();
        }
        else if (s == "2")
        {
            Params p = input();
            last = run_full_pipeline(p, /*auto_open_html*/ true);
            have_results = true;
        }
        else if (s == "3")
        {
            if (!have_results)
            {
                std::cout << "Сначала выполните расчёт (п.2)\n";
                continue;
            }
            menu_outputs(last.files);
        }
        else if (s == "4")
        {
            if (!have_results)
            {
                std::cout << "Сначала выполните расчёт (п.2)\n";
                continue;
            }
            std::cout << "\n— Противовесы —\n";
            std::cout << "Статический момент одного противовеса S_prot = "
                      << last.cw.S_prot << " кг·м\n";
            std::cout << "Угол сектора одного противовеса alpha = "
                      << last.cw.alpha_deg << " °\n";
            std::cout << "(формулы: см. модуль counterweights; используются m_rotating="
                      << last.cm.m_rotating << ", Fp=πD²/4, а также осевые расстояния "
                      << "[axis_p,web_p,axis_n,web_n] = ["
                      << last.cm.axis_p << "," << last.cm.web_p << ","
                      << last.cm.axis_n << "," << last.cm.web_n << "])\n";
            if (!last.cw.message.empty())
                std::cout << last.cw.message << "\n";
        }
        else if (s == "0")
        {
            break;
        }
    }
}
