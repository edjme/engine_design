#include "cw_counterweights.h"
#include <cmath>
#include <fstream>
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// -------------------- простые утилиты STL --------------------
struct V3
{
    double x, y, z;
};

static inline double mm(double m) { return m * 1000.0; }

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

// цилиндр вдоль Z (радиус/высота/центр в МЕТРАХ)
static void add_cyl_Z_mm(std::ofstream &f, double R_m, double h_m,
                         double cx_m, double cy_m, double z0_m, int seg)
{
    const double R = mm(R_m);
    const double h = mm(h_m);
    const double cx = mm(cx_m), cy = mm(cy_m);
    const double z0 = mm(z0_m);

    const double d = 2.0 * M_PI / seg;
    for (int i = 0; i < seg; i++)
    {
        const double a0 = i * d, a1 = (i + 1) * d;
        const V3 c0{cx, cy, z0}, c1{cx, cy, z0 + h};
        const V3 p0{cx + R * std::cos(a0), cy + R * std::sin(a0), z0};
        const V3 p1{cx + R * std::cos(a1), cy + R * std::sin(a1), z0};
        const V3 q0{p0.x, p0.y, z0 + h}, q1{p1.x, p1.y, z0 + h};
        tri(f, nZn(), c0, p1, p0);
        tri(f, nZp(), c1, q0, q1);
        const V3 n{std::cos((a0 + a1) * 0.5), std::sin((a0 + a1) * 0.5), 0.0};
        tri(f, n, p0, p1, q1);
        tri(f, n, p0, q1, q0);
    }
}

static void add_block_mm(std::ofstream &f,
                         double x0_m, double x1_m, double y0_m, double y1_m, double z0_m, double z1_m)
{
    V3 A{mm(x0_m), mm(y0_m), mm(z0_m)}, B{mm(x1_m), mm(y0_m), mm(z0_m)},
        C{mm(x1_m), mm(y1_m), mm(z0_m)}, D{mm(x0_m), mm(y1_m), mm(z0_m)};
    V3 E{mm(x0_m), mm(y0_m), mm(z1_m)}, G{mm(x1_m), mm(y0_m), mm(z1_m)},
        H{mm(x1_m), mm(y1_m), mm(z1_m)}, K{mm(x0_m), mm(y1_m), mm(z1_m)};
    tri(f, nZn(), A, C, B);
    tri(f, nZn(), A, D, C);
    tri(f, nZp(), E, G, H);
    tri(f, nZp(), E, H, K);
    tri(f, nXn(), A, E, K);
    tri(f, nXn(), A, K, D);
    tri(f, nXp(), B, C, H);
    tri(f, nXp(), B, H, G);
    tri(f, nYn(), A, B, G);
    tri(f, nYn(), A, G, E);
    tri(f, nYp(), D, K, H);
    tri(f, nYp(), D, H, C);
}

// «стадион» щеки (прямоугольник + 2 полуокружности), экструзия по Z
static void add_stadium_extrude_Z_mm(std::ofstream &f,
                                     double x0_m, double x1_m, double y_half_m,
                                     double z0_m, double z1_m, int seg)
{
    add_block_mm(f, x0_m, x1_m, -y_half_m, +y_half_m, z0_m, z1_m);
    add_cyl_Z_mm(f, y_half_m, (z1_m - z0_m), x0_m, 0.0, z0_m, seg);
    add_cyl_Z_mm(f, y_half_m, (z1_m - z0_m), x1_m, 0.0, z0_m, seg);
}

