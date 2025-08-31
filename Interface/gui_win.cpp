// Unicode GUI for ENGINE_DESIGN (WinAPI + GDI+)
// Главная форма + отдельные окна-графики (без браузера)

#include <windows.h>
#include <commctrl.h>
#include <gdiplus.h>
#include <string>
#include <vector>
#include <functional>
#include <sstream>
#include <iomanip>
#include <algorithm>

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "uuid.lib")

// ==== расчётные модули проекта ====
#include "input_data/input.h"
#include "Calculations/ind_diagr.h"
#include "Calculations/diag_palpha.h"
#include "Calculations/forces_ksm.h"
#include "Calculations/vds_crankpin.h"
#include "Calculations/calc_mass_crankshaft.h"
#include "Calculations/cw_counterweights.h"

// -------------------- утилиты --------------------
static std::wstring utf8_to_w(const std::string &s)
{
    if (s.empty())
        return L"";
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    std::wstring w(n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), w.data(), n);
    return w;
}
static std::wstring to_w(double v, int prec = 6)
{
    std::wstringstream ss;
    ss.setf(std::ios::fixed);
    ss << std::setprecision(prec) << v;
    return ss.str();
}

// -------------------- глобальное состояние --------------------
struct AppState
{
    Params p{};
    IndicatorResults ind;
    PAlphaResults pal;
    ForcesResults fr;
    VDSCrankpinResults vds;
    CrankshaftMassResults cm;
    CWResult cw;
    bool has_calc = false;
} G;

static HFONT gFont = nullptr;

// -------------------- окно-Plot --------------------
struct PlotPayload
{
    std::function<void(HWND)> onPaint;
    std::wstring title;
};

static LRESULT CALLBACK PlotWndProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l)
{
    auto *pl = (PlotPayload *)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    switch (msg)
    {
    case WM_CREATE:
    {
        CREATESTRUCTW *cs = (CREATESTRUCTW *)l;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)cs->lpCreateParams);
        if (gFont)
            SendMessageW(hwnd, WM_SETFONT, (WPARAM)gFont, TRUE);
    }
        return 0;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        BeginPaint(hwnd, &ps);
        if (pl && pl->onPaint)
            pl->onPaint(hwnd);
        EndPaint(hwnd, &ps);
    }
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, w, l);
}

static int ShowPlotWindow(const std::wstring &title,
                          std::function<void(HWND)> onPaint,
                          int width = 1100, int height = 720)
{
    HINSTANCE hi = GetModuleHandleW(nullptr);
    static bool once = false;
    if (!once)
    {
        WNDCLASSEXW wc{sizeof(WNDCLASSEXW)};
        wc.lpfnWndProc = PlotWndProc;
        wc.hInstance = hi;
        wc.lpszClassName = L"PlotWndClass";
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        RegisterClassExW(&wc);
        once = true;
    }
    PlotPayload payload{onPaint, title};
    HWND hwnd = CreateWindowExW(
        0, L"PlotWndClass", title.c_str(),
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, width, height,
        nullptr, nullptr, hi, &payload);

    MSG m;
    while (GetMessageW(&m, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&m);
        DispatchMessageW(&m);
    }
    return (int)m.wParam;
}

// -------------------- рисование графиков (GDI+) --------------------
using namespace Gdiplus;

static void draw_axes(Graphics &g, const Rect &rc,
                      const std::wstring &xlab, const std::wstring &ylab)
{
    Pen grid(Color(255, 45, 45, 45), 1.f);
    Pen border(Color(255, 80, 80, 80), 1.f);
    SolidBrush bg(Color(255, 245, 247, 250));
    SolidBrush txt(Color(255, 30, 30, 30));
    FontFamily ff(L"Segoe UI");
    Font f(&ff, 12.f, FontStyleRegular, UnitPixel);

    g.FillRectangle(&bg, rc);
    g.DrawRectangle(&border, rc);

    // grid 6x5
    for (int i = 1; i < 6; i++)
    {
        int x = rc.X + (rc.Width * i) / 6;
        g.DrawLine(&grid, x, rc.Y, x, rc.Y + rc.Height);
    }
    for (int j = 1; j < 5; j++)
    {
        int y = rc.Y + (rc.Height * j) / 5;
        g.DrawLine(&grid, rc.X, y, rc.X + rc.Width, y);
    }
    // labels
    g.DrawString(xlab.c_str(), -1, &f, PointF(rc.X + rc.Width / 2.f - 30, rc.Y + rc.Height + 8), &txt);
    GraphicsState st = g.Save();
    g.TranslateTransform((REAL)(rc.X - 32), (REAL)(rc.Y + rc.Height / 2));
    g.RotateTransform(-90);
    g.DrawString(ylab.c_str(), -1, &f, PointF(0, 0), &txt);
    g.Restore(st);
}

