#include "calc_mass_crankshaft.h"
#include <cmath>
#include <fstream>
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

//==================== МАССА / ЦЕНТР ТЯЖЕСТИ (метры, килограммы) ====================
CrankshaftMassResults calc_mass_crankshaft(const Params& p)
{
    const double rho     = p.rho_material;    // кг/м^3
    const double axis_dx = p.r;               // м — расстояние между осями шеек в плоскости щеки = радиус кривошипа

    // Коренная шейка (цилиндр)
    const double vol_root = M_PI * std::pow(0.5 * p.diam_root_neck, 2) * p.length_root_neck;
    const double m_root   = rho * vol_root;
    const double x_root   = 0.0;

    // Шатунная шейка (цилиндр)
    const double vol_rod  = M_PI * std::pow(0.5 * p.diam_rod_neck,  2) * p.length_rod_neck;
    const double m_rod    = rho * vol_rod;
    const double x_rod    = axis_dx;          // центр шатунной по X на расстоянии R

    // Щека (1 шт) как «стадион» в плоскости X–Y: длина по X = axis_dx, ширина по Y = width_web.
    // Экструзия по Z на толщину = depth_web.
    const double y_half       = 0.5 * p.width_web;
    const double A_stad       = axis_dx * (2.0*y_half) + M_PI * y_half * y_half;
    const double vol_web_1    = A_stad * p.depth_web;

    // Вырезы под галтели (по массе учитываем вычитание двух цилиндров радиуса fillet_rad через толщину depth_web)
    const double vol_fillet   = M_PI * p.fillet_rad * p.fillet_rad * p.depth_web;
    const double vol_web_net1 = vol_web_1 - 2.0 * vol_fillet;
    const double m_web_1      = rho * vol_web_net1;
    const double m_web_total  = 2.0 * m_web_1;
    const double x_web        = axis_dx * 0.5;              // ЦТ щек по X посередине между осями шеек

    const double total_mass   = m_root + m_rod + m_web_total;

    const double x_cg = (m_root * x_root + m_rod * x_rod + m_web_total * x_web) / total_mass;

    // Приведённая к оси коренной шейки масса (твоя формула)
    const double m_root_reduce = total_mass * (x_cg * 4.0) / (p.r * M_PI * std::pow(p.diam_cyl, 2));
    const double m_rotating    = m_root_reduce + p.m_2;

    std::cout << "\n Масса колена вала: " << total_mass << " кг.\n";
    std::cout << "\n Расстояние от оси коренной шейки до ЦТ: " << x_cg << " м.\n";
    std::cout << "\n Приведенная масса коренной шейки : " << m_root_reduce << " кг/м^2\n";
    std::cout << "\n Масса вращающихся частей : " << m_rotating << " кг/м^2\n";

    return { total_mass, x_cg, m_root_reduce, m_rotating };
}

//==================== ПРОСТОЙ ГЕНЕРАТОР STL (в миллиметрах) ====================

struct V3 { double x,y,z; };

static void tri(std::ofstream& f, const V3& n, const V3& a, const V3& b, const V3& c)
{
    f << "  facet normal " << n.x << " " << n.y << " " << n.z << "\n"
      << "    outer loop\n"
      << "      vertex " << a.x << " " << a.y << " " << a.z << "\n"
      << "      vertex " << b.x << " " << b.y << " " << b.z << "\n"
      << "      vertex " << c.x << " " << c.y << " " << c.z << "\n"
      << "    endloop\n"
      << "  endfacet\n";
}
static V3 nZp(){ return {0,0, 1}; }
static V3 nZn(){ return {0,0,-1}; }
static V3 nXp(){ return {1,0,0};  }
static V3 nXn(){ return {-1,0,0}; }
static V3 nYp(){ return {0,1,0};  }
static V3 nYn(){ return {0,-1,0}; }

// Вспом. масштабирование «метры → миллиметры»
static inline double mm(double meters){ return meters * 1000.0; }

// Цилиндр вдоль Z (для шеек). Все аргументы — **в метрах**, внутри масштабируем в мм.
static void add_cyl_Z_mm(std::ofstream& f, double R_m, double h_m,
                         double cx_m, double cy_m, double z0_m, int seg)
{
    const double R  = mm(R_m);
    const double h  = mm(h_m);
    const double cx = mm(cx_m), cy = mm(cy_m);
    const double z0 = mm(z0_m);

    const double d = 2.0 * M_PI / seg;
    for (int i=0;i<seg;i++){
        double a0=i*d, a1=(i+1)*d;
        V3 c0{cx,cy,z0}, c1{cx,cy,z0+h};
        V3 p0{cx+R*std::cos(a0), cy+R*std::sin(a0), z0};
        V3 p1{cx+R*std::cos(a1), cy+R*std::sin(a1), z0};
        V3 q0{p0.x, p0.y, z0+h}, q1{p1.x, p1.y, z0+h};
        tri(f, nZn(), c0, p1, p0);
        tri(f, nZp(), c1, q0, q1);
        V3 n{ std::cos((a0+a1)*0.5), std::sin((a0+a1)*0.5), 0.0 };
        tri(f, n, p0, p1, q1);
        tri(f, n, p0, q1, q0);
    }
}