// кольцевой сектор (толстая «луна») вдоль Z, аргументы в МЕТРАХ
static void add_sector_ring_Z_mm(std::ofstream &f,
                                 double r1_m, double r2_m,
                                 double phi0, double phi1, // углы в радианах
                                 double cx_m, double cy_m,
                                 double z0_m, double z1_m,
                                 int seg)
{
    // дискретизируем по углу
    const int n = std::max(3, (int)std::ceil(seg * std::fabs(phi1 - phi0) / (2 * M_PI)));
    const double d = (phi1 - phi0) / n;
    const double r1 = mm(r1_m), r2 = mm(r2_m);
    const double cx = mm(cx_m), cy = mm(cy_m);
    const double z0 = mm(z0_m), z1 = mm(z1_m);

    for (int i = 0; i < n; i++)
    {
        const double a0 = phi0 + i * d;
        const double a1 = phi0 + (i + 1) * d;

        // нижняя грань
        V3 A{cx + r1 * std::cos(a0), cy + r1 * std::sin(a0), z0};
        V3 B{cx + r2 * std::cos(a0), cy + r2 * std::sin(a0), z0};
        V3 C{cx + r2 * std::cos(a1), cy + r2 * std::sin(a1), z0};
        V3 D{cx + r1 * std::cos(a1), cy + r1 * std::sin(a1), z0};
        tri(f, nZn(), A, C, B);
        tri(f, nZn(), A, D, C);

        // верхняя грань
        V3 A2{A.x, A.y, z1}, B2{B.x, B.y, z1}, C2{C.x, C.y, z1}, D2{D.x, D.y, z1};
        tri(f, nZp(), A2, B2, C2);
        tri(f, nZp(), A2, C2, D2);

        // внешняя боковая
        V3 nOut{std::cos((a0 + a1) * 0.5), std::sin((a0 + a1) * 0.5), 0.0};
        tri(f, nOut, B, C, C2);
        tri(f, nOut, B, C2, B2);

        // внутренняя боковая
        V3 nIn{-nOut.x, -nOut.y, 0.0};
        tri(f, nIn, D, A, A2);
        tri(f, nIn, D, A2, D2);
    }

    // торцы сектора (радиальные стенки)
    auto add_radial_wall = [&](double ang)
    {
        const double c = std::cos(ang), s = std::sin(ang);
        V3 P1{cx + r1 * c, cy + r1 * s, z0};
        V3 P2{cx + r2 * c, cy + r2 * s, z0};
        V3 P3{P2.x, P2.y, z1};
        V3 P4{P1.x, P1.y, z1};
        V3 n{s, -c, 0.0}; // наруж нормаль ориентируем правильно
        tri(f, n, P1, P2, P3);
        tri(f, n, P1, P3, P4);
    };
    add_radial_wall(phi0);
    add_radial_wall(phi1);
}

// -------------------- базовое тело колена --------------------

struct LayoutZ
{
    double z_mainL0, z_mainL1;
    double z_webL0, z_webL1;
    double z_rod0, z_rod1;
    double z_webR0, z_webR1;
    double z_mainR0, z_mainR1;
};

static LayoutZ layout_from_params(const Params &p)
{
    LayoutZ L{};
    const double Lm = p.length_root_neck;
    const double Lr = p.length_rod_neck;
    const double Wz = p.depth_web;

    L.z_rod0 = -0.5 * Lr;
    L.z_rod1 = +0.5 * Lr;
    L.z_webL0 = L.z_rod0 - Wz;
    L.z_webL1 = L.z_rod0;
    L.z_webR0 = L.z_rod1;
    L.z_webR1 = L.z_rod1 + Wz;
    L.z_mainL0 = L.z_webL0 - Lm;
    L.z_mainL1 = L.z_webL0;
    L.z_mainR0 = L.z_webR1;
    L.z_mainR1 = L.z_webR1 + Lm;
    return L;
}

