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
    const double rho     = p.rho_material;        // кг/м^3
    const double R       = p.r;                   // м — расстояние между осями шеек (по X)
    const double y_half  = 0.5 * p.width_web;     // м — половина ширины щеки по Y
    const double Wz      = p.depth_web;           // м — толщина щеки по Z
    const double Rm      = 0.5 * p.diam_root_neck;
    const double Rr      = 0.5 * p.diam_rod_neck;

    // Коренная
    const double vol_root = M_PI * Rm*Rm * p.length_root_neck;
    const double m_root   = rho * vol_root;
    const double x_root   = 0.0;

    // Шатунная
    const double vol_rod  = M_PI * Rr*Rr * p.length_rod_neck;
    const double m_rod    = rho * vol_rod;
    const double x_rod    = R;

    // Общие полезные величины
    const double A_rect    = R * (2.0 * y_half);             // прямоугольник 0..R
    const double A_fullCir = M_PI * y_half * y_half;         // полный круг
    const double A_semi    = 0.5 * A_fullCir;                // полукруг
    const double V_fillet  = M_PI * p.fillet_rad * p.fillet_rad * Wz; // «галтель» (одна)

    double total_mass = 0.0;
    double x_cg = 0.0;

    if (std::lround(p.config_crankshaft) == 2) {
        // ─────────────────── НЕПОЛНООПОРНЫЙ ───────────────────
        // Левая щека — полная (стадион): прямоугольник + два полукруга
        const double A_web_L = A_rect + A_fullCir;                 // «стадион»
        const double V_web_L = A_web_L * Wz - 2.0 * V_fillet;      // два выреза под галтели
        const double m_web_L = rho * V_web_L;
        const double x_web_L = R * 0.5;                            // симметрия

        // Правая щека — ПОЛУЩЕКА: прямоугольник [0..R] + ПРАВЫЙ полукруг (центр в x=R),
        // плоская грань по оси коренной шейки x=0.
        const double A_web_R = A_rect + A_semi;
        const double V_web_R = A_web_R * Wz - 2.0 * V_fillet;      // у неё тоже 2 галтели (к коренной и к шатунной)
        const double m_web_R = rho * V_web_R;

        // Центр тяжести правой полущёки: смесь прямоугольника и полукруга
        const double x_rect    = R * 0.5;
        const double x_semi_R  = R + 4.0 * y_half / (3.0 * M_PI);  // центроид полукруга вправо от центра
        const double x_web_R   = (A_rect * x_rect + A_semi * x_semi_R) / (A_web_R);

        total_mass = m_root + m_rod + m_web_L + m_web_R;
        x_cg = (m_root*x_root + m_rod*x_rod + m_web_L*x_web_L + m_web_R*x_web_R) / total_mass;
    } else {
        // ─────────────────── ПОЛНООПОРНЫЙ ───────────────────
        const double A_web    = A_rect + A_fullCir;               // «стадион»
        const double V_web    = A_web * Wz - 2.0 * V_fillet;
        const double m_web_1  = rho * V_web;
        const double m_web_2  = rho * V_web;
        const double m_web_tot= m_web_1 + m_web_2;
        const double x_web    = R * 0.5;

        total_mass = m_root + m_rod + m_web_tot;
        x_cg = (m_root*x_root + m_rod*x_rod + m_web_tot*x_web) / total_mass;
    }
    
     //вспомогательные расстояния (для контроля компоновки)
    const double axis_p = p.length_root_neck + 2.0*p.depth_web + p.length_rod_neck;
    const double web_p  = p.length_root_neck + 3.0*p.depth_web + 2.0*p.length_rod_neck;
    const double axis_n = p.length_rod_neck  + p.depth_web;
    const double web_n  = 2.0*p.depth_web    + 2.0*p.length_rod_neck;

    std::cout
        << "\n=== Контрольные расстояния ===\n"
        << "Полноопорный:  axis_p = " << axis_p << " м,  web_p = " << web_p << " м\n"
        << "Неполноопорный:axis_n = " << axis_n << " м,  web_n = " << web_n << " м\n";


    // Приведённая масса к оси коренной
    const double m_root_reduce = total_mass * (x_cg * 4.0) / (p.r * M_PI * std::pow(p.diam_cyl, 2));
    const double m_rotating    = m_root_reduce + p.m_2;

    std::cout << "\nКонфигурация: " << (std::lround(p.config_crankshaft)==2 ? "неполноопорный" : "полноопорный") << "\n";
    std::cout << "Масса колена: " << total_mass << " кг\n";
    std::cout << "x_CG (от оси коренной): " << x_cg << " м\n";
    std::cout << "Приведённая масса коренной: " << m_root_reduce << " кг/м^2\n";
    std::cout << "Масса вращающихся частей: " << m_rotating << " кг/м^2\n";

    return { total_mass, x_cg, m_root_reduce, m_rotating, axis_p, web_p, axis_n, web_n };
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

