// Interface/gui_win.cpp
// ---------------------
// WinAPI + GDI+ GUI: редактирование Params, запуск расчётов,
// автосоздание папки "output\\Расчетные данные (YYYY-MM-DD HH-MM-SS)",
// стек папок (откат), графики P–V / P(α) / Forces / ВДС, STL/СW, сводка.
// Компиляция (MinGW-w64 g++):
//   -std=c++17 -lgdiplus -lgdi32 -lole32 -luuid -DUNICODE -D_UNICODE -municode
//   -finput-charset=UTF-8 -fexec-charset=UTF-8

#include <windows.h>
#include <gdiplus.h>
#include <string>
#include <vector>
#include <cwchar>
#include <algorithm>
#include <fstream>
#include <ctime>
#include <sys/stat.h>

#pragma comment(lib, "gdiplus.lib")

// ---- расчётные заголовки ----
#include "input_data/input.h"
#include "Calculations/ind_diagr.h"
#include "Calculations/diag_palpha.h"
#include "Calculations/forces_ksm.h"
#include "Calculations/vds_crankpin.h"
#include "Calculations/calc_mass_crankshaft.h"
#include "Calculations/cw_counterweights.h"
#include "Calculations/balance_inertia.h"

using namespace Gdiplus;

//==================== Глобальное состояние ====================
struct AppData
{
    Params p{};
    IndicatorResults ind{};
    PAlphaResults pal{};
    ForcesResults fr{};
    VDSCrankpinResults vds{};
    CrankshaftMassResults cm{};
    CWResult cw{};
    BalanceResults bi{};

    bool has_ind = false, has_pal = false, has_fr = false,
         has_vds = false, has_cm = false, has_cw = false, has_bi = false;

    std::wstring outDir;               // активная папка вывода
    std::vector<std::wstring> history; // стек папок вывода (для отката)
} G;

static HWND g_summaryEdit = nullptr; // нижний многострочный EDIT
struct FieldDesc
{
    const wchar_t *label;
    HWND edit = nullptr;
    enum T
    {
        Dbl,
        Int
    } type = Dbl;
    void *ptr = nullptr;
};
static std::vector<FieldDesc> F;

//==================== Утилиты ====================
static std::wstring utf8_to_wide(const std::string &s)
{
    if (s.empty())
        return L"";
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    std::wstring w(n ? n - 1 : 0, L'\0');
    if (n > 1)
        MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &w[0], n);
    return w;
}
static std::string wide_to_utf8(const std::wstring &w)
{
    if (w.empty())
        return {};
    int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string s(n ? n - 1 : 0, '\0');
    if (n > 1)
        WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, &s[0], n, nullptr, nullptr);
    return s;
}
template <typename T>
static std::pair<T, T> vminmax(const std::vector<T> &v)
{
    if (v.empty())
        return {T(0), T(1)};
    auto mm = std::minmax_element(v.begin(), v.end());
    return {*mm.first, *mm.second};
}
static std::wstring fmt_double(double x, int prec = 6)
{
    wchar_t b[64];
    swprintf(b, 64, L"%.*f", prec, x);
    return b;
}
static std::wstring fmt_int(int x)
{
    wchar_t b[32];
    swprintf(b, 32, L"%d", x);
    return b;
}

// безопасное создание каталога (wchar)
static bool ensure_dir(const std::wstring &wpath)
{
    if (wpath.empty())
        return false;
    if (CreateDirectoryW(wpath.c_str(), nullptr) || GetLastError() == ERROR_ALREADY_EXISTS)
        return true;
    return false;
}
// yyyy-mm-dd hh-mm-ss
static std::wstring make_timestamp()
{
    std::time_t t = std::time(nullptr);
    std::tm lt{};
#ifdef _WIN32
    localtime_s(&lt, &t);
#else
    lt = *std::localtime(&t);
#endif
    wchar_t b[64];
    swprintf(b, 64, L"%04d-%02d-%02d %02d-%02d-%02d",
             lt.tm_year + 1900, lt.tm_mon + 1, lt.tm_mday, lt.tm_hour, lt.tm_min, lt.tm_sec);
    return b;
}
// создать новую папку результатов и сделать её активной
static std::wstring create_new_out_dir()
{
    ensure_dir(L"output");
    std::wstring dir = L"output\\Расчетные данные (" + make_timestamp() + L")";
    ensure_dir(dir);
    G.history.push_back(dir);
    G.outDir = dir;
    return dir;
}

