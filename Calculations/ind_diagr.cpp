#include "ind_diagr.h"
#include <cmath>
#include <fstream>
#include <sstream>
#include <algorithm>

#ifdef _WIN32
  #include <windows.h>
#endif

static inline double sqr(double x){ return x*x; }

// -- внутренний помощник: запись простого HTML с Canvas-графиком --
static void save_indicator_html(const std::string& path,
                                const std::vector<double>& V,
                                const std::vector<double>& P,
                                const IndicatorResults& R)
{
    // Конвертируем давления в МПа для удобства отображения
    std::vector<double> Pmpa; Pmpa.reserve(P.size());
    for (double x : P) Pmpa.push_back(x * 1e-6);

    // Поиск диапазонов
    auto [VminIt, VmaxIt] = std::minmax_element(V.begin(), V.end());
    auto [PminIt, PmaxIt] = std::minmax_element(Pmpa.begin(), Pmpa.end());
    double Vmin=*VminIt, Vmax=*VmaxIt, Pmin=*PminIt, Pmax=*PmaxIt;

    // Небольшие поля
    double Vpad = (Vmax - Vmin) * 0.05;
    double Ppad = (Pmax - Pmin) * 0.10;
    if (Vpad == 0) Vpad = (Vmax==0? 1.0: std::abs(Vmax)*0.05);
    if (Ppad == 0) Ppad = (Pmax==0? 0.1: std::abs(Pmax)*0.10);
    Vmin -= Vpad; Vmax += Vpad;
    Pmin -= Ppad; Pmax += Ppad;

    std::ofstream f(path);
    if (!f) return;

    // Простая страница с Canvas и рисованием полилинии
    f <<
R"(<!doctype html>
<html lang="ru"><head<meta charset="utf-8">
<title>Индикаторная диаграмма P–V</title>
<meta charset="utf-8">
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
)</" << "head><body>\n"
<< R"(<div class="card">
  <h1>Индикаторная диаграмма P–V</h1>
  <div class="meta">P — в МПа, V — в м³. Контур построен по параметрам из CSV.</div>
  <canvas id="pv" width="1000" height="600"></canvas>
  <div class="row" style="margin-top:12px">
    <div class="box"><h3>Опорные объёмы</h3>
      <div>V<sub>a</sub> = )" << R.Va << R"( м³</div>
      <div>V<sub>c</sub> = )" << R.Vc << R"( м³</div>
      <div>V<sub>h</sub> = )" << R.Vh << R"( м³</div>
    </div>
    <div class="box"><h3>Опорные давления</h3>
      <div>P<sub>a</sub> = )" << R.Pa*1e-6 << R"( МПа</div>
      <div>P<sub>c</sub> = )" << R.Pc*1e-6 << R"( МПа</div>
      <div>P<sub>z</sub> = )" << R.Pz*1e-6 << R"( МПа</div>
      <div>P<sub>b</sub> = )" << R.Pb*1e-6 << R"( МПа</div>
    </div>
    <div class="box"><h3>Интегралы</h3>
      <div>A<sub>cycle</sub> = )" << R.A_cycle << R"( Дж</div>
      <div>Q<sub>in</sub>    = )" << R.Q_in   << R"( Дж</div>
      <div>η = )" << R.eta << R"(</div>
    </div>
  </div>
</div>
<script>
const V = )";

    // Массивы данных встраиваем как JSON
    f << "[";
    for (size_t i=0;i<V.size();++i){ if (i) f<<","; f<<V[i]; }
    f << "];\nconst P = [";
    for (size_t i=0;i<Pmpa.size();++i){ if (i) f<<","; f<<Pmpa[i]; }
    f << "];\n";

    f << "const Vmin="<<Vmin<<", Vmax="<<Vmax<<", Pmin="<<Pmin<<", Pmax="<<Pmax<<";\n";
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

// оси
ctx.strokeStyle = '#334155'; ctx.lineWidth = 1;
ctx.beginPath();
// рамка
ctx.rect(padL, padT, w, h);
// вспомогательные сетки (5×5)
for(let i=1;i<5;i++){
  let xx = padL + i*w/5, yy = padT + i*h/5;
  ctx.moveTo(xx, padT); ctx.lineTo(xx, padT+h);
  ctx.moveTo(padL, yy); ctx.lineTo(padL+w, yy);
}
ctx.stroke();

// подписи осей
ctx.fillStyle = '#aab';
ctx.font = '12px system-ui';
ctx.fillText('V (м³)', padL + w/2 - 20, H - 30);
ctx.save();
ctx.translate(20, padT + h/2 + 30);
ctx.rotate(-Math.PI/2);
ctx.fillText('P (МПа)', 0, 0);
ctx.restore();

