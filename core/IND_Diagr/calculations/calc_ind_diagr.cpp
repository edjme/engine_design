#include "calc_ind_diagr.h"
#include <cmath>
#include <fstream>
#include <sstream>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#endif

namespace
{
    constexpr double PI = 3.1415926535897932384626433832795;

    inline double sqr(double x) { return x * x; }

    // -- HTML с Canvas-графиком P–V --
    void save_indicator_html(const std::string &path,
                             const std::vector<double> &V,
                             const std::vector<double> &P,
                             const IndicatorResults &R)
    {
        // Давление в МПа для графика
        std::vector<double> Pmpa;
        Pmpa.reserve(P.size());
        for (double x : P)
            Pmpa.push_back(x * 1e-6);

        auto minmax = [](const std::vector<double> &v)
        {
            auto it = std::minmax_element(v.begin(), v.end());
            return std::pair<double, double>(v.empty() ? 0 : *it.first, v.empty() ? 0 : *it.second);
        };
        auto [Vmin0, Vmax0] = minmax(V);
        auto [Pmin0, Pmax0] = minmax(Pmpa);
        double Vmin = Vmin0, Vmax = Vmax0, Pmin = Pmin0, Pmax = Pmax0;

        double Vpad = (Vmax - Vmin) * 0.05;
        double Ppad = (Pmax - Pmin) * 0.10;
        if (Vpad == 0)
            Vpad = (Vmax == 0 ? 1.0 : std::abs(Vmax) * 0.05);
        if (Ppad == 0)
            Ppad = (Pmax == 0 ? 0.1 : std::abs(Pmax) * 0.10);
        Vmin -= Vpad;
        Vmax += Vpad;
        Pmin -= Ppad;
        Pmax += Ppad;

        std::ofstream f(path);
        if (!f)
            return;

        f <<
            R"(<!doctype html>
<html lang="ru">
<head>
  <meta charset="utf-8">
  <title>Индикаторная диаграмма P–V</title>
  <style>
    body{font-family:system-ui,-apple-system,Segoe UI,Roboto,Arial,sans-serif;margin:0;padding:16px;background:#0b0d0f;color:#e8e8e8;}
    .card{max-width:1000px;margin:auto;background:#121417;border:1px solid #23262d;border-radius:12px;padding:16px;box-shadow:0 10px 30px rgba(0,0,0,.35);}
    h1{margin:0 0 12px 0;font-size:20px}
    canvas{width:100%;height:600px;background:#0e1114;border-radius:8px}
    .meta{font-size:12px;color:#aab;}
    .row{display:flex;gap:16px;flex-wrap:wrap}
    .box{flex:1 1 240px;background:#0e1114;border:1px solid #23262d;border-radius:8px;padding:8px}
    .box h3{margin:0 0 6px 0;font-size:13px;color:#c7d2fe}
    code{color:#cbd5e1}
  </style>
</head>
<body>
<div class="card">
  <h1>Индикаторная диаграмма P–V</h1>
  <div class="meta">P — МПа, V — м³. Контур построен по параметрам из CSV.</div>
  <canvas id="pv" width="1000" height="600"></canvas>
  <div class="row" style="margin-top:12px">
    <div class="box"><h3>Опорные объёмы</h3>
      <div>V<sub>a</sub> = )"
          << R.Va << R"( м³</div>
      <div>V<sub>c</sub> = )"
          << R.Vc << R"( м³</div>
      <div>V<sub>h</sub> = )"
          << R.Vh << R"( м³</div>
    </div>
    <div class="box"><h3>Опорные давления</h3>
      <div>P<sub>a</sub> = )"
          << R.Pa * 1e-6 << R"( МПа</div>
      <div>P<sub>c</sub> = )"
          << R.Pc * 1e-6 << R"( МПа</div>
      <div>P<sub>z</sub> = )"
          << R.Pz * 1e-6 << R"( МПа</div>
      <div>P<sub>b</sub> = )"
          << R.Pb * 1e-6 << R"( МПа</div>
    </div>
    <div class="box"><h3>Интегралы</h3>
      <div>A<sub>cycle</sub> = )"
          << R.A_cycle << R"( Дж</div>
      <div>Q<sub>in</sub>    = )"
          << R.Q_in << R"( Дж</div>
      <div>η = )"
          << R.eta << R"(</div>
    </div>
  </div>