//==================== Рисование осей/линий ====================
static void draw_axes(Graphics &g, const Rect &rc, const std::wstring &xlab, const std::wstring &ylab)
{
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);

    SolidBrush bg(Color(255, 245, 247, 250));
    g.FillRectangle(&bg, rc);
    Pen frame(Color(255, 51, 65, 85), 1.f);
    g.DrawRectangle(&frame, rc);

    Pen grid(Color(255, 41, 50, 65), 1.f);
    for (int i = 1; i < 6; i++)
    {
        INT x = rc.X + (INT)std::lround((double)rc.Width * i / 6);
        g.DrawLine(&grid, x, rc.Y, x, rc.GetBottom());
    }
    for (int j = 1; j < 5; j++)
    {
        INT y = rc.Y + (INT)std::lround((double)rc.Height * j / 5);
        g.DrawLine(&grid, rc.X, y, rc.GetRight(), y);
    }

    FontFamily ff(L"Segoe UI");
    Font font(&ff, 12.f, FontStyleRegular, UnitPixel);
    SolidBrush txt(Color(255, 80, 90, 105));
    StringFormat center;
    center.SetAlignment(StringAlignmentCenter);
    RectF rx((REAL)rc.X, (REAL)(rc.GetBottom() + 6), (REAL)rc.Width, 20.f);
    g.DrawString(xlab.c_str(), -1, &font, rx, &center, &txt);

    g.TranslateTransform((REAL)(rc.X - 36), (REAL)(rc.Y + rc.Height / 2));
    g.RotateTransform(-90.f);
    RectF ry(-80.f, -12.f, 160.f, 24.f);
    g.DrawString(ylab.c_str(), -1, &font, ry, &center, &txt);
    g.ResetTransform();
}
static void map_polyline(Graphics &g, const Rect &rc,
                         const std::vector<double> &X, const std::vector<double> &Y,
                         double xmin, double xmax, double ymin, double ymax,
                         Color color, float width = 2.f)
{
    if (X.empty() || Y.empty() || X.size() != Y.size())
        return;
    Pen pen(color, width);
    auto xmap = [&](double x)
    { if(xmax==xmin)xmax=xmin+1; double t=(x-xmin)/(xmax-xmin); t=std::clamp(t,0.0,1.0); return rc.X+(INT)std::lround(t*rc.Width); };
    auto ymap = [&](double y)
    { if(ymax==ymin)ymax=ymin+1; double t=1.0-(y-ymin)/(ymax-ymin); t=std::clamp(t,0.0,1.0); return rc.Y+(INT)std::lround(t*rc.Height); };
    for (size_t i = 1; i < X.size(); ++i)
        g.DrawLine(&pen, xmap(X[i - 1]), ymap(Y[i - 1]), xmap(X[i]), ymap(Y[i]));
}