// метки по осям (5 делений)
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
ctx.strokeStyle = '#60a5fa'; // лазурный
ctx.lineWidth = 2;
ctx.beginPath();
for(let i=0;i<V.length;i++){
  let x = xMap(V[i]), y = yMap(P[i]);
  if(i===0) ctx.moveTo(x,y); else ctx.lineTo(x,y);
}
ctx.stroke();

// точки a, c, z, z', b (пример маркеров)
function dot(v,p,color){ ctx.fillStyle=color; ctx.beginPath(); ctx.arc(xMap(v),yMap(p),4,0,6.283); ctx.fill(); }
)";

    // Маркеры ключевых точек
    f << "dot("<<R.Va<<","<<R.Pa*1e-6<<", '#34d399'); // a\n";
    f << "dot("<<R.Vc<<","<<R.Pc*1e-6<<", '#fbbf24'); // c\n";
    f << "dot("<<R.Vz<<","<<R.Pz*1e-6<<", '#f59e0b'); // z\n";
    f << "dot("<<R.Vz_<<","<<R.Pz_*1e-6<<", '#fb7185'); // z'\n";
    f << "dot("<<R.Va<<","<<R.Pb*1e-6<<", '#22d3ee'); // b\n";

    f << "</script>\n</body></html>";
}

