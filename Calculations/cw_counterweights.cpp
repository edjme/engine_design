#include "cw_counterweights.h"
#include <fstream>
#include <cmath>
#include <algorithm>
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ────────────────────────── простые векторы и треугольники ──────────────────────
struct V3 { double x,y,z; };
static inline void tri(std::ofstream& f, const V3& n, const V3& a, const V3& b, const V3& c){
    f << "  facet normal " << n.x << " " << n.y << " " << n.z << "\n"
      << "    outer loop\n"
      << "      vertex " << a.x << " " << a.y << " " << a.z << "\n"
      << "      vertex " << b.x << " " << b.y << " " << b.z << "\n"
      << "      vertex " << c.x << " " << c.y << " " << c.z << "\n"
      << "    endloop\n"
      << "  endfacet\n";
}
static inline V3 nZp(){ return {0,0, 1}; }
static inline V3 nZn(){ return {0,0,-1}; }
static inline V3 nXp(){ return {1,0,0};  }
static inline V3 nXn(){ return {-1,0,0}; }
static inline V3 nYp(){ return {0,1,0};  }
static inline V3 nYn(){ return {0,-1,0}; }
static inline double mm(double m){ return m*1000.0; }

// ────────────────────────── базовые примитивы (в миллиметрах) ───────────────────
static void add_cyl_Z_mm(std::ofstream& f, double Rm, double h_m,
                         double cx_m, double cy_m, double z0_m, int seg)
{
    const double R  = mm(Rm);
    const double h  = mm(h_m);
    const double cx = mm(cx_m), cy = mm(cy_m);
    const double z0 = mm(z0_m);

    const double d = 2.0*M_PI/seg;
    for(int i=0;i<seg;i++){
        const double a0=i*d, a1=(i+1)*d;
        V3 c0{cx,cy,z0}, c1{cx,cy,z0+h};
        V3 p0{cx+R*std::cos(a0), cy+R*std::sin(a0), z0};
        V3 p1{cx+R*std::cos(a1), cy+R*std::sin(a1), z0};
        V3 q0{p0.x, p0.y, z0+h}, q1{p1.x, p1.y, z0+h};
        tri(f,nZn(),c0,p1,p0);
        tri(f,nZp(),c1,q0,q1);
        V3 n{ std::cos((a0+a1)*0.5), std::sin((a0+a1)*0.5), 0.0 };
        tri(f,n,p0,p1,q1);
        tri(f,n,p0,q1,q0);
    }
}

static void add_block_mm(std::ofstream& f,
    double x0_m,double x1_m, double y0_m,double y1_m, double z0_m,double z1_m)
{
    V3 A{mm(x0_m),mm(y0_m),mm(z0_m)}, B{mm(x1_m),mm(y0_m),mm(z0_m)},
       C{mm(x1_m),mm(y1_m),mm(z0_m)}, D{mm(x0_m),mm(y1_m),mm(z0_m)};
    V3 E{mm(x0_m),mm(y0_m),mm(z1_m)}, G{mm(x1_m),mm(y0_m),mm(z1_m)},
       H{mm(x1_m),mm(y1_m),mm(z1_m)}, K{mm(x0_m),mm(y1_m),mm(z1_m)};
    tri(f,nZn(),A,C,B); tri(f,nZn(),A,D,C);
    tri(f,nZp(),E,G,H); tri(f,nZp(),E,H,K);
    tri(f,nXn(),A,E,K); tri(f,nXn(),A,K,D);
    tri(f,nXp(),B,C,H); tri(f,nXp(),B,H,G);
    tri(f,nYn(),A,B,G); tri(f,nYn(),A,G,E);
    tri(f,nYp(),D,K,H); tri(f,nYp(),D,H,C);
}

// «Стадион» (прямоугольник + полукруги по концам) с экструзией по Z
static void add_stadium_extrude_Z_mm(std::ofstream& f,
    double x0_m, double x1_m, double y_half_m,
    double z0_m, double z1_m, int seg)
{
    add_block_mm(f, x0_m, x1_m, -y_half_m, +y_half_m, z0_m, z1_m);
    add_cyl_Z_mm(f, y_half_m, (z1_m - z0_m), x0_m, 0.0, z0_m, seg);
    add_cyl_Z_mm(f, y_half_m, (z1_m - z0_m), x1_m, 0.0, z0_m, seg);
}