//==================== Окна графиков ====================
static LRESULT CALLBACK PlotWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        BeginPaint(hwnd, &ps);
        auto fn = (void (*)(HWND))GetWindowLongPtr(hwnd, GWLP_USERDATA);
        if (fn)
            fn(hwnd);
        EndPaint(hwnd, &ps);
        return 0;
    }
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}
static int ShowPlotWindow(const std::wstring &title, void (*paintFn)(HWND),
                          int w = 1200, int h = 800)
{
    HINSTANCE hi = GetModuleHandleW(nullptr);
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc = PlotWndProc;
    wc.hInstance = hi;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"PlotWndClass";
    RegisterClassExW(&wc);

    HWND wnd = CreateWindowExW(0, L"PlotWndClass", title.c_str(),
                               WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                               CW_USEDEFAULT, CW_USEDEFAULT, w, h,
                               nullptr, nullptr, hi, nullptr);
    if (!wnd)
        return 1;
    SetWindowLongPtr(wnd, GWLP_USERDATA, (LONG_PTR)paintFn);
    ShowWindow(wnd, SW_SHOW);
    UpdateWindow(wnd);

    MSG msg;
    while (IsWindow(wnd) && GetMessageW(&msg, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
        if (!IsWindowVisible(wnd))
            break;
    }
    return 0;
}

// четыре «рисовалки»
static void PaintPV(HWND hwnd)
{
    Graphics g(hwnd);
    RECT r;
    GetClientRect(hwnd, &r);
    Rect rc(80, 40, (r.right - r.left) - 240, (r.bottom - r.top) - 120);
    draw_axes(g, rc, L"V, м³", L"P, МПа");
    if (!G.has_ind)
        return;
    std::vector<double> P;
    for (double p : G.ind.P_path)
        P.push_back(p * 1e-6);
    auto vx = vminmax(G.ind.V_path), vy = vminmax(P);
    map_polyline(g, rc, G.ind.V_path, P, vx.first, vx.second, vy.first, vy.second, Color(255, 58, 121, 254), 2.f);
}
static void PaintPAlpha(HWND hwnd)
{
    Graphics g(hwnd);
    RECT r;
    GetClientRect(hwnd, &r);
    Rect rc(80, 40, (r.right - r.left) - 240, (r.bottom - r.top) - 120);
    draw_axes(g, rc, L"α, град", L"P, Па");
    if (!G.has_pal)
        return;
    auto ax = vminmax(G.pal.alpha_deg), py = vminmax(G.pal.P_alpha);
    map_polyline(g, rc, G.pal.alpha_deg, G.pal.P_alpha, ax.first, ax.second, py.first, py.second, Color(255, 56, 189, 248), 2.f);
}
static void PaintForces(HWND hwnd)
{
    Graphics g(hwnd);
    RECT r;
    GetClientRect(hwnd, &r);
    Rect rc(80, 40, (r.right - r.left) - 240, (r.bottom - r.top) - 120);
    draw_axes(g, rc, L"α, град", L"Н / Н·м");
    if (!G.has_fr)
        return;
    auto [amin, amax] = vminmax(G.fr.alpha_deg);
    auto fsum = vminmax(G.fr.F_sum), fin = vminmax(G.fr.F_in), mm = vminmax(G.fr.M_cr);
    double ymin = std::min({fsum.first, fin.first, mm.first});
    double ymax = std::max({fsum.second, fin.second, mm.second});
    map_polyline(g, rc, G.fr.alpha_deg, G.fr.F_sum, amin, amax, ymin, ymax, Color(255, 58, 121, 254), 2.f);
    map_polyline(g, rc, G.fr.alpha_deg, G.fr.F_in, amin, amax, ymin, ymax, Color(255, 31, 187, 115), 2.f);
    map_polyline(g, rc, G.fr.alpha_deg, G.fr.M_cr, amin, amax, ymin, ymax, Color(255, 251, 191, 36), 2.f);
}
static void PaintVDS(HWND hwnd)
{
    Graphics g(hwnd);
    RECT r;
    GetClientRect(hwnd, &r);
    Rect rc(80, 40, (r.right - r.left) - 240, (r.bottom - r.top) - 120);
    draw_axes(g, rc, L"X=Z+P′c, Н", L"Y=T, Н");
    if (!G.has_vds)
        return;
    auto zx = vminmax(G.vds.Z_shifted), ty = vminmax(G.vds.T_same);
    map_polyline(g, rc, G.vds.Z_shifted, G.vds.T_same, zx.first, zx.second, ty.first, ty.second, Color(255, 139, 92, 246), 2.f);
}

//==================== Форма параметров ====================
enum : int
{
    ID_BTN_LOAD = 10,
    ID_BTN_SAVE,
    ID_BTN_NEW_RUN,
    ID_BTN_RECALC,
    ID_BTN_ROLLBACK,
    ID_BTN_PV,
    ID_BTN_PALPHA,
    ID_BTN_FORCES,
    ID_BTN_VDS,
    ID_BTN_STL,
    ID_BTN_STL_CW
};

static void add_row(HWND parent, int idx, const wchar_t *name, FieldDesc::T t, void *ptr)
{
    const int colW = 320, leftX = 10, topY = 10, rowH = 28, labelW = 170, editW = 120;
    HINSTANCE hi = (HINSTANCE)GetWindowLongPtr(parent, GWLP_HINSTANCE);
    int col = idx % 2, row = idx / 2;
    int x = leftX + col * colW, y = topY + row * rowH;
    CreateWindowExW(0, L"STATIC", name, WS_CHILD | WS_VISIBLE, x, y + 6, labelW, 18, parent, nullptr, hi, nullptr);
    HWND ed = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                              x + labelW, y, editW, 24, parent, nullptr, hi, nullptr);
    F.push_back(FieldDesc{name, ed, t, ptr});
}
static void create_param_ui(HWND hwnd)
{
    // все поля из Params
    F.reserve(40);
    int i = 0;
    add_row(hwnd, i++, L"diam_cyl (м)", FieldDesc::Dbl, &G.p.diam_cyl);
    add_row(hwnd, i++, L"stroke (м)", FieldDesc::Dbl, &G.p.stroke);
    add_row(hwnd, i++, L"epsilent", FieldDesc::Dbl, &G.p.epsilent);
    add_row(hwnd, i++, L"p_a (Па)", FieldDesc::Dbl, &G.p.p_a);
    add_row(hwnd, i++, L"p_r (Па)", FieldDesc::Dbl, &G.p.p_r);
    add_row(hwnd, i++, L"n_1", FieldDesc::Dbl, &G.p.n_1);
    add_row(hwnd, i++, L"n_2", FieldDesc::Dbl, &G.p.n_2);
    add_row(hwnd, i++, L"lymbda_z", FieldDesc::Dbl, &G.p.lymbda_z);
    add_row(hwnd, i++, L"ro", FieldDesc::Dbl, &G.p.ro);
    add_row(hwnd, i++, L"lyambda (R/L)", FieldDesc::Dbl, &G.p.lyambda);
    add_row(hwnd, i++, L"n (об/мин)", FieldDesc::Dbl, &G.p.n);
    add_row(hwnd, i++, L"m_pd (кг/м²)", FieldDesc::Dbl, &G.p.m_pd);
    add_row(hwnd, i++, L"r (м)", FieldDesc::Dbl, &G.p.r);
    add_row(hwnd, i++, L"leng_rod (м)", FieldDesc::Dbl, &G.p.leng_rod);
    add_row(hwnd, i++, L"m_rod (кг/м²)", FieldDesc::Dbl, &G.p.m_rod);
    add_row(hwnd, i++, L"m_2 (кг/м²)", FieldDesc::Dbl, &G.p.m_2);
    add_row(hwnd, i++, L"w (рад/с)", FieldDesc::Dbl, &G.p.w);
    add_row(hwnd, i++, L"tau", FieldDesc::Int, &G.p.tau);
    add_row(hwnd, i++, L"count_cyl", FieldDesc::Int, &G.p.count_cyl);
    add_row(hwnd, i++, L"gamma (°)", FieldDesc::Dbl, &G.p.gamma);
    add_row(hwnd, i++, L"diam_root_neck (м)", FieldDesc::Dbl, &G.p.diam_root_neck);
    add_row(hwnd, i++, L"diam_rod_neck (м)", FieldDesc::Dbl, &G.p.diam_rod_neck);
    add_row(hwnd, i++, L"length_rod_neck (м)", FieldDesc::Dbl, &G.p.length_rod_neck);
    add_row(hwnd, i++, L"length_root_neck (м)", FieldDesc::Dbl, &G.p.length_root_neck);
    add_row(hwnd, i++, L"depth_web (м)", FieldDesc::Dbl, &G.p.depth_web);
    add_row(hwnd, i++, L"fillet_rad (м)", FieldDesc::Dbl, &G.p.fillet_rad);
    add_row(hwnd, i++, L"width_web (м)", FieldDesc::Dbl, &G.p.width_web);
    add_row(hwnd, i++, L"dist_axes (м)", FieldDesc::Dbl, &G.p.dist_axes);
    add_row(hwnd, i++, L"dist_web (м)", FieldDesc::Dbl, &G.p.dist_web);
    add_row(hwnd, i++, L"rho_material (кг/м³)", FieldDesc::Dbl, &G.p.rho_material);
    add_row(hwnd, i++, L"config_crankshaft (1/2)", FieldDesc::Int, &G.p.config_crankshaft);
    add_row(hwnd, i++, L"config_prot (1..3)", FieldDesc::Int, &G.p.config_prot);
    add_row(hwnd, i++, L"r_prot1 (м)", FieldDesc::Dbl, &G.p.r_prot1);
    add_row(hwnd, i++, L"r_prot2 (м)", FieldDesc::Dbl, &G.p.r_prot2);
    add_row(hwnd, i++, L"depth_prot (м)", FieldDesc::Dbl, &G.p.depth_prot);

    // Кнопки (справа)
    HINSTANCE hi = (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE);
    int bx = 700, by = 10;
    auto addBtn = [&](int id, const wchar_t *txt)
    {
        CreateWindowExW(0, L"BUTTON", txt, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                        bx, by, 280, 34, hwnd, (HMENU)(INT_PTR)id, hi, nullptr);
        by += 40;
    };
    addBtn(ID_BTN_LOAD, L"Загрузить из CSV");
    addBtn(ID_BTN_SAVE, L"Сохранить в CSV");
    addBtn(ID_BTN_NEW_RUN, L"Новый расчёт (создать папку)");
    addBtn(ID_BTN_RECALC, L"Пересчитать в текущую папку");
    addBtn(ID_BTN_ROLLBACK, L"Откат к предыдущей папке");
    by += 8;
    addBtn(ID_BTN_PV, L"Показать P–V");
    addBtn(ID_BTN_PALPHA, L"Показать P(α)");
    addBtn(ID_BTN_FORCES, L"Показать силы/момент");
    addBtn(ID_BTN_VDS, L"Показать ВДС");
    by += 8;
    addBtn(ID_BTN_STL, L"Экспорт STL колена");
    addBtn(ID_BTN_STL_CW, L"Экспорт STL + CW");

    // Сводка
    g_summaryEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL | ES_READONLY,
                                    10, 560, 970, 220, hwnd, nullptr, hi, nullptr);
}
static void fill_fields_from_params()
{
    for (auto &f : F)
    {
        if (!f.edit)
            continue;
        std::wstring s = (f.type == FieldDesc::Dbl)
                             ? fmt_double(*(double *)f.ptr, 6)
                             : fmt_int((int)std::llround(*(double *)f.ptr));
        SetWindowTextW(f.edit, s.c_str());
    }
}
static void collect_params_from_fields()
{
    wchar_t buf[128];
    for (auto &f : F)
    {
        if (!f.edit)
            continue;
        GetWindowTextW(f.edit, buf, 128);
        if (f.type == FieldDesc::Dbl)
        {
            wchar_t *e = nullptr;
            double v = wcstod(buf, &e);
            if (e != buf)
                *(double *)f.ptr = v;
        }
        else
        {
            wchar_t *e = nullptr;
            long v = wcstol(buf, &e, 10);
            if (e != buf)
                *(double *)f.ptr = (double)v;
        }
    }
}
static bool save_params_to_csv(const Params &p, const std::wstring &pathW)
{
    std::ofstream out(wide_to_utf8(pathW));
    if (!out)
        return false;
    out << "param,value\n";
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
    out << "r_prot1," << p.r_prot1 << "\n";
    out << "r_prot2," << p.r_prot2 << "\n";
    out << "depth_prot," << p.depth_prot << "\n";
    return true;
}