// -- основной расчёт P–V, + CSV, + HTML --
IndicatorResults build_indicator_PV(const Params& p,
                                    int samples_per_segment,
                                    const std::string& output_csv_path,
                                    const std::string& output_html_path,
                                    bool auto_open_html)
{
    IndicatorResults R{};

    // 1) Геометрия
    const double D   = p.diam_cyl;
    const double S   = p.stroke;
    const double eps = p.epsilent;

    R.F_p = M_PI * D*D / 4.0;
    R.Vh  = S * R.F_p;
    R.Vc  = R.Vh / (eps - 1.0);
    R.Va  = R.Vh + R.Vc;

    // 2) Давления
    R.Pa = p.p_a;
    R.Pr = p.p_r;

    const double n1 = p.n_1;
    const double n2 = p.n_2;

    R.Pc  = R.Pa * std::pow(eps, n1);
    R.Vz  = R.Vc;
    R.Pz  = R.Pc * p.lymbda_z;
    R.Vz_ = R.Vz * p.ro;
    R.Pz_ = R.Pz;
    R.Pb  = R.Pz_ * std::pow(R.Vz_ / R.Va, n2);

    // 3) Сегменты
    auto reserve_seg = [&](std::vector<double>& V, std::vector<double>& P, int N){
        V.clear(); P.clear(); V.reserve(N); P.reserve(N);
    };
    const int N = std::max(8, samples_per_segment);

    reserve_seg(R.V_comp, R.P_comp, N);
    for (int i=0;i<N;++i){
        double t = (double)i/(N-1);
        double V = R.Va + (R.Vc - R.Va)*t;
        double P = R.Pa * std::pow(R.Va / V, n1);
        R.V_comp.push_back(V); R.P_comp.push_back(P);
    }

    reserve_seg(R.V_iso_add, R.P_iso_add, 2);
    R.V_iso_add.push_back(R.Vc); R.P_iso_add.push_back(R.Pc);
    R.V_iso_add.push_back(R.Vc); R.P_iso_add.push_back(R.Pz);

    reserve_seg(R.V_preexp, R.P_preexp, 2);
    R.V_preexp.push_back(R.Vz);  R.P_preexp.push_back(R.Pz);
    R.V_preexp.push_back(R.Vz_); R.P_preexp.push_back(R.Pz);

    reserve_seg(R.V_exp, R.P_exp, N);
    for (int i=0;i<N;++i){
        double t = (double)i/(N-1);
        double V = R.Vz_ + (R.Va - R.Vz_)*t;
        double P = R.Pz_ * std::pow(R.Vz_ / V, n2);
        R.V_exp.push_back(V); R.P_exp.push_back(P);
    }

    reserve_seg(R.V_drop_b, R.P_drop_b, 2);
    R.V_drop_b.push_back(R.Va); R.P_drop_b.push_back(R.Pb);
    R.V_drop_b.push_back(R.Va); R.P_drop_b.push_back(R.Pr);

    reserve_seg(R.V_exh, R.P_exh, 2);
    R.V_exh.push_back(R.Va); R.P_exh.push_back(R.Pr);
    R.V_exh.push_back(R.Vc); R.P_exh.push_back(R.Pr);

    reserve_seg(R.V_drop_r, R.P_drop_r, 2);
    R.V_drop_r.push_back(R.Vc); R.P_drop_r.push_back(R.Pr);
    R.V_drop_r.push_back(R.Vc); R.P_drop_r.push_back(R.Pa);

    reserve_seg(R.V_int, R.P_int, 2);
    R.V_int.push_back(R.Vc); R.P_int.push_back(R.Pa);
    R.V_int.push_back(R.Va); R.P_int.push_back(R.Pa);

    // 4) Полный контур
    auto append = [&](const std::vector<double>& V, const std::vector<double>& P){
        R.V_path.insert(R.V_path.end(), V.begin(), P.size()?V.end():V.end());
        R.P_path.insert(R.P_path.end(), P.begin(), P.end());
    };
    R.V_path.clear(); R.P_path.clear(); R.V_path.reserve(2*2 + 4*N + 8);

    // по порядку обхода
    R.V_path.insert(R.V_path.end(), R.V_comp.begin(),    R.V_comp.end());
    R.P_path.insert(R.P_path.end(), R.P_comp.begin(),    R.P_comp.end());
    R.V_path.insert(R.V_path.end(), R.V_iso_add.begin(), R.V_iso_add.end());
    R.P_path.insert(R.P_path.end(), R.P_iso_add.begin(), R.P_iso_add.end());
    R.V_path.insert(R.V_path.end(), R.V_preexp.begin(),  R.V_preexp.end());
    R.P_path.insert(R.P_path.end(), R.P_preexp.begin(),  R.P_preexp.end());
    R.V_path.insert(R.V_path.end(), R.V_exp.begin(),     R.V_exp.end());
    R.P_path.insert(R.P_path.end(), R.P_exp.begin(),     R.P_exp.end());
    R.V_path.insert(R.V_path.end(), R.V_drop_b.begin(),  R.V_drop_b.end());
    R.P_path.insert(R.P_path.end(), R.P_drop_b.begin(),  R.P_drop_b.end());
    R.V_path.insert(R.V_path.end(), R.V_exh.begin(),     R.V_exh.end());
    R.P_path.insert(R.P_path.end(), R.P_exh.begin(),     R.P_exh.end());
    R.V_path.insert(R.V_path.end(), R.V_drop_r.begin(),  R.V_drop_r.end());
    R.P_path.insert(R.P_path.end(), R.P_drop_r.begin(),  R.P_drop_r.end());
    R.V_path.insert(R.V_path.end(), R.V_int.begin(),     R.V_int.end());
    R.P_path.insert(R.P_path.end(), R.P_int.begin(),     R.P_int.end());

    // 5) Работа за цикл
    R.A_cycle = 0.0;
    for (size_t i=1;i<R.V_path.size();++i){
        double dV   = R.V_path[i] - R.V_path[i-1];
        double Pavg = 0.5*(R.P_path[i] + R.P_path[i-1]);
        R.A_cycle  += Pavg * dV;
    }

    // 6) Теплоподвод и КПД
    R.Q_in = (R.Pz - R.Pc)*R.Vc + R.Pz*(R.Vz_ - R.Vz);
    R.eta  = (R.Q_in != 0.0 ? R.A_cycle / R.Q_in : 0.0);

    // 7) CSV (если нужно)
    if (!output_csv_path.empty()){
        std::ofstream fcsv(output_csv_path);
        if (fcsv){
            fcsv << "V,P\n";
            for (size_t i=0;i<R.V_path.size();++i){
                fcsv << R.V_path[i] << "," << R.P_path[i] << "\n";
            }
        }
    }

    // 8) HTML-график (если нужно)
    if (!output_html_path.empty()){
        save_indicator_html(output_html_path, R.V_path, R.P_path, R);
#ifdef _WIN32
        if (auto_open_html) {
            ShellExecuteA(nullptr, "open", output_html_path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        }
#endif
    }

    // 9) Summary
    std::ostringstream ss;
    ss << "Indicator PV built.\n"
       << "Va="<<R.Va<<" m^3, Vc="<<R.Vc<<" m^3, Vh="<<R.Vh<<" m^3\n"
       << "Pc="<<R.Pc<<" Pa, Pz="<<R.Pz<<" Pa, Pz'="<<R.Pz_<<" Pa, Pb="<<R.Pb<<" Pa\n"
       << "A_cycle="<<R.A_cycle<<" J per cyl, Q_in="<<R.Q_in<<" J, eta="<<R.eta<<"\n";
    R.summary = ss.str();

    return R;
}
