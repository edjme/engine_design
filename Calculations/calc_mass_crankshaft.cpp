#include "calc_mass_crankshaft.h"
#include <cmath>
#include <fstream>
#include <iostream>

namespace
{
    constexpr double PI = 3.1415926535897932384626433832795;

    // -------------------- примитивы STL (в мм) --------------------
    struct V3
    {
        double x, y, z;
    };
    inline double mm(double m) { return m * 1000.0; }

    static void tri(std::ofstream &f, const V3 &n, const V3 &a, const V3 &b, const V3 &c)
    {
        f << "  facet normal " << n.x << " " << n.y << " " << n.z << "\n"
          << "    outer loop\n"
          << "      vertex " << a.x << " " << a.y << " " << a.z << "\n"
          << "      vertex " << b.x << " " << b.y << " " << b.z << "\n"
          << "      vertex " << c.x << " " << c.y << " " << c.z << "\n"
          << "    endloop\n"
          << "  endfacet\n";
    }
    static V3 nZp() { return {0, 0, 1}; }
    static V3 nZn() { return {0, 0, -1}; }
    static V3 nXp() { return {1, 0, 0}; }
    static V3 nXn() { return {-1, 0, 0}; }
    static V3 nYp() { return {0, 1, 0}; }
    static V3 nYn() { return {0, -1, 0}; }

    // цилиндр вдоль Z (радиус/высота/центр — в метрах)
    static void add_cyl_Z_mm(std::ofstream &f, double R_m, double h_m,
                             double cx_m, double cy_m, double z0_m, int seg)
    {
        const int N = std::max(3, seg);
        const double R = mm(R_m), h = mm(h_m);
        const double cx = mm(cx_m), cy = mm(cy_m), z0 = mm(z0_m);
        const double dA = 2.0 * PI / N;

        for (int i = 0; i < N; i++)
        {
            const double a0 = i * dA, a1 = (i + 1) * dA;
            V3 c0{cx, cy, z0}, c1{cx, cy, z0 + h};
            V3 p0{cx + R * std::cos(a0), cy + R * std::sin(a0), z0};
            V3 p1{cx + R * std::cos(a1), cy + R * std::sin(a1), z0};
            V3 q0{p0.x, p0.y, z0 + h}, q1{p1.x, p1.y, z0 + h};
            tri(f, nZn(), c0, p1, p0);
            tri(f, nZp(), c1, q0, q1);
            V3 n{std::cos((a0 + a1) * 0.5), std::sin((a0 + a1) * 0.5), 0.0};
            tri(f, n, p0, p1, q1);
            tri(f, n, p0, q1, q0);
        }
    }

    // призма-трапеция: по Y от y0 до y1, ширина по X линейно меняется от b до a; экструзия по Z
    static void add_trapezoid_prism_mm(std::ofstream &f,
                                       double y0_m, double y1_m, double halfX_bottom_m, double halfX_top_m,
                                       double z0_m, double z1_m)
    {
        const double xb = mm(halfX_bottom_m), xa = mm(halfX_top_m);
        const double y0 = mm(y0_m), y1 = mm(y1_m);
        const double z0 = mm(z0_m), z1 = mm(z1_m);

        // 8 вершин
        V3 A{-xb, y0, z0}, B{+xb, y0, z0}, C{+xa, y1, z0}, D{-xa, y1, z0};
        V3 E{-xb, y0, z1}, F{+xb, y0, z1}, G{+xa, y1, z1}, H{-xa, y1, z1};

        // нижняя и верхняя грани (Z константа)
        tri(f, nZn(), A, C, B);
        tri(f, nZn(), A, D, C);
        tri(f, nZp(), E, F, G);
        tri(f, nZp(), E, G, H);

        // боковые
        tri(f, nXn(), A, E, H);
        tri(f, nXn(), A, H, D); // X = -side
        tri(f, nXp(), B, C, G);
        tri(f, nXp(), B, G, F); // X = +side
        tri(f, nYn(), A, B, F);
        tri(f, nYn(), A, F, E); // Y = y0
        tri(f, nYp(), D, H, G);
        tri(f, nYp(), D, G, C); // Y = y1
    }