</div>
<script>
const V = )";

        // массивы как JSON
        auto dump = [&](const std::vector<double> &v)
        {
            f << "[";
            for (size_t i = 0; i < v.size(); ++i)
            {
                if (i)
                    f << ",";
                f << v[i];
            }
            f << "]";
        };
        dump(V);
        f << ";\nconst P = ";
        dump(Pmpa);
        f << ";\n";
        f << "const Vmin=" << Vmin << ", Vmax=" << Vmax << ", Pmin=" << Pmin << ", Pmax=" << Pmax << ";\n";

        f <<
            R"(const canvas = document.getElementById('pv');
const ctx = canvas.getContext('2d');
const W = canvas.width, H = canvas.height;
const padL = 70, padR = 20, padT = 20, padB = 60;
const w = W - padL - padR, h = H - padT - padB;

function xMap(v){ return padL + ( (v - Vmin) / (Vmax - Vmin) ) * w; }
function yMap(p){ return padT + (1 - ( (p - Pmin) / (Pmax - Pmin) )) * h; }

// фон
ctx.fillStyle = '#0e1114'; ctx.fillRect(0,0,W,H);

// рамка + сетка
ctx.strokeStyle = '#334155'; ctx.lineWidth = 1; ctx.strokeRect(padL, padT, w, h);
ctx.strokeStyle = '#293241'; ctx.beginPath();
for(let i=1;i<5;i++){
  let xx = padL + i*w/5, yy = padT + i*h/5;
  ctx.moveTo(xx, padT); ctx.lineTo(xx, padT+h);
  ctx.moveTo(padL, yy); ctx.lineTo(padL+w, yy);
}
ctx.stroke();

// подписи осей
ctx.fillStyle = '#aab'; ctx.font = '12px system-ui';
ctx.fillText('V (м³)', padL + w/2 - 20, H - 30);
ctx.save(); ctx.translate(20, padT + h/2 + 30); ctx.rotate(-Math.PI/2);
ctx.fillText('P (МПа)', 0, 0); ctx.restore();

// деления (5 шт)
ctx.fillStyle = '#cbd5e1';
ctx.textAlign = 'center';
for(let i=0;i<=5;i++){
  let vv = Vmin + (Vmax - Vmin)*i/5;
  let xx = padL + (w * i/5);
  ctx.fillText(vv.toExponential(2), xx, H - 40);
}
ctx.textAlign = 'right';
for(let i=0;i<=5;i++){
  let pp = Pmin + (Pmax - Pmin)*i/5;
  let yy = padT + h - (h * i/5);
  ctx.fillText(pp.toFixed(2), padL - 8, yy+4);
}

// линия контура
ctx.strokeStyle = '#60a5fa'; ctx.lineWidth = 2;
ctx.beginPath();
for(let i=0;i<V.length;i++){
  let x = xMap(V[i]), y = yMap(P[i]);
  if(i===0) ctx.moveTo(x,y); else ctx.lineTo(x,y);
}
ctx.stroke();

// ключевые точки (a,c,z,z',b)
function dot(v,p,color){ ctx.beginPath(); ctx.fillStyle=color; ctx.arc(xMap(v),yMap(p),4,0,6.283); ctx.fill(); }
)";

        // маркеры ключевых точек
        f << "dot(" << R.Va << "," << R.Pa * 1e-6 << ", '#34d399');\n";   // a
        f << "dot(" << R.Vc << "," << R.Pc * 1e-6 << ", '#fbbf24');\n";   // c
        f << "dot(" << R.Vz << "," << R.Pz * 1e-6 << ", '#f59e0b');\n";   // z
        f << "dot(" << R.Vz_ << "," << R.Pz_ * 1e-6 << ", '#fb7185');\n"; // z'
        f << "dot(" << R.Va << "," << R.Pb * 1e-6 << ", '#22d3ee');\n";   // b

        f << "</script>\n</body>\n</html>\n";
    }

    // собрать путь: конкатенация V_i/P_i в общий массив
    inline void append_path(std::vector<double> &Vdst, std::vector<double> &Pdst,
                            const std::vector<double> &Vs, const std::vector<double> &Ps)
    {
        Vdst.insert(Vdst.end(), Vs.begin(), Vs.end());
        Pdst.insert(Pdst.end(), Ps.begin(), Ps.end());
    }
} // namespace