template <class T>
static std::pair<double, double> vminmax(const std::vector<T> &v)
{
    if (v.empty())
        return {0, 1};
    auto [mi, ma] = std::minmax_element(v.begin(), v.end());
    if (*mi == *ma)
        return {*mi - 1, *ma + 1};
    return {*mi, *ma};
}

static void map_polyline(Graphics &g, const Rect &rc,
                         const std::vector<double> &X, const std::vector<double> &Y,
                         double xmin, double xmax, double ymin, double ymax,
                         Color color = Color(255, 58, 121, 254), float width = 2.f)
{
    if (X.empty() || Y.empty() || X.size() != Y.size())
        return;
    Pen pen(color, width);
    auto xf = [&](double x)
    { return rc.X + (INT)((x - xmin) / (xmax - xmin) * rc.Width); };
    auto yf = [&](double y)
    { return rc.Y + rc.Height - (INT)((y - ymin) / (ymax - ymin) * rc.Height); };
    for (size_t i = 1; i < X.size(); ++i)
    {
        g.DrawLine(&pen, xf(X[i - 1]), yf(Y[i - 1]), xf(X[i]), yf(Y[i]));
    }
}

// ---- конкретные «рисовальщики» ----
static void PaintPV(HWND hwnd)
{
    Graphics g(hwnd);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    RECT r;
    GetClientRect(hwnd, &r);
    Rect rc(80, 40, (r.right - r.left) - 120, (r.bottom - r.top) - 120);
    draw_axes(g, rc, L"V, м³", L"P, МПа");

    std::vector<double> V = G.ind.V_path;
    std::vector<double> Pmpa;
    Pmpa.reserve(G.ind.P_path.size());
    for (double p : G.ind.P_path)
        Pmpa.push_back(p * 1e-6);

    auto [vmin, vmax] = vminmax(V);
    auto [pmin, pmax] = vminmax(Pmpa);
    map_polyline(g, rc, V, Pmpa, vmin, vmax, pmin, pmax);
}

static void PaintPalpha(HWND hwnd)
{
    Graphics g(hwnd);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    RECT r;
    GetClientRect(hwnd, &r);
    Rect rc(80, 40, (r.right - r.left) - 120, (r.bottom - r.top) - 120);
    draw_axes(g, rc, L"α, град", L"P, МПа");

    std::vector<double> A = G.pal.alpha_deg;
    std::vector<double> P;
    P.reserve(G.pal.P_alpha.size());
    for (double p : G.pal.P_alpha)
        P.push_back(p * 1e-6);

    auto [amin, amax] = vminmax(A);
    auto [pmin, pmax] = vminmax(P);
    map_polyline(g, rc, A, P, amin, amax, pmin, pmax);
}

// fix: proper rc for forces
static void PaintForces(HWND hwnd)
{
    Graphics g(hwnd);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    RECT r;
    GetClientRect(hwnd, &r);
    Rect rc(80, 40, (r.right - r.left) - 120, (r.bottom - r.top) - 120);
    draw_axes(g, rc, L"α, град", L"Силы, Н / Момент, Н·м");

    auto [amin, amax] = vminmax(G.fr.alpha_deg);
    auto [fmin, fmax] = vminmax(G.fr.F_sum);
    auto zmm = vminmax(G.fr.M_cr);
    fmin = std::min(fmin, zmm.first);
    fmax = std::max(fmax, zmm.second);

    map_polyline(g, rc, G.fr.alpha_deg, G.fr.F_sum, amin, amax, fmin, fmax, Color(255, 58, 121, 254), 2.0f);
    map_polyline(g, rc, G.fr.alpha_deg, G.fr.F_in, amin, amax, fmin, fmax, Color(255, 31, 187, 115), 1.6f);
    map_polyline(g, rc, G.fr.alpha_deg, G.fr.M_cr, amin, amax, fmin, fmax, Color(255, 251, 191, 36), 1.6f);
}