    // «капсула» с разными радиусами по краям (в плоскости Y–X), экструзия по Z
    static void add_tapered_capsule_YX_extrude_Z_mm(std::ofstream &f,
                                                    double y0_m, double y1_m, double halfX_bottom_m, double halfX_top_m,
                                                    double z0_m, double z1_m, int seg)
    {
        add_trapezoid_prism_mm(f, y0_m, y1_m, halfX_bottom_m, halfX_top_m, z0_m, z1_m);
        // округлые «торцы»
        add_cyl_Z_mm(f, halfX_bottom_m, (z1_m - z0_m), 0.0, y0_m, z0_m, seg); // внизу (Y=y0)
        add_cyl_Z_mm(f, halfX_top_m, (z1_m - z0_m), 0.0, y1_m, z0_m, seg);    // вверху (Y=y1)
    }
} // namespace

//==================== МАССА / ЦЕНТР ТЯЖЕСТИ (СИ) ====================
// Оси: Y — вверх (0 — ось коренной; R — ось шатунной), Z — вдоль коренной (ось), X — ширина.
CrankshaftMassResults calc_mass_crankshaft(const Params &p)
{
    const double rho = p.rho_material;          // кг/м^3
    const double R = p.r;                       // м, межосевое расстояние по Y
    const double Wz = p.depth_web;              // м, толщина (экструзия по Z)
    const double Rm = 0.5 * p.diam_root_neck;   // м
    const double Rr = 0.5 * p.diam_rod_neck;    // м
    const double Lm = 0.5 * p.length_root_neck; // м
    const double Lr = p.length_rod_neck;        // м
    const double rf = p.fillet_rad;             // м

    // требуемые полуширины щеки:
    // внизу (коренная): половина «ширины» = Rm + rf
    // вверху (шатунная): половина «ширины» = Rr + rf
    const double xb = Rm + rf; // halfX_bottom
    const double xa = Rr + rf; // halfX_top   (вверху уже — как просил)

    // Шейки (цилиндры вдоль Z)
    const double vol_root = PI * Rm * Rm * Lm;
    const double vol_rod = PI * Rr * Rr * Lr;
    const double m_root = rho * vol_root;
    const double y_root = 0.0;
    const double m_rod = rho * vol_rod;
    const double y_rod = R;

    // Площадь «трапеции» между Y=0 и Y=R с основаниями 2*xb и 2*xa
    const double A_trap = (xb + xa) * R; // = ((2xb+2xa)/2)*R
    // Центроид трапеции (от нижнего основания)
    const double y_trap = R * (2 * xb + xa) / (3 * (xb + xa));

    // Полуокружности по краям (по X-радиусам xb и xa) — их «2D-площади» лежат в плоскости Y–X,
    // центры по Y на 0 и R соответственно:
    const double A_bot_semi = 0.5 * PI * xb * xb;
    const double y_bot = 0.0;
    const double A_top_semi = 0.5 * PI * xa * xa;
    const double y_top = R;

    // Полная 2D-площадь поперечника щеки и её центроид по Y
    auto cheek_area = [&](bool half_right) -> std::pair<double, double>
    {
        if (half_right)
        {
            // полущёка (неполноопорный): трапеция + верхняя полуокружность
            const double A = A_trap + A_top_semi;
            const double Y = (A_trap * y_trap + A_top_semi * y_top) / A;
            return {A, Y};
        }
        else
        {
            // полная щека (полноопорный): трапеция + обе полуокружности
            const double A = A_trap + A_bot_semi + A_top_semi;
            const double Y = (A_trap * y_trap + A_bot_semi * y_bot + A_top_semi * y_top) / A;
            return {A, Y};
        }
    };

    // Объём «галтели» (аппрокс.) на одну щёку: 2 цилиндрических выреза вдоль Z
    const double V_fillet = PI * rf * rf * Wz;

    const bool nonfull = (std::lround(p.config_crankshaft) == 2);

    // Суммарная масса и момент по Y
    double Mtot = m_root + m_rod;
    double My = m_root * y_root + m_rod * y_rod;

    if (nonfull)
    {
        // Левая — полная щека
        {
            auto [A, Y] = cheek_area(/*half_right=*/false);
            const double V = A * Wz - 2.0 * V_fillet; // две галтели (к коренной и к шатунной)
            const double m = rho * V;
            Mtot += m;
            My += m * Y;
        }
        // Правая — ПОЛУЩЕКА (плоская грань по Y=0, закругление только сверху)
        {
            auto [A, Y] = cheek_area(/*half_right=*/true);
            const double V = A * Wz - 2.0 * V_fillet; // также 2 галтели
            const double m = rho * V;
            Mtot += m;
            My += m * Y;
        }
    }
    else
    {
        // Обе — полные щеки
        auto [A, Y] = cheek_area(/*half_right=*/false);
        const double V = A * Wz - 2.0 * V_fillet; // одна щека
        const double m = rho * V;
        Mtot += m + m;
        My += m * Y + m * Y;
    }

    const double total_mass = Mtot;
    const double y_cg = (Mtot > 0 ? My / Mtot : 0.0);

    // Контрольные расстояния по Z (как раньше)
    const double axis_p = p.length_root_neck + 2.0 * p.depth_web + p.length_rod_neck;
    const double web_p = p.length_root_neck + 3.0 * p.depth_web + 2.0 * p.length_rod_neck;
    const double axis_n = p.length_rod_neck + p.depth_web;
    const double web_n = 2.0 * p.depth_web + 2.0 * p.length_rod_neck;

    // Приведение к оси коренной (кг/м^2)
    const double Fp = PI * p.diam_cyl * p.diam_cyl / 4.0;
    const double m_root_reduce = (Fp > 0 && p.r > 0) ? (total_mass * y_cg / (p.r * Fp)) : 0.0;
    const double m_rotating = m_root_reduce + p.m_2;

    std::cout
        << "\n=== Контрольные расстояния (по Z) ===\n"
        << "Полноопорный:  axis_p=" << axis_p << "  web_p=" << web_p << "\n"
        << "Неполноопорный:axis_n=" << axis_n << "  web_n=" << web_n << "\n";
    std::cout << "Конфигурация: " << (nonfull ? "неполноопорный" : "полноопорный") << "\n";
    std::cout << "Масса колена: " << total_mass << " кг\n";
    std::cout << "y_CG (от оси коренной вверх): " << y_cg << " м\n";
    std::cout << "m_root_reduce: " << m_root_reduce << " кг/м^2;  m_rotating: " << m_rotating << " кг/м^2\n";

    return {total_mass, y_cg, m_root_reduce, m_rotating, axis_p, web_p, axis_n, web_n};
}