//==================== Сводка ====================
static void update_summary_text()
{
    std::wstring s = L"Активная папка вывода: " + G.outDir + L"\r\n\r\n";

    if (G.has_cw)
    {
        s += L"— Противовесы (CW)\r\n";
        s += L"   α, град = " + fmt_double(G.cw.alpha_deg, 3) + L"\r\n";
        s += L"   S_prot, кг·м = " + fmt_double(G.cw.S_prot, 6) + L"\r\n";
        if (!G.cw.message.empty())
            s += utf8_to_wide(G.cw.message) + L"\r\n";
    }
    if (G.has_bi)
    {
        s += L"— Баланс сил/моментов инерции\r\n";
        s += L"   S_prot1 = " + fmt_double(G.bi.S_prot1, 6) + L"\r\n";
        s += L"   S_prot2 = " + fmt_double(G.bi.S_prot2, 6) + L"\r\n";
        if (!G.bi.summary.empty())
            s += utf8_to_wide(G.bi.summary) + L"\r\n";
    }
    if (G.has_fr)
    {
        s += L"\r\n— Силы КШМ\r\n" + utf8_to_wide(G.fr.summary) + L"\r\n";
    }
    if (G.has_vds)
    {
        s += L"\r\n— ВДС\r\n" + utf8_to_wide(G.vds.summary) + L"\r\n";
    }
    if (G.has_cm)
    {
        s += L"\r\n— Масса и ЦТ колена\r\n";
        s += L"   m = " + fmt_double(G.cm.total_mass, 6) + L" кг,  y_CG = " + fmt_double(G.cm.y_cg, 6) + L" м\r\n";
        s += L"   m_root_reduce = " + fmt_double(G.cm.m_root_reduce, 6) + L" кг/м²,  m_rotating = " + fmt_double(G.cm.m_rotating, 6) + L" кг/м²\r\n";
        s += L"   axis_p=" + fmt_double(G.cm.axis_p, 6) + L", web_p=" + fmt_double(G.cm.web_p, 6) + L"\r\n";
        s += L"   axis_n=" + fmt_double(G.cm.axis_n, 6) + L", web_n=" + fmt_double(G.cm.web_n, 6) + L"\r\n";
    }

    if (g_summaryEdit)
        SetWindowTextW(g_summaryEdit, s.c_str());
}