// Сектор кольца (между радиусами r1..r2, углы phi0..phi1, экструзия по Z). Радианы, метры.
static void add_ring_sector_extrude_Z_mm(std::ofstream& f,
    double r1_m, double r2_m, double phi0, double phi1,
    double z0_m, double z1_m, int seg_all)
{
    // сегментов по дуге — пропорционально доле окружности, но не меньше 3
    int seg = std::max(3, (int)std::ceil((phi1 - phi0) / (2.0*M_PI) * seg_all));
    const double d = (phi1 - phi0) / seg;

    const double r1 = mm(r1_m), r2 = mm(r2_m);
    const double z0 = mm(z0_m),  z1 = mm(z1_m);

    for(int i=0;i<seg;i++){
        const double a0 = phi0 + i*d;
        const double a1 = phi0 + (i+1)*d;

        V3 p1a{ r1*std::cos(a0), r1*std::sin(a0), z0 };
        V3 p1b{ r1*std::cos(a1), r1*std::sin(a1), z0 };
        V3 p2a{ r2*std::cos(a0), r2*std::sin(a0), z0 };
        V3 p2b{ r2*std::cos(a1), r2*std::sin(a1), z0 };

        V3 q1a{ p1a.x, p1a.y, z1 }, q1b{ p1b.x, p1b.y, z1 };
        V3 q2a{ p2a.x, p2a.y, z1 }, q2b{ p2b.x, p2b.y, z1 };

        // нижняя крышка (-Z)
        tri(f, nZn(), p1a, p2b, p2a);
        tri(f, nZn(), p1a, p1b, p2b);
        // верхняя крышка (+Z)
        tri(f, nZp(), q1a, q2a, q2b);
        tri(f, nZp(), q1a, q2b, q1b);

        // бок (внешняя окружность r2)
        V3 n2{ std::cos((a0+a1)*0.5), std::sin((a0+a1)*0.5), 0.0 };
        tri(f, n2, p2a, p2b, q2b);
        tri(f, n2, p2a, q2b, q2a);

        // бок (внутренняя окружность r1) — нормаль внутрь отрицательная
        V3 n1{ -std::cos((a0+a1)*0.5), -std::sin((a0+a1)*0.5), 0.0 };
        tri(f, n1, p1b, p1a, q1a);
        tri(f, n1, p1b, q1a, q1b);
    }

    // радиальные торцы
    {
        const double c0 = std::cos(phi0), s0 = std::sin(phi0);
        const double c1 = std::cos(phi1), s1 = std::sin(phi1);
        V3 A0{ r1*c0, r1*s0, z0 }, B0{ r2*c0, r2*s0, z0 },
           C0{ r1*c0, r1*s0, z1 }, D0{ r2*c0, r2*s0, z1 };
        V3 A1{ r1*c1, r1*s1, z0 }, B1{ r2*c1, r2*s1, z0 },
           C1{ r1*c1, r1*s1, z1 }, D1{ r2*c1, r2*s1, z1 };
        // торец на phi0 (нормаль вдоль -Y×X, т.е. перпендикуляр к радиусу)
        V3 n0{ s0, -c0, 0 };
        tri(f,n0,A0,B0,D0); tri(f,n0,A0,D0,C0);
        // торец на phi1
        V3 n1v{ -s1, c1, 0 };
        tri(f,n1v,B1,A1,C1); tri(f,n1v,B1,C1,D1);
    }
}

// ────────────────────────── вычисления для ориентации и размеров ────────────────
static inline double clamp(double v, double a, double b){ return std::max(a,std::min(b,v)); }

// осевые и «вебовые» расстояния (как ты печатал в calc_mass_crankshaft)
static void distances_for_variants(const Params& p,
                                   double& axis_p, double& web_p,
                                   double& axis_n, double& web_n)
{
    axis_p = p.length_root_neck + 2.0*p.depth_web + p.length_rod_neck;
    web_p  = p.length_root_neck + 3.0*p.depth_web + 2.0*p.length_rod_neck;
    axis_n = p.length_rod_neck  + p.depth_web;
    web_n  = 2.0*p.depth_web    + 2.0*p.length_rod_neck;
}