// Прямоугольный блок. Аргументы в метрах.
static void add_block_mm(std::ofstream& f,
                         double x0_m,double x1_m, double y0_m,double y1_m, double z0_m,double z1_m)
{
    V3 A{mm(x0_m),mm(y0_m),mm(z0_m)}, B{mm(x1_m),mm(y0_m),mm(z0_m)},
       C{mm(x1_m),mm(y1_m),mm(z0_m)}, D{mm(x0_m),mm(y1_m),mm(z0_m)};
    V3 E{mm(x0_m),mm(y0_m),mm(z1_m)}, G{mm(x1_m),mm(y0_m),mm(z1_m)},
       H{mm(x1_m),mm(y1_m),mm(z1_m)}, K{mm(x0_m),mm(y1_m),mm(z1_m)};
    tri(f, nZn(), A,C,B); tri(f, nZn(), A,D,C);
    tri(f, nZp(), E,G,H); tri(f, nZp(), E,H,K);
    tri(f, nXn(), A,E,K); tri(f, nXn(), A,K,D);
    tri(f, nXp(), B,C,H); tri(f, nXp(), B,H,G);
    tri(f, nYn(), A,B,G); tri(f, nYn(), A,G,E);
    tri(f, nYp(), D,K,H); tri(f, nYp(), D,H,C);
}

// «Стадион» (прямоугольник + два полукруга по Y) с экструзией по Z. Аргументы в метрах.
static void add_stadium_extrude_Z_mm(std::ofstream& f,
                                     double x0_m, double x1_m, double y_half_m,
                                     double z0_m, double z1_m, int seg)
{
    // прямой участок
    add_block_mm(f, x0_m, x1_m, -y_half_m, +y_half_m, z0_m, z1_m);
    // полукруги по концам
    add_cyl_Z_mm(f, y_half_m, (z1_m - z0_m), x0_m, 0.0, z0_m, seg);
    add_cyl_Z_mm(f, y_half_m, (z1_m - z0_m), x1_m, 0.0, z0_m, seg);
}

// Экспорт STL в мм
bool export_crank_STL_mm(const Params& p, const std::string& stl_path, int seg)
{
    // Геометрия (метры)
    const double axis_dx = p.r;                // длина щеки по X (межосевое в плоскости щёк)
    const double Rm      = 0.5 * p.diam_root_neck;
    const double Rr      = 0.5 * p.diam_rod_neck;

    // Компоновка по Z (полноопорный): mainL | webL | rod | webR | mainR
    const double Lm = p.length_root_neck;
    const double Lr = p.length_rod_neck;
    const double Wz = p.depth_web;             // толщина щеки по Z (экструзия)
    const double y_half = 0.5 * p.width_web;   // половина ширины щеки по Y

    const double z_rod0  = -0.5 * Lr;
    const double z_rod1  = +0.5 * Lr;
    const double z_webL0 = z_rod0 - Wz;
    const double z_webL1 = z_rod0;
    const double z_webR0 = z_rod1;
    const double z_webR1 = z_rod1 + Wz;
    const double z_mainL0= z_webL0 - Lm;
    const double z_mainR0= z_webR1;

    std::ofstream f(stl_path);
    if(!f) return false;

    f << "solid crank_fullsupport_mm\n";

    // коренные (ось X=0, Y=0)
    add_cyl_Z_mm(f, Rm, Lm, 0.0, 0.0, z_mainL0, seg);
    add_cyl_Z_mm(f, Rm, Lm, 0.0, 0.0, z_mainR0, seg);

    // шатунная (ось X=R)
    add_cyl_Z_mm(f, Rr, Lr, axis_dx, 0.0, z_rod0, seg);

    // щеки как «стадион» (ширина по Y = width_web, экструзия по Z = depth_web)
    add_stadium_extrude_Z_mm(f, 0.0, axis_dx, y_half, z_webL0, z_webL1, seg);
    add_stadium_extrude_Z_mm(f, 0.0, axis_dx, y_half, z_webR0, z_webR1, seg);

    f << "endsolid crank_fullsupport_mm\n";
    return true;
}
