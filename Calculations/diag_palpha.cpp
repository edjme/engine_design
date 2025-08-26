#include "diag_palpha.h"
#include <cmath>
#include <fstream>
#include <sstream>
#include <algorithm>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifdef _WIN32
  #include <windows.h>
#endif

// безопасный clamp для аргумента asin
static inline double clamp11(double x){ return x<-1.0? -1.0 : (x>1.0? 1.0 : x); }
static inline double deg2rad(double d){ return d * M_PI / 180.0; }

// Поршневое перемещение (от ВМТ) и объём над поршнем для КШМ при угле a (рад).
// Формулы как мы использовали ранее:
// beta = asin(λ sin a); S_p = R(1 - cos a) + L(1 - cos beta); V(a) = Vc + F_p * S_p
static inline double piston_S(double R, double L, double lambda, double a){
    // beta через lambda (страховка на случай несоответствия R/L)
    double s = clamp11(lambda * std::sin(a));
    double beta = std::asin(s);
    double Sp = R*(1.0 - std::cos(a)) + L*(1.0 - std::cos(beta));
    return Sp;
}

static inline double volume_V(double Vc, double Fp, double R, double L, double lambda, double a){
    return Vc + Fp * piston_S(R, L, lambda, a);
}

// простая бисекция для решения V(a) = V_target на интервале [aL,aR] (рад)
static double solve_phi_bisect(double Vc, double Fp, double R, double L, double lambda,
                               double V_target,
                               double aL, double aR, double eps = 1e-10, int itmax = 200)
{
    double fL = volume_V(Vc,Fp,R,L,lambda,aL) - V_target;
    double fR = volume_V(Vc,Fp,R,L,lambda,aR) - V_target;
    // если концовка не охватывает корень, попробуем расширить вправо
    if (fL*fR > 0.0){
        // попробуем увеличить верхнюю границу до π (180°)
        aR = M_PI;
        fR = volume_V(Vc,Fp,R,L,lambda,aR) - V_target;
        if (fL*fR > 0.0){
            // корня нет на [0..π] — вернём небольшой угол (пусть φ≈0)
            return 0.0;
        }
    }
    for (int it=0; it<itmax; ++it){
        double mid = 0.5*(aL+aR);
        double fm  = volume_V(Vc,Fp,R,L,lambda,mid) - V_target;
        if (std::fabs(fm) < eps || (aR-aL) < 1e-12) return mid;
        if (fL*fm <= 0.0){ aR = mid; fR = fm; }
        else             { aL = mid; fL = fm; }
    }
    return 0.5*(aL+aR);
}