// ────────────────────────── основной экспортёр ──────────────────────────────────
CWSummary build_counterweights_and_export(
    const Params& p,
    const CrankshaftMassResults& cm,
    CWVariant var,
    const std::string& stl_path,
    int seg,
    bool clamp_alpha_on_limit)
{
    CWSummary out;

    // геометрия колена (метры)
    const double Rm = 0.5*p.diam_root_neck;
    const double Rr = 0.5*p.diam_rod_neck;
    const double axis_dx = p.r;                 // между осями шеек (по X)
    const double y_half  = 0.5*p.width_web;     // половина ширины щеки (по Y)
    const double Wz      = p.depth_web;         // толщина щеки (по Z)
    const double Lm      = p.length_root_neck;
    const double Lr      = p.length_rod_neck;

    // Z-компоновка (полноопорный как базовый шаблон)
    const double z_rod0  = -0.5*Lr;
    const double z_rod1  = +0.5*Lr;
    const double z_webL0 = z_rod0 - Wz;
    const double z_webL1 = z_rod0;
    const double z_webR0 = z_rod1;
    const double z_webR1 = z_rod1 + Wz;
    const double z_mainL0= z_webL0 - Lm;
    const double z_mainR0= z_webR1;

    // радиусы противовеса берём из CSV; если нули — расчёт по «первому приближению»
    const double r1 = (p.r_prot1 > 0 ? p.r_prot1 : (0.5*p.diam_root_neck + p.fillet_rad));
    const double r2 = (p.r_prot2 > 0 ? p.r_prot2 : (r1 + p.r));
    const double depth_prot = (p.depth_prot > 0 ? p.depth_prot : p.depth_web);

    // статический момент и угол сектора
    double axis_p, web_p, axis_n, web_n; distances_for_variants(p, axis_p, web_p, axis_n, web_n);
    const double Fp = p.r; // плечо — берём радиус кривошипа (как мы использовали ранее)

    if      (var == CWVariant::FullSupport_V1) out.S_prot = 0.5 * cm.m_rotating * Fp;
    else if (var == CWVariant::FullSupport_V2) out.S_prot =       cm.m_rotating * Fp * (axis_p/web_p);
    else                                       out.S_prot =       cm.m_rotating * Fp * (axis_n/web_n);

    const double denom = 2.0*p.rho_material*depth_prot*(std::pow(r2,3) - std::pow(r1,3));
    double arg = (3.0*out.S_prot)/denom;
    if (clamp_alpha_on_limit) arg = clamp(arg, -1.0, +1.0);

    out.alpha_rad = 2.0*std::asin(arg);
    out.alpha_deg = out.alpha_rad * 180.0/M_PI;

    if (!std::isfinite(out.alpha_rad) || out.alpha_rad <= 0.0){
        out.ok = false;
        out.message = "Невалидный угол сектора противовеса (asin аргумент вне [-1;1]). "
                      "Увеличьте depth_prot или r_prot2, уменьшите r_prot1.";
        std::cerr << out.message << "\n";
    }
    if (out.alpha_deg > 180.0){
        out.ok = false;
        out.message = "alpha_prot > 180°. Необходимо увеличить щеку противовеса (depth_prot).";
        std::cerr << out.message << "\n";
    }

    // где лежат сектора (по Z) — строго ПОД ЩЁКАМИ:
    const bool put_left  = (var == CWVariant::FullSupport_V1) || (var == CWVariant::SemiSupport);
    const bool put_right = (var == CWVariant::FullSupport_V1) || (var == CWVariant::FullSupport_V2);

    // угловая ориентация сектора — «в сторону шатунной шейки» (ось +X):
    const double phi0 = -0.5*out.alpha_rad;
    const double phi1 = +0.5*out.alpha_rad;

    // ───────────────────── запись STL ─────────────────────
    std::ofstream f(stl_path);
    if(!f){ out.ok=false; out.message = "Не удалось открыть STL для записи: " + stl_path; return out; }
    f << "solid crank_with_cw_mm\n";

    // 1) коренные шейки
    add_cyl_Z_mm(f, Rm, Lm, 0.0, 0.0, z_mainL0, seg);
    add_cyl_Z_mm(f, Rm, Lm, 0.0, 0.0, z_mainR0, seg);

    // 2) шатунная шейка
    add_cyl_Z_mm(f, Rr, Lr, axis_dx, 0.0, z_rod0, seg);

    // 3) щеки
    // левая щека — всегда полная
    add_stadium_extrude_Z_mm(f, 0.0, axis_dx, 0.5*p.width_web, z_webL0, z_webL1, seg);

    // правая щека: полная для полноопорных, половинка по X>=0 — для неполноопорного
    if (var == CWVariant::SemiSupport){
        // половинка относительно оси коренной (X=0): берём только область X>=0
        // реализационно — «стадион» целиком и вычесть блок X<0.
        // так как STL — только треугольники без булевых, построим половинку как прямоугольник + один полуцилиндр у x=axis_dx
        // (т.е. отсекаем дугу у x=0).
        const double x0 = 0.0, x1 = axis_dx;
        // прямой пояс (от 0 до axis_dx) но только половина «стадиона» — оставим прямоугольник + один полукруг на x=axis_dx:
        add_block_mm(f, x0, x1, 0.0, +0.5*p.width_web, z_webR0, z_webR1);     // верхняя половина
        add_block_mm(f, x0, x1, -0.5*p.width_web, 0.0, z_webR0, z_webR1);     // нижняя половина
        add_cyl_Z_mm(f, 0.5*p.width_web, (z_webR1 - z_webR0), x1, 0.0, z_webR0, seg); // только правый полукруг
        // (левый полукруг у x=0 отсутствует => половинка)
    } else {
        add_stadium_extrude_Z_mm(f, 0.0, axis_dx, 0.5*p.width_web, z_webR0, z_webR1, seg);
    }

    // 4) противовесы — строго под щеками (их Z-толщина = depth_prot)
    if (put_left){
        add_ring_sector_extrude_Z_mm(f, r1, r2, phi0, phi1,
                                     z_webL0, z_webL0 + depth_prot, seg);
    }
    if (put_right){
        add_ring_sector_extrude_Z_mm(f, r1, r2, phi0, phi1,
                                     z_webR1 - depth_prot, z_webR1, seg);
    }

    f << "endsolid crank_with_cw_mm\n";
    f.close();

    // небольшой отчёт в консоль
    std::cout << "\n=== ПРОТИВОВЕСЫ ===\n"
              << "r_prot1 = " << r1 << " м,  r_prot2 = " << r2 << " м,  depth_prot = " << depth_prot << " м\n"
              << "S_prot  = " << out.S_prot << "\n"
              << "alpha   = " << out.alpha_deg << " град\n"
              << "STL: " << stl_path << "\n";

    return out;
}