// Рисуем само колено (без противовесов)
static void add_crank_body(std::ofstream &f, const Params &p, CWVariant var, int seg)
{
    const double axis_dx = p.r;
    const double Rm = 0.5 * p.diam_root_neck;
    const double Rr = 0.5 * p.diam_rod_neck;
    const double y_half = 0.5 * p.width_web;

    const LayoutZ L = layout_from_params(p);

    // 1) коренные шейки
    // левая коренная всегда есть
    add_cyl_Z_mm(f, Rm, p.length_root_neck, 0.0, 0.0, L.z_mainL0, seg);
    // правая коренная — только для полноопорных вариантов
    if (var != CWVariant::SemiSupport)
    {
        add_cyl_Z_mm(f, Rm, p.length_root_neck, 0.0, 0.0, L.z_mainR0, seg);
    }

    // 2) шатунная шейка
    add_cyl_Z_mm(f, Rr, p.length_rod_neck, axis_dx, 0.0, L.z_rod0, seg);

    // 3) щеки
    // левая щека — всегда полная (стадион)
    add_stadium_extrude_Z_mm(f, 0.0, axis_dx, y_half, L.z_webL0, L.z_webL1, seg);

    if (var == CWVariant::SemiSupport)
    {
        // НЕПОЛНООПОРНЫЙ: правая ПОЛУЩЕКА.
        // Плоская грань проходит по оси коренной (X = 0),
        // поэтому строим прямоугольник [0..R] × [−y_half..+y_half] и добавляем ПРАВЫЙ полукруг с центром в X=R.
        add_block_mm(f, 0.0, axis_dx, -y_half, +y_half, L.z_webR0, L.z_webR1);          // прямоугольная часть
        add_cyl_Z_mm(f, y_half, (L.z_webR1 - L.z_webR0), axis_dx, 0.0, L.z_webR0, seg); // правый полукруг
        // (правой коренной здесь нет — см. выше)
    }
    else
    {
        // ПОЛНООПОРНЫЕ: правая щека — полная (стадион)
        add_stadium_extrude_Z_mm(f, 0.0, axis_dx, y_half, L.z_webR0, L.z_webR1, seg);
    }
}

// -------------------- ПРОТИВОВЕСЫ --------------------

/*
  Один противовес на продолжении заданной щеки.

  Геометрия/системы координат:
  - Щёки у нас лежат в плоскости XY, их толщина направлена вдоль Z (экструзия по Z).
  - Противовес тоже строим в плоскости XY как КОЛЬЦЕВОЙ СЕКТОР,
    то есть его «угол» задаётся вокруг оси Z.
  - Сектор ориентируем ВНИЗ по оси Y (в направлении −Y).
  - По Z размещаем «вплотную снаружи» к соответствующей щеке:
      * левая щека:  наружу слева → [ z_webL0 - depth_prot ; z_webL0 ]
      * правая щека: наружу справа → [ z_webR1 ; z_webR1 + depth_prot ]
*/
static void add_one_counterweight(std::ofstream &f, const Params &p,
                                  bool left,            // true = под левой щекой, false = под правой
                                  double r1, double r2, // внутренний/внешний радиусы (МЕТРЫ)
                                  double alpha,         // угол сектора (РАДИАНЫ), 0 < alpha ≤ π
                                  int seg)              // дискретизация окружности
{
    // Центр щёк в плоскости XY: середина между осями шеек (по X = 0), по Y = 0
    const double cx = 0.0;
    const double cy = 0.0;

    // Позиционирование по Z относительно толщи́н щёк
    const LayoutZ L = layout_from_params(p);
    double z0, z1;
    if (left)
    {
        // слева наружу: слой противовеса примыкает к внешней плоскости левой щеки
        z1 = L.z_webL0;         // плоскость наружной стороны левой щеки
        z0 = z1 + p.depth_prot; // уходим наружу на thickness = depth_prot
    }
    else
    {
        // справа наружу
        z0 = L.z_webR1; // плоскость наружной стороны правой щеки
        z1 = z0 - p.depth_prot;
    }

    // Ориентируем сектор ВНИЗ по оси X (минус X).
    // В нашей системе координат «вниз» — угол φ = −π.
    // Сектор симметрично откладываем вокруг этой оси на ±alpha/2.
    const double phi_center = -M_PI; // 180°: направление −X
    const double phi0 = phi_center - 0.5 * alpha;
    const double phi1 = phi_center + 0.5 * alpha;

    // Строим «толстую луну»: кольцевой сектор, экструзия вдоль Z.
    add_sector_ring_Z_mm(f, r1, r2, phi0, phi1, cx, cy, z0, z1, seg);
}