static void PaintVDS(HWND hwnd)
{
    Graphics g(hwnd);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    RECT r;
    GetClientRect(hwnd, &r);
    Rect rc(80, 40, (r.right - r.left) - 120, (r.bottom - r.top) - 120);
    draw_axes(g, rc, L"X = Z + P′c, Н", L"Y = T, Н");

    const auto &X = G.vds.Z_shifted;
    const auto &Y = G.vds.T_same;
    auto [xmin, xmax] = vminmax(X);
    auto [ymin, ymax] = vminmax(Y);
    map_polyline(g, rc, X, Y, xmin, xmax, ymin, ymax, Color(255, 58, 121, 254), 2.0f);
}

// -------------------- пайплайн --------------------
static void run_pipeline(bool auto_open_html = false)
{
    G.p = input();
    G.ind = build_indicator_PV(G.p, 400, "output/ind_pv.csv", "output/ind_pv.html", auto_open_html);
    G.pal = build_P_alpha(G.p, G.ind, 0.5, "output/P_alpha.csv", "output/P_alpha.html", auto_open_html);
    G.fr = build_forces_ksm(G.p, G.pal, "output/forces_ksm.csv", "output/forces_ksm.html", auto_open_html);
    G.vds = build_vds_crankpin(G.p, G.fr, "output/vds_crankpin.csv", "output/vds_crankpin.html", auto_open_html);
    G.cm = calc_mass_crankshaft(G.p);
    export_crank_STL_mm(G.p, "output/crank_mm.stl", 128);

    CWVariant var = CWVariant::FullSupport_V1;
    const long vsel = (long)std::llround(G.p.config_prot);
    if (vsel == 2)
        var = CWVariant::FullSupport_V2;
    else if (vsel == 3)
        var = CWVariant::SemiSupport;
    G.cw = build_counterweights_and_export(G.p, G.cm, var, "output/crank_with_cw_mm.stl", 128, true);
    G.has_calc = true;
}

// -------------------- главное окно --------------------
enum : UINT
{
    ID_BTN_INPUT = 1001,
    ID_BTN_RUN,
    ID_BTN_PV,
    ID_BTN_PALPHA,
    ID_BTN_FORCES,
    ID_BTN_VDS,
    ID_BTN_STL_BASE,
    ID_BTN_STL_CW,
    ID_BTN_MOMENTS,
    ID_LIST_PARAMS
};

static void fill_params_list(HWND hList)
{
    SendMessageW(hList, LB_RESETCONTENT, 0, 0);
    auto add = [&](const std::wstring &k, double v)
    {
        std::wstring row = k + L"," + to_w(v, 9);
        SendMessageW(hList, LB_ADDSTRING, 0, (LPARAM)row.c_str());
    };
    // — выводим всё как раньше —
    const Params &p = G.p;
    add(L"diam_cyl", p.diam_cyl);
    add(L"stroke", p.stroke);
    add(L"epsilent", p.epsilent);
    add(L"p_a", p.p_a);
    add(L"p_r", p.p_r);
    add(L"n_1", p.n_1);
    add(L"n_2", p.n_2);
    add(L"lymbda_z", p.lymbda_z);
    add(L"ro", p.ro);
    add(L"lyambda", p.lyambda);
    add(L"n", p.n);
    add(L"m_pd", p.m_pd);
    add(L"r", p.r);
    add(L"leng_rod", p.leng_rod);
    add(L"m_rod", p.m_rod);
    add(L"m_2", p.m_2);
    add(L"w", p.w);
    add(L"tau", p.tau);
    add(L"count_cyl", p.count_cyl);
    add(L"gamma", p.gamma);
    add(L"diam_root_neck", p.diam_root_neck);
    add(L"diam_rod_neck", p.diam_rod_neck);
    add(L"length_rod_neck", p.length_rod_neck);
    add(L"length_root_neck", p.length_root_neck);
    add(L"depth_web", p.depth_web);
    add(L"fillet_rad", p.fillet_rad);
    add(L"width_web", p.width_web);
    add(L"dist_axes", p.dist_axes);
    add(L"dist_web", p.dist_web);
    add(L"rho_material", p.rho_material);
    add(L"config_crankshaft", p.config_crankshaft);
    add(L"config_prot", p.config_prot);
    add(L"depth_prot", p.depth_prot);
    add(L"r_prot1", p.r_prot1);
    add(L"r_prot2", p.r_prot2);
}