//==================== STL (мм) ====================
// Визуальная геометрия: трапеция по Y–X (внизу шире, вверху уже) + круги на Y=0 и Y=R.
bool export_crank_STL_mm(const Params &p, const std::string &stl_path, int seg)
{
    const bool nonfull = (std::lround(p.config_crankshaft) == 2);

    const double R = p.r;
    const double Rm = 0.5 * p.diam_root_neck;
    const double Rr = 0.5 * p.diam_rod_neck;
    const double Lm = 0.5 * p.length_root_neck;
    const double Lr = p.length_rod_neck;
    const double Wz = p.depth_web;
    const double xb = Rm + p.fillet_rad; // низ (коренная)
    const double xa = Rr + p.fillet_rad; // верх (шатунная)

    // Компоновка вдоль Z (слева→направо)
    const double z_rod0 = -0.5 * Lr;
    const double z_rod1 = +0.5 * Lr;
    const double z_webL0 = z_rod0 - Wz;
    const double z_webL1 = z_rod0;
    const double z_webR0 = z_rod1;
    const double z_webR1 = z_rod1 + Wz;
    const double z_mainL0 = z_webL0 - Lm;

    std::ofstream f(stl_path);
    if (!f)
        return false;

    f << "solid crank_mm\n";

    // Левая коренная (ось Z, центр Y=0)
    add_cyl_Z_mm(f, Rm, Lm, 0.0, 0.0, z_mainL0, seg);

    // Левая щека — полная (трапеция + оба «круга»)
    add_tapered_capsule_YX_extrude_Z_mm(f, 0.0, R, xb, xa, z_webL0, z_webL1, seg);

    // Шатунная (ось Z, центр Y=R)
    add_cyl_Z_mm(f, Rr, Lr, 0.0, R, z_rod0, seg);

    if (nonfull)
    {
        // Правая ПОЛУЩЕКА: трапеция + верхний «круг» (без нижнего)
        add_trapezoid_prism_mm(f, 0.0, R, xb, xa, z_webR0, z_webR1);
        add_cyl_Z_mm(f, xa, (z_webR1 - z_webR0), 0.0, R, z_webR0, seg);
        // правой коренной нет
    }
    else
    {
        // Правая полная щека и правая коренная
        add_tapered_capsule_YX_extrude_Z_mm(f, 0.0, R, xb, xa, z_webR0, z_webR1, seg);
        add_cyl_Z_mm(f, Rm, Lm, 0.0, 0.0, z_webR1, seg);
    }

    f << "endsolid crank_mm\n";
    return true;
}
