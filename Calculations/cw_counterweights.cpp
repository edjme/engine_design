#include "cw_counterweights.h"
#include <cmath>
#include <fstream>
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// -------------------- утилиты STL (в мм) --------------------
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

// цилиндр вдоль Z (радиус/высота/центр – в метрах)
static void add_cyl_Z_mm(std::ofstream &f, double R_m, double h_m,
                         double cx_m, double cy_m, double z0_m, int seg)
{
    const int N = std::max(3, seg);
    const double R = mm(R_m), h = mm(h_m), cx = mm(cx_m), cy = mm(cy_m), z0 = mm(z0_m);
    const double d = 2.0 * M_PI / N;
    for (int i = 0; i < N; i++)
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

// призма-трапеция: по Y от y0 до y1; полуширина X: снизу xb, сверху xa; экструзия по Z
static void add_trapezoid_prism_mm(std::ofstream &f,
                                   double y0_m, double y1_m, double halfX_bottom_m, double halfX_top_m,
                                   double z0_m, double z1_m)
{
    const double xb = mm(halfX_bottom_m), xa = mm(halfX_top_m);
    const double y0 = mm(y0_m), y1 = mm(y1_m);
    const double z0 = mm(z0_m), z1 = mm(z1_m);

    V3 A{-xb, y0, z0}, B{+xb, y0, z0}, C{+xa, y1, z0}, D{-xa, y1, z0};
    V3 E{-xb, y0, z1}, F{+xb, y0, z1}, G{+xa, y1, z1}, H{-xa, y1, z1};

    tri(f, nZn(), A, C, B);
    tri(f, nZn(), A, D, C);
    tri(f, nZp(), E, F, G);
    tri(f, nZp(), E, G, H);

    tri(f, nXn(), A, E, H);
    tri(f, nXn(), A, H, D);
    tri(f, nXp(), B, C, G);
    tri(f, nXp(), B, G, F);
    tri(f, nYn(), A, B, F);
    tri(f, nYn(), A, F, E);
    tri(f, nYp(), D, H, G);
    tri(f, nYp(), D, G, C);
}

// «капсула» Y–X с разными радиусами по краям + экструзия по Z
static void add_tapered_capsule_YX_extrude_Z_mm(std::ofstream &f,
                                                double y0_m, double y1_m, double halfX_bottom_m, double halfX_top_m,
                                                double z0_m, double z1_m, int seg)
{
    add_trapezoid_prism_mm(f, y0_m, y1_m, halfX_bottom_m, halfX_top_m, z0_m, z1_m);
    // нижний и верхний «круги»
    add_cyl_Z_mm(f, halfX_bottom_m, (z1_m - z0_m), 0.0, y0_m, z0_m, seg);
    add_cyl_Z_mm(f, halfX_top_m, (z1_m - z0_m), 0.0, y1_m, z0_m, seg);
}

// кольцевой сектор (противовес) вдоль Z
static void add_sector_ring_Z_mm(std::ofstream &f,
                                 double r1_m, double r2_m, double phi0, double phi1,
                                 double cx_m, double cy_m, double z0_m, double z1_m, int seg)
{
    const int n = std::max(3, (int)std::ceil(seg * std::fabs(phi1 - phi0) / (2 * M_PI)));
    const double d = (phi1 - phi0) / n;
    const double r1 = mm(r1_m), r2 = mm(r2_m);
    const double cx = mm(cx_m), cy = mm(cy_m);
    const double z0 = mm(z0_m), z1 = mm(z1_m);

    for (int i = 0; i < n; i++)
    {
        const double a0 = phi0 + i * d, a1 = phi0 + (i + 1) * d;

        V3 A{cx + r1 * std::cos(a0), cy + r1 * std::sin(a0), z0};
        V3 B{cx + r2 * std::cos(a0), cy + r2 * std::sin(a0), z0};
        V3 C{cx + r2 * std::cos(a1), cy + r2 * std::sin(a1), z0};
        V3 D{cx + r1 * std::cos(a1), cy + r1 * std::sin(a1), z0};
        tri(f, nZn(), A, C, B);
        tri(f, nZn(), A, D, C);

        V3 A2{A.x, A.y, z1}, B2{B.x, B.y, z1}, C2{C.x, C.y, z1}, D2{D.x, D.y, z1};
        tri(f, nZp(), A2, B2, C2);
        tri(f, nZp(), A2, C2, D2);

        V3 nOut{std::cos((a0 + a1) * 0.5), std::sin((a0 + a1) * 0.5), 0.0};
        tri(f, nOut, B, C, C2);
        tri(f, nOut, B, C2, B2);

        V3 nIn{-nOut.x, -nOut.y, 0.0};
        tri(f, nIn, D, A, A2);
        tri(f, nIn, D, A2, D2);
    }

    auto wall = [&](double ang)
    {
        const double c = std::cos(ang), s = std::sin(ang);
        V3 P1{cx + r1 * c, cy + r1 * s, z0};
        V3 P2{cx + r2 * c, cy + r2 * s, z0};
        V3 P3{P2.x, P2.y, z1};
        V3 P4{P1.x, P1.y, z1};
        V3 n{s, -c, 0.0};
        tri(f, n, P1, P2, P3);
        tri(f, n, P1, P3, P4);
    };
    wall(phi0);
    wall(phi1);
}

// -------------------- базовое тело колена (как в mass-модуле) --------------------
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
    const double Lm = 0.5 * p.length_root_neck; // ВЕЗДЕ используем половину
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

// Рисуем колено (Y — вверх, Z — ось коренной, X — ширина)
static void add_crank_body(std::ofstream &f, const Params &p, CWVariant var, int seg)
{
    const double R = p.r;
    const double Rm = 0.5 * p.diam_root_neck;
    const double Rr = 0.5 * p.diam_rod_neck;
    const double xb = Rm + p.fillet_rad; // низ (у коренной) шире
    const double xa = Rr + p.fillet_rad; // верх (у шатунной) уже

    const LayoutZ L = layout_from_params(p);

    // 1) коренные
    add_cyl_Z_mm(f, Rm, (L.z_mainL1 - L.z_mainL0), 0.0, 0.0, L.z_mainL0, seg);
    if (var != CWVariant::SemiSupport)
    {
        add_cyl_Z_mm(f, Rm, (L.z_mainR1 - L.z_mainR0), 0.0, 0.0, L.z_mainR0, seg);
    }

    // 2) шатунная
    add_cyl_Z_mm(f, Rr, (L.z_rod1 - L.z_rod0), 0.0, R, L.z_rod0, seg);

    // 3) щеки
    // левая — полная «капсула»: трапеция + круги на Y=0 и Y=R
    add_tapered_capsule_YX_extrude_Z_mm(f, 0.0, R, xb, xa, L.z_webL0, L.z_webL1, seg);

    if (var == CWVariant::SemiSupport)
    {
        // правая ПОЛУЩЕКА: трапеция + верхний круг (нижнего нет)
        add_trapezoid_prism_mm(f, 0.0, R, xb, xa, L.z_webR0, L.z_webR1);
        add_cyl_Z_mm(f, xa, (L.z_webR1 - L.z_webR0), 0.0, R, L.z_webR0, seg);
    }
    else
    {
        add_tapered_capsule_YX_extrude_Z_mm(f, 0.0, R, xb, xa, L.z_webR0, L.z_webR1, seg);
    }
}

// -------------------- ПРОТИВОВЕСЫ --------------------
// Один противовес на продолжении щеки; сектор в плоскости XY, смотрит в −X.
static void add_one_counterweight(std::ofstream &f, const Params &p,
                                  bool left, double r1, double r2,
                                  double alpha, int seg)
{
    const double cx = 0.0, cy = 0.0; // центр вокруг оси Z
    const LayoutZ L = layout_from_params(p);

    double z0, z1;
    if (left)
    {
        z1 = L.z_webL0;
        z0 = z1 + p.depth_prot;
    } // слева наружу
    else
    {
        z0 = L.z_webR1;
        z1 = z0 - p.depth_prot;
    } // справа наружу

    const double phi_center = -M_PI / 2; // «вниз» по Y
    const double phi0 = phi_center - 0.5 * alpha;
    const double phi1 = phi_center + 0.5 * alpha;

    add_sector_ring_Z_mm(f, r1, r2, phi0, phi1, cx, cy, z0, z1, seg);
}

static void add_counterweights(std::ofstream &f, const Params &p, CWVariant var,
                               double r1, double r2, double alpha, int seg)
{
    switch (var)
    {
    case CWVariant::FullSupport_V1:
        add_one_counterweight(f, p, true, r1, r2, alpha, seg);
        add_one_counterweight(f, p, false, r1, r2, alpha, seg);
        break;
    case CWVariant::FullSupport_V2:
        add_one_counterweight(f, p, true, r1, r2, alpha, seg); // только слева
        break;
    case CWVariant::SemiSupport:
        add_one_counterweight(f, p, true, r1, r2, alpha, seg); // только слева
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
    bool clamp_alpha_on_limit)
{
    const double r1 = p.r_prot1;
    const double r2 = p.r_prot2;
    const double depth = p.depth_prot;

    CWResult out{false, 0.0, 0.0, ""};
    if (!(r2 > r1) || depth <= 0.0)
    {
        out.message = "[CW] Неверные r_prot1/r_prot2/depth_prot";
        return out;
    }

    // площадь поршня
    const double Fp = M_PI * p.diam_cyl * p.diam_cyl / 4.0;

    // Статический момент ОДНОГО противовеса (по ТЗ)
    double S = 0.0;
    switch (var)
    {
    case CWVariant::FullSupport_V1:
        S = 0.5 * cm.m_rotating * Fp * p.r;
        break;
    case CWVariant::FullSupport_V2:
        S = cm.m_rotating * Fp * (cm.axis_p / cm.web_p) * p.r;
        break;
    case CWVariant::SemiSupport:
        S = cm.m_rotating * Fp * (cm.axis_n / cm.web_n) * p.r;
        break;
    }
    out.S_prot = S;

    // Угол сектора
    const double denom = 2.0 * p.rho_material * depth * (r2 * r2 * r2 - r1 * r1 * r1);
    if (denom <= 0.0)
    {
        out.message = "[CW] Неверные геометрические параметры противовеса";
        return out;
    }
    double alpha = 2.0 * std::asin((3.0 * S) / denom); // рад

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

    // STL
    std::ofstream f(stl_path_mm);
    if (!f)
    {
        out.message = "[CW] Не удалось открыть файл STL для записи";
        return out;
    }
    f << "solid crank_with_cw_mm\n";
    add_crank_body(f, p, var, seg);
    add_counterweights(f, p, var, r1, r2, alpha, seg);
    f << "endsolid crank_with_cw_mm\n";

    out.ok = true;
    out.alpha_deg = alpha * 180.0 / M_PI;
    return out;
}