static void show_message(HWND hParent, const std::wstring &title, const std::wstring &text)
{
    MessageBoxW(hParent, text.c_str(), title.c_str(), MB_OK | MB_ICONINFORMATION);
}

static LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l)
{
    switch (msg)
    {
    case WM_CREATE:
    {
        HINSTANCE hi = (HINSTANCE)GetWindowLongPtrW(hwnd, GWLP_HINSTANCE);
        HWND hList = CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", L"",
                                     WS_CHILD | WS_VISIBLE | LBS_NOTIFY | WS_VSCROLL | WS_HSCROLL,
                                     250, 10, 900, 620, hwnd, (HMENU)ID_LIST_PARAMS, hi, nullptr);

        auto btn = [&](UINT id, int y, const wchar_t *txt)
        {
            HWND b = CreateWindowExW(0, L"BUTTON", txt,
                                     WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                     10, y, 220, 34, hwnd, (HMENU)(INT_PTR)id, hi, nullptr);
            SendMessageW(b, WM_SETFONT, (WPARAM)gFont, TRUE);
            return b;
        };
        btn(ID_BTN_INPUT, 10, L"Входные данные (перечитать CSV)");
        btn(ID_BTN_RUN, 50, L"Расчёт (полный пайплайн)");
        btn(ID_BTN_PV, 100, L"Показать индикаторную P–V");
        btn(ID_BTN_PALPHA, 140, L"Показать P(α)");
        btn(ID_BTN_FORCES, 180, L"Показать силы в КШМ");
        btn(ID_BTN_VDS, 220, L"Показать ВДС шейки");
        btn(ID_BTN_STL_BASE, 260, L"STL базового колена");
        btn(ID_BTN_STL_CW, 300, L"STL + противовесы");
        btn(ID_BTN_MOMENTS, 340, L"Статические моменты противовесов");

        G.p = input();
        fill_params_list(hList);
        if (gFont)
            SendMessageW(hList, WM_SETFONT, (WPARAM)gFont, TRUE);
    }
        return 0;

    case WM_COMMAND:
    {
        UINT id = LOWORD(w);
        switch (id)
        {
        case ID_BTN_INPUT:
        {
            G.p = input();
            fill_params_list(GetDlgItem(hwnd, ID_LIST_PARAMS));
            show_message(hwnd, L"Входные данные", L"CSV перечитан и отображён.");
        }
        break;
        case ID_BTN_RUN:
        {
            run_pipeline(false);
            fill_params_list(GetDlgItem(hwnd, ID_LIST_PARAMS));
            show_message(hwnd, L"Расчёт", L"Полный расчёт выполнен. Файлы в папке output.");
        }
        break;
        case ID_BTN_PV:
        {
            if (!G.has_calc)
            {
                run_pipeline(false);
            }
            ShowPlotWindow(L"P–V: индикаторная диаграмма", PaintPV, 1100, 700);
        }
        break;
        case ID_BTN_PALPHA:
        {
            if (!G.has_calc)
            {
                run_pipeline(false);
            }
            ShowPlotWindow(L"P(α): развёртка", PaintPalpha, 1100, 700);
        }
        break;
        case ID_BTN_FORCES:
        {
            if (!G.has_calc)
            {
                run_pipeline(false);
            }
            ShowPlotWindow(L"Силы в КШМ", PaintForces, 1100, 700);
        }
        break;
        case ID_BTN_VDS:
        {
            if (!G.has_calc)
            {
                run_pipeline(false);
            }
            ShowPlotWindow(L"ВДС шатунной шейки", PaintVDS, 1100, 700);
        }
        break;
        case ID_BTN_STL_BASE:
        {
            export_crank_STL_mm(G.p, "output/crank_mm.stl", 128);
            show_message(hwnd, L"STL", L"Сохранено: output\\crank_mm.stl");
        }
        break;
        case ID_BTN_STL_CW:
        {
            CWVariant var = CWVariant::FullSupport_V1;
            const long vsel = (long)std::llround(G.p.config_prot);
            if (vsel == 2)
                var = CWVariant::FullSupport_V2;
            else if (vsel == 3)
                var = CWVariant::SemiSupport;
            if (!G.has_calc)
                G.cm = calc_mass_crankshaft(G.p);
            G.cw = build_counterweights_and_export(G.p, G.cm, var, "output/crank_with_cw_mm.stl", 128, true);
            std::wstringstream ss;
            ss << L"Сохранено: output\\crank_with_cw_mm.stl\n"
               << L"S_prot=" << to_w(G.cw.S_prot) << L" кг·м, α=" << to_w(G.cw.alpha_deg) << L"°\n"
               << utf8_to_w(G.cw.message);
            show_message(hwnd, L"STL + противовесы", ss.str());
        }
        break;
        case ID_BTN_MOMENTS:
        {
            if (!G.has_calc)
                run_pipeline(false);
            std::wstringstream ss;
            const double Fp = 3.141592653589793 * (G.p.diam_cyl * G.p.diam_cyl) / 4.0;
            ss << L"Fp=" << to_w(Fp) << L" м²\n"
               << L"m_root_reduce=" << to_w(G.cm.m_root_reduce) << L" кг/м²\n"
               << L"m_rotating=" << to_w(G.cm.m_rotating) << L" кг/м²\n"
               << L"axis_p=" << to_w(G.cm.axis_p) << L", web_p=" << to_w(G.cm.web_p)
               << L"\naxis_n=" << to_w(G.cm.axis_n) << L", web_n=" << to_w(G.cm.web_n)
               << L"\nS_prot (последний расчёт CW)=" << to_w(G.cw.S_prot) << L" кг·м, α=" << to_w(G.cw.alpha_deg) << L"°";
            show_message(hwnd, L"Статические моменты", ss.str());
        }
        break;
        }
    }
    break;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, w, l);
}