// --- HTML отрисовка P(α) ---
static void save_palpha_html(const std::string& path,
                             const std::vector<double>& alpha_deg,
                             const std::vector<double>& P_pa,
                             double phi_deg,
                             const IndicatorResults& ind)
{
    // Перевод давления в МПа для графика
    std::vector<double> P_MPa; P_MPa.reserve(P_pa.size());
    for (double x: P_pa) P_MPa.push_back(x * 1e-6);

    // Диапазоны
    auto [aminIt, amaxIt] = std::minmax_element(alpha_deg.begin(), alpha_deg.end());
    auto [pminIt, pmaxIt] = std::minmax_element(P_MPa.begin(), P_MPa.end());
    double Amin=*aminIt, Amax=*amaxIt, Pmin=*pminIt, Pmax=*pmaxIt;
    if (Amax<=Amin) { Amax=Amin+1.0; }
    double Apad = (Amax-Amin)*0.02;
    double Ppad = (Pmax-Pmin)*0.10; if (Ppad==0) Ppad=0.1;
    Amin -= Apad; Amax += Apad; Pmin -= Ppad; Pmax += Ppad;

    std::ofstream f(path);
    if (!f) return;

    f <<
R"(<!doctype html>
<html lang="ru"><head><meta charset="utf-8">
<title>Развёртка индикаторной диаграммы P(α)</title>
<style>
  body{font-family:system-ui,-apple-system,Segoe UI,Roboto,Arial,sans-serif;margin:0;padding:16px;background:#0b0d0f;color:#e8e8e8;}
  .card{max-width:1100px;margin:auto;background:#121417;border:1px solid #23262d;border-radius:12px;padding:16px;box-shadow:0 10px 30px rgba(0,0,0,.35);}
  h1{margin:0 0 12px 0;font-size:20px}
  .row{display:flex;gap:16px;flex-wrap:wrap}
  .box{flex:1 1 260px;background:#0e1114;border:1px solid #23262d;border-radius:8px;padding:8px}
  .box h3{margin:0 0 6px 0;font-size:13px;color:#c7d2fe}
  canvas{width:100%;height:520px;background:#0e1114;border-radius:8px}
  .meta{font-size:12px;color:#aab;}
  code{color:#cbd5e1}
</style>
</head><body>
<div class="card">
  <h1>P(α) — развёртка индикаторной диаграммы</h1>
  <div class="meta">Давление — в МПа, угол α — в градусах (0…720). Вертикальные линии: 0, 180, 360, 540, 720 и α=360+φ.</div>
  <canvas id="palpha" width="1050" height="520"></canvas>
  <div class="row" style="margin-top:12px">
    <div class="box"><h3>Опорные давления</h3>
      <div>P<sub>a</sub> = )" << ind.Pa*1e-6 << R"( МПа</div>
      <div>P<sub>r</sub> = )" << ind.Pr*1e-6 << R"( МПа</div>
      <div>P<sub>c</sub> = )" << ind.Pc*1e-6 << R"( МПа</div>
      <div>P<sub>z</sub> = )" << ind.Pz*1e-6 << R"( МПа</div>
      <div>P<sub>b</sub> = )" << ind.Pb*1e-6 << R"( МПа</div>
    </div>
    <div class="box"><h3>Геометрия</h3>
      <div>V<sub>c</sub> = )" << ind.Vc << R"( м³</div>
      <div>V<sub>a</sub> = )" << ind.Va << R"( м³</div>
      <div>V<sub>z'</sub> = )" << ind.Vz_ << R"( м³</div>
      <div>φ = )" << phi_deg << R"( °</div>
    </div>
  </div>
</div>
<script>
const A = )";

    f << "[";
    for (size_t i=0;i<alpha_deg.size();++i){ if(i) f<<","; f<<alpha_deg[i]; }
    f << "];\nconst P = [";
    for (size_t i=0;i<P_MPa.size();++i){ if(i) f<<","; f<<P_MPa[i]; }
    f << "];\n";
    f << "const Amin="<<Amin<<", Amax="<<Amax<<", Pmin="<<Pmin<<", Pmax="<<Pmax<<";\n";
    f << "const phiDeg="<<phi_deg<<";\n";

    f <<
R"(const canvas=document.getElementById('palpha');
const ctx=canvas.getContext('2d');
const W=canvas.width,H=canvas.height;
const padL=60,padR=20,padT=20,padB=50;
const w=W-padL-padR,h=H-padT-padB;
function xMap(a){ return padL + ( (a-Amin)/(Amax-Amin) )*w; }
function yMap(p){ return padT + (1 - ( (p-Pmin)/(Pmax-Pmin) ))*h; }

// фон и рамка
ctx.fillStyle='#0e1114'; ctx.fillRect(0,0,W,H);
ctx.strokeStyle='#334155'; ctx.lineWidth=1;
ctx.beginPath(); ctx.rect(padL,padT,w,h); ctx.stroke();

// сетка 6×5
ctx.beginPath();
for (let i=1;i<6;i++){ let x=padL+i*w/6; ctx.moveTo(x,padT); ctx.lineTo(x,padT+h); }
for (let j=1;j<5;j++){ let y=padT+j*h/5; ctx.moveTo(padL,y); ctx.lineTo(padL+w,y); }
ctx.strokeStyle='#293241'; ctx.stroke();

// подписи осей
ctx.fillStyle='#aab'; ctx.font='12px system-ui';
ctx.fillText('α (град)', padL+w/2-20, H-24);
ctx.save(); ctx.translate(16,padT+h/2+24); ctx.rotate(-Math.PI/2);
ctx.fillText('P (МПа)', 0,0); ctx.restore();

// деления по α
ctx.fillStyle='#cbd5e1'; ctx.textAlign='center';
for (let i=0;i<=6;i++){ let a=Amin+(Amax-Amin)*i/6; let x=xMap(a); ctx.fillText(a.toFixed(0), x, H-30); }

// деления по P
ctx.textAlign='right';
for (let j=0;j<=5;j++){ let p=Pmin+(Pmax-Pmin)*j/5; let y=yMap(p); ctx.fillText(p.toFixed(2), padL-6, y+4); }

// вертикальные ориентиры: 0,180,360,540,720, 360+φ
const marks=[0,180,360,540,720, 360+phiDeg];
ctx.strokeStyle='#475569'; ctx.setLineDash([4,4]); ctx.beginPath();
for (let m of marks){ if(m < Amin || m > Amax) continue; let x=xMap(m);
  ctx.moveTo(x,padT); ctx.lineTo(x,padT+h);
}
ctx.stroke(); ctx.setLineDash([]);

// кривая P(α)
ctx.strokeStyle='#60a5fa'; ctx.lineWidth=2; ctx.beginPath();
for (let i=0;i<A.length;i++){
  let x=xMap(A[i]), y=yMap(P[i]);
  if(i===0) ctx.moveTo(x,y); else ctx.lineTo(x,y);
}
ctx.stroke();

// надписи к вертикалям (сверху)
ctx.fillStyle='#94a3b8'; ctx.textAlign='center';
function label(x,txt){ ctx.fillText(txt, x, padT+12); }
for (let m of [0,180,360,540,720]){
  if(m < Amin || m > Amax) continue; label(xMap(m), m.toFixed(0));
}
if (360+phiDeg >= Amin && 360+phiDeg <= Amax) label(xMap(360+phiDeg), '360+φ');

</script>
</body></html>
)";
}

// --- основной расчёт ---
PAlphaResults build_P_alpha(const Params& p,
                            const IndicatorResults& ind,
                            double step_deg,
                            const std::string& output_csv_path,
                            const std::string& output_html_path,
                            bool auto_open_html)
{
    PAlphaResults R{};

    // геометрия
    const double Rcr = p.r;                 // радиус кривошипа
    double lambda = p.lyambda;              // геом. характеристика КШМ (R/L)
    double Lrod   = p.leng_rod;            // длина шатуна

    // согласование λ и L на случай, если одно из них «главное»
    if (lambda <= 0.0 && Lrod > 0.0) lambda = Rcr / Lrod;
    if (Lrod   <= 0.0 && lambda > 0.0) Lrod = Rcr / lambda;

    const double Fp  = M_PI * p.diam_cyl * p.diam_cyl / 4.0;

    // из индикаторной диаграммы: опорные давления/объёмы
    const double Vc  = ind.Vc;
    const double Va  = ind.Va;
    const double Vz_ = ind.Vz_; // Vc * rho
    const double Pa  = ind.Pa;
    const double Pr  = ind.Pr;
    const double Pz  = ind.Pz;
    const double n1  = p.n_1;
    const double n2  = p.n_2;

    // найдём φ: V(φ) = Vz_ (радианы, от 0..π)
    double phi = solve_phi_bisect(Vc, Fp, Rcr, Lrod, lambda, Vz_, 0.0, M_PI/2.0);
    R.phi_deg = phi * 180.0 / M_PI;

    // сетка по α
    const double Amax = (p.tau == 4 ? 720.0 : 360.0);
    if (step_deg <= 0.0) step_deg = 1.0;
    int N = (int)std::floor(Amax / step_deg) + 1;

    R.alpha_deg.resize(N);
    R.V_alpha.resize(N);
    R.P_alpha.resize(N);

    for (int i=0;i<N;++i){
        double a_deg = i * step_deg;
        double a     = deg2rad(a_deg);
        double V     = volume_V(Vc, Fp, Rcr, Lrod, lambda, std::fmod(a, 2*M_PI)); // период 2π

        // кусочно-заданное P(α)
        double P = 0.0;
        if (p.tau == 4) {
            if (a_deg < 180.0) {
                // впуск
                P = Pa;
            } else if (a_deg < 360.0) {
                // сжатие
                P = Pa * std::pow(Va / V, n1);
            } else if (std::abs(a_deg - 360.0) < 1e-9 || a_deg <= 360.0 + R.phi_deg) {
                // изобара при Pz (добавление теплоты)
                P = Pz;
            } else if (a_deg < 540.0) {
                // расширение (политропа)
                P = Pz * std::pow(Vz_ / V, n2);
            } else {
                // выпуск
                P = Pr;
            }
        } else {
            // 2-тактный: упрощённая схема — один рабочий цикл на 360°
            double a360 = std::fmod(a_deg, 360.0);
            if (a360 < 180.0) {
                // сжатие до ~180°, затем быстрый рост P к Pz и расширение до 360°
                if (a360 < 160.0)      P = Pa * std::pow(Va / V, n1);
                else if (a360 < 180.0) P = Pz;
                else                   P = Pz * std::pow(Vz_ / V, n2);
            } else {
                // продувка/наполнение — близко к Pa
                P = Pa;
            }
        }

        R.alpha_deg[i] = a_deg;
        R.V_alpha[i]   = V;
        R.P_alpha[i]   = P;
    }

    // CSV
    if (!output_csv_path.empty()){
        std::ofstream f(output_csv_path);
        if (f){
            f << "alpha_deg,P, V\n";
            for (size_t i=0;i<R.alpha_deg.size();++i){
                f << R.alpha_deg[i] << "," << R.P_alpha[i] << "," << R.V_alpha[i] << "\n";
            }
        }
    }

    // HTML-график
    if (!output_html_path.empty()){
        save_palpha_html(output_html_path, R.alpha_deg, R.P_alpha, R.phi_deg, ind);
    #ifdef _WIN32
        if (auto_open_html) {
            ShellExecuteA(nullptr, "open", output_html_path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        }
    #endif
    }

    // summary
    std::ostringstream ss;
    ss << "P(alpha) built. step="<< step_deg <<"°, points="<< N << "\n"
       << "phi="<< R.phi_deg <<"°, V(phi)="<< volume_V(Vc,Fp,Rcr,Lrod,lambda,phi)
       << " m^3, target Vz'="<< Vz_ <<" m^3\n";
    R.summary = ss.str();

    return R;
}