/*
  Набор противовесов для варианта компоновки:
    - FullSupport_V1: два противовеса — под левой и под правой щекой.
    - FullSupport_V2: один противовес — под правой щекой.
    - SemiSupport   : один противовес — под левой щекой.
  Во всех случаях сектор расположен В ПЛОСКОСТИ ЩЁК (XY), вокруг оси Z и смотрит ВНИЗ (−X).
*/
static void add_counterweights(std::ofstream &f, const Params &p, CWVariant var,
                               double r1, double r2, double alpha, int seg)
{
    switch (var)
    {
    case CWVariant::FullSupport_V1:
        add_one_counterweight(f, p, /*left=*/true, r1, r2, alpha, seg);  // под левой щекой
        add_one_counterweight(f, p, /*left=*/false, r1, r2, alpha, seg); // под правой щекой
        break;

    case CWVariant::FullSupport_V2:
        add_one_counterweight(f, p, /*left=*/true, r1, r2, alpha, seg); // только под левой
        break;

    case CWVariant::SemiSupport:
        add_one_counterweight(f, p, /*left=*/true, r1, r2, alpha, seg); // только под левой
        break;
    }
}

// -------------------- API --------------------

CWResult build_counterweights_and_export(
    const Params &p,
    const CrankshaftMassResults &cm,
    CWVariant var,
    const std::string &stl_path_mm,
    int seg,
    bool clamp_alpha_on_limit // можно не использовать, но сигнатуру держим
)
{
    // Параметры противовеса из CSV:
    const double r1 = p.r_prot1;
    const double r2 = p.r_prot2;
    const double depth = p.depth_prot;

    CWResult out{false, 0.0, 0.0, ""};

    if (!(r2 > r1) || depth <= 0.0)
    {
        out.message = "[CW] Неверные r_prot1/r_prot2/depth_prot";
        return out;
    }

    // Площадь поршня
    const double Fp = M_PI * p.diam_cyl * p.diam_cyl / 4.0;

    // ---- Статический момент S_prot по ТЗ  ----
    double S = 0.0;
    switch (var)
    {
    case CWVariant::FullSupport_V1:
        // S_prot = 0.5 * m_rotating * Fp
        S = 0.5 * cm.m_rotating * Fp * p.r;
        break;
    case CWVariant::FullSupport_V2:
        // S_prot = m_rotating * Fp * (axis_p / web_p)
        S = cm.m_rotating * Fp * (cm.axis_p / cm.web_p) * p.r;
        break;
    case CWVariant::SemiSupport:
        // S_prot = m_rotating * Fp * (axis_n / web_n)
        S = cm.m_rotating * Fp * (cm.axis_n / cm.web_n) * p.r;
        break;
    }
    out.S_prot = S;

    // ---- Угол сектора ----
    const double denom = 2.0 * p.rho_material * depth * (r2 * r2 * r2 - r1 * r1 * r1);
    if (denom <= 0.0)
    {
        out.message = "[CW] Неверные геометрические параметры противовеса";
        return out;
    }
    double alpha = 2.0 * std::asin((3.0 * S) / denom); // рад

    // ---- Ограничение 180° ----
    if (alpha > M_PI)
    {
        if (clamp_alpha_on_limit)
        {
            alpha = M_PI * (1.0 - 1e-6);
            out.message = "[CW] Предупреждение: alpha_prot > 180°. Угол ограничен 180°.";
        }
        else
        {
            out.message = "[CW] Угол сектора > 180°. Увеличьте depth_prot.";
            return out;
        }
    }
    if (alpha <= 0.0)
    {
        out.message = "[CW] Угол сектора получился <= 0.";
        return out;
    }

    // ---- Пишем STL ----
    std::ofstream f(stl_path_mm);
    if (!f)
    {
        out.message = "[CW] Не удалось открыть файл STL для записи";
        return out;
    }

    f << "solid crank_with_cw_mm\n";
    // 1) тело колена
    add_crank_body(f, p, var, seg);
    // 2) противовесы (вниз, на продолжении щек)
    add_counterweights(f, p, var, r1, r2, alpha, seg);
    f << "endsolid crank_with_cw_mm\n";

    out.ok = true;
    out.alpha_deg = alpha * 180.0 / M_PI;
    return out;
}