// -------------------- запуск GUI --------------------
int run_gui()
{
    SetProcessDPIAware();

    // общий шрифт
    gFont = CreateFontW(-18, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                        0, 0, 0, 0, L"Segoe UI");

    // GDI+
    ULONG_PTR gdipToken = 0;
    Gdiplus::GdiplusStartupInput gsi;
    Gdiplus::GdiplusStartup(&gdipToken, &gsi, nullptr);

    // главное окно
    HINSTANCE hi = GetModuleHandleW(nullptr);
    WNDCLASSEXW wc{sizeof(WNDCLASSEXW)};
    wc.lpfnWndProc = MainWndProc;
    wc.hInstance = hi;
    wc.lpszClassName = L"EngineDesignWnd";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassExW(&wc);

    HWND hwnd = CreateWindowExW(0, wc.lpszClassName, L"ENGINE DESIGN — GUI",
                                WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                CW_USEDEFAULT, CW_USEDEFAULT, 1200, 720,
                                nullptr, nullptr, hi, nullptr);

    MSG m;
    while (GetMessageW(&m, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&m);
        DispatchMessageW(&m);
    }

    if (gFont)
    {
        DeleteObject(gFont);
        gFont = nullptr;
    }
    Gdiplus::GdiplusShutdown(gdipToken);
    return (int)m.wParam;
}