// -- основной расчёт P–V + CSV + HTML --
IndicatorResults build_indicator_PV(const IndParams &p,
                                    int samples_per_segment,
                                    const std::string &output_csv_path,
                                    const std::string &output_html_path,
                                    bool auto_open_html)
{
    IndicatorResults R{};

    // 1) Геометрия и базовые давления
    const double D = p.diam_cyl;
    const double S = p.radcrank * 2;
    const double eps = p.epsilent;
    const bool is2T = (std::lround(p.tau) == 2);

    R.F_p = PI * D * D / 4.0;
    R.Vh = S * R.F_p;
    R.Vc = R.Vh / (eps - 1.0);
    R.Va = R.Vh + R.Vc;

    R.Pa = p.p_a;
    R.Pr = p.p_r;

    const double n1 = p.n_1;
    const double n2 = p.n_2;

    // 2) Ключевые точки по принятой схеме
    R.Pc = R.Pa * std::pow(eps, n1); // c
    R.Vz = R.Vc;
    R.Pz = R.Pc * p.lymbda_z; // z
    R.Vz_ = R.Vz * p.ro;      // z' (изобарное «предрасширение»)
    R.Pz_ = R.Pz;
    R.Pb = R.Pz_ * std::pow(R.Vz_ / R.Va, n2); // b

    auto reserve = [&](std::vector<double> &V, std::vector<double> &P, int N)
    {
        V.clear();
        P.clear();
        V.reserve(N);
        P.reserve(N);
    };
    const int N = max(8, samples_per_segment);

    // 3) Гладкие участки
    // a -> c (сжатие)
    reserve(R.V_comp, R.P_comp, N);
    for (int i = 0; i < N; ++i)
    {
        double t = double(i) / (N - 1);
        double V = R.Va + (R.Vc - R.Va) * t;
        double P = R.Pa * std::pow(R.Va / V, n1);
        R.V_comp.push_back(V);
        R.P_comp.push_back(P);
    }
    // c -> z (изохора)
    reserve(R.V_iso_add, R.P_iso_add, 2);
    R.V_iso_add.push_back(R.Vc);
    R.P_iso_add.push_back(R.Pc);
    R.V_iso_add.push_back(R.Vc);
    R.P_iso_add.push_back(R.Pz);
    // z -> z' (изобара при Pz)
    reserve(R.V_preexp, R.P_preexp, 2);
    R.V_preexp.push_back(R.Vz);
    R.P_preexp.push_back(R.Pz);
    R.V_preexp.push_back(R.Vz_);
    R.P_preexp.push_back(R.Pz);
    // z' -> b (расширение n2)
    reserve(R.V_exp, R.P_exp, N);
    for (int i = 0; i < N; ++i)
    {
        double t = double(i) / (N - 1);
        double V = R.Vz_ + (R.Va - R.Vz_) * t;
        double P = R.Pz_ * std::pow(R.Vz_ / V, n2);
        R.V_exp.push_back(V);
        R.P_exp.push_back(P);
    }

    // 4) Развилка по тактности и сборка контура
    R.V_path.clear();
    R.P_path.clear();

    if (!is2T)
    {
        // ===== 4-тактный (720°)
        // b -> r' (вертикаль к Pr при Va)
        reserve(R.V_drop_b, R.P_drop_b, 2);
        R.V_drop_b.push_back(R.Va);
        R.P_drop_b.push_back(R.Pb);
        R.V_drop_b.push_back(R.Va);
        R.P_drop_b.push_back(R.Pr);

        // r' -> r (выпуск по Pr: Va -> Vc)
        reserve(R.V_exh, R.P_exh, 2);
        R.V_exh.push_back(R.Va);
        R.P_exh.push_back(R.Pr);
        R.V_exh.push_back(R.Vc);
        R.P_exh.push_back(R.Pr);

        // r -> r'' (вертикаль к Pa при Vc)
        reserve(R.V_drop_r, R.P_drop_r, 2);
        R.V_drop_r.push_back(R.Vc);
        R.P_drop_r.push_back(R.Pr);
        R.V_drop_r.push_back(R.Vc);
        R.P_drop_r.push_back(R.Pa);

        // r'' -> a (впуск по Pa: Vc -> Va)
        reserve(R.V_int, R.P_int, 2);
        R.V_int.push_back(R.Vc);
        R.P_int.push_back(R.Pa);
        R.V_int.push_back(R.Va);
        R.P_int.push_back(R.Pa);

        // Сборка
        append_path(R.V_path, R.P_path, R.V_comp, R.P_comp);
        append_path(R.V_path, R.P_path, R.V_iso_add, R.P_iso_add);
        append_path(R.V_path, R.P_path, R.V_preexp, R.P_preexp);
        append_path(R.V_path, R.P_path, R.V_exp, R.P_exp);
        append_path(R.V_path, R.P_path, R.V_drop_b, R.P_drop_b);
        append_path(R.V_path, R.P_path, R.V_exh, R.P_exh);
        append_path(R.V_path, R.P_path, R.V_drop_r, R.P_drop_r);
        append_path(R.V_path, R.P_path, R.V_int, R.P_int);
    }
    else
    {
        // ===== 2-тактный (360°): упрощённый цикл
        // блоудаун (вертикаль до Pr при Va)
        reserve(R.V_drop_b, R.P_drop_b, 2);
        R.V_drop_b.push_back(R.Va);
        R.P_drop_b.push_back(R.Pb);
        R.V_drop_b.push_back(R.Va);
        R.P_drop_b.push_back(R.Pr);

        // подъём до Pa при Va (условно «наполнение»)
        reserve(R.V_drop_r, R.P_drop_r, 2);
        R.V_drop_r.push_back(R.Va);
        R.P_drop_r.push_back(R.Pr);
        R.V_drop_r.push_back(R.Va);
        R.P_drop_r.push_back(R.Pa);

        // Сборка
        append_path(R.V_path, R.P_path, R.V_comp, R.P_comp);
        append_path(R.V_path, R.P_path, R.V_iso_add, R.P_iso_add);
        append_path(R.V_path, R.P_path, R.V_preexp, R.P_preexp);
        append_path(R.V_path, R.P_path, R.V_exp, R.P_exp);
        append_path(R.V_path, R.P_path, R.V_drop_b, R.P_drop_b);
        append_path(R.V_path, R.P_path, R.V_drop_r, R.P_drop_r);
    }

    // 5) Индикаторная работа за цикл (по контуру)
    R.A_cycle = 0.0;
    for (size_t i = 1; i < R.V_path.size(); ++i)
    {
        double dV = R.V_path[i] - R.V_path[i - 1];
        double Pavg = 0.5 * (R.P_path[i] + R.P_path[i - 1]);
        R.A_cycle += Pavg * dV;
    }

    // 6) Подвод теплоты и КПД (в рамках нашей идеализированной схемы)
    R.Q_in = (R.Pz - R.Pc) * R.Vc + R.Pz * (R.Vz_ - R.Vz);
    R.eta = (R.Q_in != 0.0 ? R.A_cycle / R.Q_in : 0.0);

    // 7) CSV
    if (!output_csv_path.empty())
    {
        std::ofstream fcsv(output_csv_path);
        if (fcsv)
        {
            fcsv << "V,P\n";
            for (size_t i = 0; i < R.V_path.size(); ++i)
            {
                fcsv << R.V_path[i] << "," << R.P_path[i] << "\n";
            }
        }
    }

    // 8) HTML
    if (!output_html_path.empty())
    {
        save_indicator_html(output_html_path, R.V_path, R.P_path, R);
#ifdef _WIN32
        if (auto_open_html)
        {
            ShellExecuteA(nullptr, "open", output_html_path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        }
#endif
    }

    // 9) Summary
    std::ostringstream ss;
    ss << "Indicator PV built.\n"
       << "Va=" << R.Va << " m^3, Vc=" << R.Vc << " m^3, Vh=" << R.Vh << " m^3\n"
       << "Pc=" << R.Pc << " Pa, Pz=" << R.Pz << " Pa, Pz'=" << R.Pz_ << " Pa, Pb=" << R.Pb << " Pa\n"
       << "A_cycle=" << R.A_cycle << " J per cyl, Q_in=" << R.Q_in << " J, eta=" << R.eta << "\n";
    R.summary = ss.str();

    return R;
}