static inline double mm(double meters){ return meters * 1000.0; }

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

static void add_stadium_extrude_Z_mm(std::ofstream& f,
                                     double x0_m, double x1_m, double y_half_m,
                                     double z0_m, double z1_m, int seg)
{
    add_block_mm(f, x0_m, x1_m, -y_half_m, +y_half_m, z0_m, z1_m);
    add_cyl_Z_mm(f, y_half_m, (z1_m - z0_m), x0_m, 0.0, z0_m, seg);
    add_cyl_Z_mm(f, y_half_m, (z1_m - z0_m), x1_m, 0.0, z0_m, seg);
}

// ── Экспорт STL в мм ──
bool export_crank_STL_mm(const Params& p, const std::string& stl_path, int seg)
{
    const bool nonfull = (std::lround(p.config_crankshaft) == 2);

    // Геометрия (метры)
    const double R      = p.r;
    const double Rm     = 0.5 * p.diam_root_neck;
    const double Rr     = 0.5 * p.diam_rod_neck;
    const double Lm     = p.length_root_neck;
    const double Lr     = p.length_rod_neck;
    const double Wz     = p.depth_web;
    const double y_half = 0.5 * p.width_web;

    // Компоновка по Z
    const double z_rod0   = -0.5 * Lr;
    const double z_rod1   =  0.5 * Lr;
    const double z_webL0  = z_rod0 - Wz;
    const double z_webL1  = z_rod0;
    const double z_webR0  = z_rod1;
    const double z_webR1  = z_rod1 + Wz;
    const double z_mainL0 = z_webL0 - Lm;

    std::ofstream f(stl_path);
    if(!f) return false;

    f << "solid crank_mm\n";

    // коренная слева
    add_cyl_Z_mm(f, Rm, Lm, 0.0, 0.0, z_mainL0, seg);

    // левая полная щека (стадион)
    add_stadium_extrude_Z_mm(f, 0.0, R, y_half, z_webL0, z_webL1, seg);

    // шатунная
    add_cyl_Z_mm(f, Rr, Lr, R, 0.0, z_rod0, seg);

    if (nonfull) {
        // правая ПОЛУЩЕКА: прямоугольник 0..R + правый полукруг (плоскость по оси коренной x=0)
        add_block_mm(f, 0.0, R, -y_half, +y_half, z_webR0, z_webR1);
        add_cyl_Z_mm(f, y_half, (z_webR1 - z_webR0), R, 0.0, z_webR0, seg);
        // (в неполноопорном второй коренной шейки нет)
    } else {
        // полноопорный: правая полная щека и правая коренная
        add_stadium_extrude_Z_mm(f, 0.0, R, y_half, z_webR0, z_webR1, seg);
        add_cyl_Z_mm(f, Rm, Lm, 0.0, 0.0, z_webR1, seg);
    }

    f << "endsolid crank_mm\n";
    return true;
}