//==================== Запуски расчётов ====================
static void run_all_into_current_dir()
{
    // пути в активной папке
    std::wstring pv_csv = G.outDir + L"\\indicator_pv.csv";
    std::wstring pv_html = G.outDir + L"\\indicator_pv.html";
    std::wstring pa_csv = G.outDir + L"\\p_alpha.csv";
    std::wstring pa_html = G.outDir + L"\\p_alpha.html";
    std::wstring fr_csv = G.outDir + L"\\forces.csv";
    std::wstring fr_html = G.outDir + L"\\forces.html";
    std::wstring vds_csv = G.outDir + L"\\vds_crankpin.csv";
    std::wstring vds_html = G.outDir + L"\\vds_crankpin.html";
    std::wstring bi_csv = G.outDir + L"\\balance_inertia.csv";
    std::wstring bi_html = G.outDir + L"\\balance_inertia.html";
    std::wstring stl_cr = G.outDir + L"\\crank_mm.stl";
    std::wstring stl_cw = G.outDir + L"\\crank_with_cw_mm.stl";

    // 1) Индикаторная
    G.ind = build_indicator_PV(G.p, 400, wide_to_utf8(pv_csv), wide_to_utf8(pv_html), false);
    G.has_ind = true;

    // 2) P(α)
    G.pal = build_P_alpha(G.p, G.ind, 0.5, wide_to_utf8(pa_csv), wide_to_utf8(pa_html), false);
    G.has_pal = true;

    // 3) Силы
    G.fr = build_forces_ksm(G.p, G.pal, wide_to_utf8(fr_csv), wide_to_utf8(fr_html), false);
    G.has_fr = true;

    // 4) ВДС
    G.vds = build_vds_crankpin(G.p, G.fr, wide_to_utf8(vds_csv), wide_to_utf8(vds_html), false);
    G.has_vds = true;

    // 5) Масса/ЦТ
    G.cm = calc_mass_crankshaft(G.p);
    G.has_cm = true;

    // 6) Баланс инерции
    G.bi = build_balance_inertia(G.p, G.cm, wide_to_utf8(bi_csv), wide_to_utf8(bi_html), false);
    G.has_bi = true;

    // 7) STL и CW
    export_crank_STL_mm(G.p, wide_to_utf8(stl_cr), 128);
    CWVariant var = CWVariant::FullSupport_V1;
    if ((int)G.p.config_prot == 2)
        var = CWVariant::FullSupport_V2;
    else if ((int)G.p.config_prot == 3)
        var = CWVariant::SemiSupport;
    G.cw = build_counterweights_and_export(G.p, G.cm, var, wide_to_utf8(stl_cw), 128, true);
    G.has_cw = G.cw.ok;

    // 8) сводка
    update_summary_text();
}

//==================== Главное окно ====================
static LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_CREATE:
        create_param_ui(hwnd);
        fill_fields_from_params();
        if (G.outDir.empty())
            create_new_out_dir();
        update_summary_text();
        return 0;

    case WM_COMMAND:
    {
        switch (LOWORD(wp))
        {
        case ID_BTN_LOAD:
            G.p = input();
            fill_fields_from_params();
            MessageBoxW(hwnd, L"Параметры загружены из input_data\\input.csv",
                        L"OK", MB_OK | MB_ICONINFORMATION);
            return 0;

        case ID_BTN_SAVE:
            collect_params_from_fields();
            if (save_params_to_csv(G.p, L"input_data\\input.csv"))
                MessageBoxW(hwnd, L"Сохранено в input_data\\input.csv", L"OK", MB_OK | MB_ICONINFORMATION);
            else
                MessageBoxW(hwnd, L"Ошибка записи CSV", L"Ошибка", MB_OK | MB_ICONERROR);
            return 0;

        case ID_BTN_NEW_RUN:
            collect_params_from_fields();
            create_new_out_dir();       // новая папка
            run_all_into_current_dir(); // расчёт в неё
            MessageBoxW(hwnd, (L"Расчёт завершён.\nПапка: " + G.outDir).c_str(),
                        L"OK", MB_OK | MB_ICONINFORMATION);
            return 0;

        case ID_BTN_RECALC:
            collect_params_from_fields();
            if (G.outDir.empty())
                create_new_out_dir();
            run_all_into_current_dir(); // пересчёт в текущую папку
            MessageBoxW(hwnd, (L"Пересчитано в текущую папку:\n" + G.outDir).c_str(),
                        L"OK", MB_OK | MB_ICONINFORMATION);
            return 0;

        case ID_BTN_ROLLBACK:
            if (G.history.size() >= 2)
            {
                G.history.pop_back();        // удаляем текущую
                G.outDir = G.history.back(); // активной становится предыдущая
                update_summary_text();
                MessageBoxW(hwnd, (L"Откат к папке:\n" + G.outDir).c_str(),
                            L"Откат", MB_OK | MB_ICONINFORMATION);
            }
            else
            {
                MessageBoxW(hwnd, L"Нет предыдущей папки в истории.",
                            L"Откат", MB_OK | MB_ICONWARNING);
            }
            return 0;

        case ID_BTN_PV:
            if (!G.has_ind)
            {
                MessageBoxW(hwnd, L"Сначала рассчитай.", L"Нет данных", MB_OK | MB_ICONWARNING);
                return 0;
            }
            ShowPlotWindow(L"Индикаторная P–V", &PaintPV);
            return 0;
        case ID_BTN_PALPHA:
            if (!G.has_pal)
            {
                MessageBoxW(hwnd, L"Сначала рассчитай.", L"Нет данных", MB_OK | MB_ICONWARNING);
                return 0;
            }
            ShowPlotWindow(L"Развёртка P(α)", &PaintPAlpha);
            return 0;
        case ID_BTN_FORCES:
            if (!G.has_fr)
            {
                MessageBoxW(hwnd, L"Сначала рассчитай.", L"Нет данных", MB_OK | MB_ICONWARNING);
                return 0;
            }
            ShowPlotWindow(L"Силы и момент в КШМ", &PaintForces);
            return 0;
        case ID_BTN_VDS:
            if (!G.has_vds)
            {
                MessageBoxW(hwnd, L"Сначала рассчитай.", L"Нет данных", MB_OK | MB_ICONWARNING);
                return 0;
            }
            ShowPlotWindow(L"ВДС шатунной шейки", &PaintVDS);
            return 0;

        case ID_BTN_STL:
        {
            std::wstring path = G.outDir.empty() ? L"output\\crank_mm.stl" : (G.outDir + L"\\crank_mm.stl");
            export_crank_STL_mm(G.p, wide_to_utf8(path), 128);
            MessageBoxW(hwnd, (L"Сохранено: " + path).c_str(), L"STL", MB_OK | MB_ICONINFORMATION);
            return 0;
        }
        case ID_BTN_STL_CW:
        {
            std::wstring path = G.outDir.empty() ? L"output\\crank_with_cw_mm.stl" : (G.outDir + L"\\crank_with_cw_mm.stl");
            G.cm = calc_mass_crankshaft(G.p);
            G.has_cm = true;
            CWVariant var = CWVariant::FullSupport_V1;
            if ((int)G.p.config_prot == 2)
                var = CWVariant::FullSupport_V2;
            else if ((int)G.p.config_prot == 3)
                var = CWVariant::SemiSupport;
            G.cw = build_counterweights_and_export(G.p, G.cm, var, wide_to_utf8(path), 128, true);
            G.has_cw = G.cw.ok;
            update_summary_text();
            std::wstring msg = L"Сохранено: " + path + L"\nS_prot=" +
                               fmt_double(G.cw.S_prot, 6) + L" кг·м, α=" +
                               fmt_double(G.cw.alpha_deg, 3) + L"°";
            if (!G.cw.message.empty())
            {
                msg += L"\n";
                msg += utf8_to_wide(G.cw.message);
            }
            MessageBoxW(hwnd, msg.c_str(), L"STL + CW", MB_OK | MB_ICONINFORMATION);
            return 0;
        }
        }
        break;
    }

    case WM_SIZE:
    {
        int W = LOWORD(lp), H = HIWORD(lp);
        if (g_summaryEdit)
            MoveWindow(g_summaryEdit, 10, H - 240, W - 40, 220, TRUE);
        return 0;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

//==================== Точка входа GUI ====================
int run_gui()
{
    SetProcessDPIAware();
    GdiplusStartupInput gsi;
    ULONG_PTR gtoken = 0;
    if (GdiplusStartup(&gtoken, &gsi, nullptr) != Ok)
        return 1;

    G.p = input(); // загрузка стандартных параметров
    ensure_dir(L"output");
    create_new_out_dir(); // сразу создаём первую папку

    HINSTANCE hInst = GetModuleHandleW(nullptr);
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = MainWndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"MainWndClass";
    RegisterClassExW(&wc);

    HWND hMain = CreateWindowExW(0, L"MainWndClass", L"Engine GUI — WinAPI/GDI+",
                                 WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                 CW_USEDEFAULT, CW_USEDEFAULT, 1030, 820,
                                 nullptr, nullptr, hInst, nullptr);
    ShowWindow(hMain, SW_SHOW);
    UpdateWindow(hMain);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    GdiplusShutdown(gtoken);
    return 0;
}
