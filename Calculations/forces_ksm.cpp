#include "Calculations/forces_ksm.h"
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

static inline double clamp11(double x){ return x<-1.0? -1.0 : (x>1.0? 1.0 : x); }
static inline double deg2rad(double d){ return d * M_PI / 180.0; }

// точная кинематика: S(a), β(a), и d²S/da² для ускорения
static inline void kinematics_SB(double R, double L, double lambda, double a,
                                 double& beta, double& d2S_da2)
{
    // beta
    double s = clamp11(lambda * std::sin(a));
    beta = std::asin(s);

    // производные beta по α
    double cB = std::cos(beta), sB = std::sin(beta);
    double cA = std::cos(a),    sA = std::sin(a);

    // dβ/dα и d²β/dα²
    double dB = lambda * cA / cB;
    double d2B = (-lambda * sA / cB) - (lambda*lambda) * (cA*cA) * sB / (cB*cB*cB);

    // S(a) = R(1 - cos a) + L(1 - cos β)
    // d²S/da² = R cos a + L ( sinβ * d²β/da² + cosβ * (dβ/da)² )
    d2S_da2 = R * cA + L * ( sB * d2B + cB * dB * dB );
}

// HTML-страница с 4 графиками
static void save_forces_html(const std::string& path,
                             const std::vector<double>& A,
                             const std::vector<double>& Pgas_MPa,
                             const std::vector<double>& Fin,
                             const std::vector<double>& Fsum,
                             const std::vector<double>& N,
                             const std::vector<double>& K,
                             const std::vector<double>& Z,
                             const std::vector<double>& T,
                             const std::vector<double>& M,
                             const ForcesResults& R,
                             const Params& p)
{
    auto minmax = [](const std::vector<double>& v){
        auto it = std::minmax_element(v.begin(), v.end());
        return std::pair<double,double>( (v.empty()?0:*it.first), (v.empty()?0:*it.second) );
    };
    auto [Pmin, Pmax] = minmax(Pgas_MPa);
    auto [Fmin, Fmax] = minmax(Fsum);
    auto [Nmin, Nmax] = minmax(N);
    auto [Zmin, Zmax] = minmax(Z);
    auto [Tmin, Tmax] = minmax(T);
    auto [Mmin, Mmax] = minmax(M);

    std::ofstream f(path);
    if (!f) return;

    f <<
R"(<!doctype html>
<html lang="ru"><head><meta charset="utf-8">
<title>Силы в КШМ</title>
<style>
body{font-family:system-ui,-apple-system,Segoe UI,Roboto,Arial,sans-serif;margin:0;padding:16px;background:#0b0d0f;color:#e8e8e8;}
.card{max-width:1200px;margin:auto;background:#121417;border:1px solid #23262d;border-radius:12px;padding:16px;box-shadow:0 10px 30px rgba(0,0,0,.35);}
h1{margin:0 0 12px 0;font-size:20px}
.grid{display:grid;grid-template-columns:1fr 1fr;gap:16px}
.box{background:#0e1114;border:1px solid #23262d;border-radius:8px;padding:10px}
canvas{width:100%;height:360px;background:#0e1114;border-radius:8px}
.meta{font-size:12px;color:#aab}
code{color:#cbd5e1}
</style>
</head><body>
<div class="card">
<h1>Силы, действующие в КШМ</h1>
<div class="meta">На основе P(α) из развёртки; α от 0 до 360/720° в зависимости от тактности.</div>

<div class="grid">
  <div class="box"><canvas id="c1"></canvas></div>
  <div class="box"><canvas id="c2"></canvas></div>
  <div class="box"><canvas id="c3"></canvas></div>
  <div class="box"><canvas id="c4"></canvas></div>
</div>

<div class="box" style="margin-top:16px">
  <h3>Параметры</h3>
  <div class="meta">
    D = )" << p.diam_cyl << R"( м,  S = )" << p.stroke << R"( м,  F_p = )" << R.F_p << R"( м²,
    r = )" << p.r << R"( м,  L = )" << p.leng_rod << R"( м,  λ = )" << p.lyambda << R"(<br>
    ω = )" << p.w << R"( рад/с,  m<sub>rec</sub> = )" << R.m_rec << R"( кг,  τ = )" << p.tau << R"(
  </div>
</div>

</div>
<script>
const A = )";

    auto dump = [&](const std::vector<double>& v){
        f << "[";
        for (size_t i=0;i<v.size();++i){ if (i) f<<","; f<<v[i]; }
        f << "]";
    };

    dump(A); f << ";\nconst Pgas="; dump(Pgas_MPa); f << ";\n";
    f << "const Fin="; dump(Fin); f << ";\n";
    f << "const Fsum="; dump(Fsum); f << ";\n";
    f << "const Ndata="; dump(N); f << ";\n";
    f << "const Kdata="; dump(K); f << ";\n";
    f << "const Zdata="; dump(Z); f << ";\n";
    f << "const Tdata="; dump(T); f << ";\n";
    f << "const Mdata="; dump(M); f << ";\n";

    f <<
R"(function plot(canvasId, series, title, yLabel){
  const c=document.getElementById(canvasId), ctx=c.getContext('2d');
  const W=c.width, H=c.height, padL=56, padR=20, padT=20, padB=40;
  const w=W-padL-padR, h=H-padT-padB;
  function minmax(v){ let mi=1e99, ma=-1e99;
    for (let s of series) for (let x of s.y){ if(x<mi) mi=x; if(x>ma) ma=x; }
    if (mi==ma){ mi-=1; ma+=1; } return [mi,ma];
  }
  const [Ymin,Ymax] = minmax(series);
  const Amin=A[0], Amax=A[A.length-1];
  function xMap(a){ return padL + (a-Amin)/(Amax-Amin)*w; }
  function yMap(y){ return padT + (1-(y-Ymin)/(Ymax-Ymin))*h; }

  ctx.fillStyle='#0e1114'; ctx.fillRect(0,0,W,H);
  ctx.strokeStyle='#334155'; ctx.strokeRect(padL,padT,w,h);

  // сетка
  ctx.strokeStyle='#293241'; ctx.beginPath();
  for(let i=1;i<6;i++){ let x=padL+i*w/6; ctx.moveTo(x,padT); ctx.lineTo(x,padT+h); }
  for(let i=1;i<5;i++){ let y=padT+i*h/5; ctx.moveTo(padL,y); ctx.lineTo(padL+w,y); }
  ctx.stroke();

  // подписи
  ctx.fillStyle='#cbd5e1'; ctx.font='12px system-ui'; ctx.textAlign='center';
  for(let i=0;i<=6;i++){ let a=Amin+(Amax-Amin)*i/6; ctx.fillText(a.toFixed(0), padL+i*w/6, H-14); }
  ctx.save(); ctx.translate(14, padT+h/2); ctx.rotate(-Math.PI/2);
  ctx.fillText(yLabel, 0,0); ctx.restore();
  ctx.fillText(title, padL+w/2, padT-6);

  const colors=['#60a5fa','#34d399','#f87171','#fbbf24','#a78bfa'];
  series.forEach((s,idx)=>{
    ctx.strokeStyle=colors[idx%colors.length]; ctx.beginPath();
    for(let i=0;i<A.length;i++){
      let x=xMap(A[i]), y=yMap(s.y[i]);
      if(i===0) ctx.moveTo(x,y); else ctx.lineTo(x,y);
    }
    ctx.stroke();
  });
}

plot('c1', [{y:Pgas}], 'P_gas(α), МПа', 'МПа');
plot('c2', [{y:Fin},{y:Fsum}], 'F_in(α), F_sum(α), Н', 'Н');
plot('c3', [{y:Ndata},{y:Kdata}], 'N(α), K(α), Н', 'Н');
plot('c4', [{y:Zdata},{y:Tdata},{y:Mdata}], 'Z(α), T(α), M(α)', 'Н / Н·м');
</script>
</body></html>
)";
}

ForcesResults build_forces_ksm(const Params& p,
                               const PAlphaResults& palpha,
                               const std::string& out_csv,
                               const std::string& out_html,
                               bool auto_open_html)
{
    ForcesResults R{};
    const double D   = p.diam_cyl;
    const double S   = p.stroke;
    const double Fp  = M_PI * D*D / 4.0;
    const double Rcr = p.r;
    double Lrod      = p.leng_rod;
    double lambda    = p.lyambda;          // λ = R/L (геом. характеристика)
    if (lambda <= 0.0 && Lrod > 0.0) lambda = Rcr / Lrod;
    if (Lrod   <= 0.0 && lambda > 0.0) Lrod = Rcr / lambda;

    const double omega = p.w;

    R.F_p   = Fp;
    R.m_rec = p.m_pd * Fp;                 // кг (m_pd в кг/м²)

    const size_t N = palpha.alpha_deg.size();
    R.alpha_deg = palpha.alpha_deg;
    R.a_p.resize(N);
    R.beta.resize(N);
    R.P_gas.resize(N);
    R.F_in.resize(N);
    R.F_sum.resize(N);
    R.K.resize(N);
    R.N.resize(N);
    R.Z.resize(N);
    R.T.resize(N);
    R.M_cr.resize(N);

    auto upd_absmax = [](double& m, double v){ m = std::max(m, std::fabs(v)); };

    for (size_t i=0;i<N;++i){
        double a_deg = palpha.alpha_deg[i];
        double a     = deg2rad(std::fmod(a_deg, 360.0));   // рад
        double P     = palpha.P_alpha[i];                  // Па

        // кинематика
        double beta, d2S;
        kinematics_SB(Rcr, Lrod, lambda, a, beta, d2S);
        double ap = omega*omega * d2S;                     // м/с²

        // силы
        double Fgas = P * Fp;                              // Н
        double Fin  = - R.m_rec * ap;                      // Н (знак: против ускорения)
        double Fsum = Fgas + Fin;                          // по оси цилиндра

        // разложение
        double cB = std::cos(beta);
        double tB = std::tan(beta);
        // защита от cosβ≈0
        if (std::fabs(cB) < 1e-8) cB = (cB>=0? 1e-8 : -1e-8), tB = std::tan(std::acos(cB));
        double K = Fsum / cB;
        double N = Fsum * tB;

        double Z = K * std::cos(a + beta);
        double T = K * std::sin(a + beta);
        double M = T * Rcr;

        // запись
        R.a_p[i]   = ap;
        R.beta[i]  = beta;
        R.P_gas[i] = Fgas;
        R.F_in[i]  = Fin;
        R.F_sum[i] = Fsum;
        R.K[i]     = K;
        R.N[i]     = N;
        R.Z[i]     = Z;
        R.T[i]     = T;
        R.M_cr[i]  = M;

        // экстремумы
        upd_absmax(R.F1_abs_max, Fin);
        upd_absmax(R.Fsum_abs_max, Fsum);
        upd_absmax(R.N_abs_max, N);
        upd_absmax(R.K_abs_max, K);
        upd_absmax(R.Z_abs_max, Z);
        upd_absmax(R.T_abs_max, T);
        upd_absmax(R.M_abs_max, M);
    }

    // CSV
    if (!out_csv.empty()){
        std::ofstream f(out_csv);
        if (f){
            f << "alpha_deg,P_gas_N,F_in_N,F_sum_N,beta_rad,K_N,N_N,Z_N,T_N,M_Nm,a_p_mps2\n";
            for (size_t i=0;i<N;++i){
                f << R.alpha_deg[i] << "," << R.P_gas[i] << "," << R.F_in[i] << "," << R.F_sum[i] << ","
                  << R.beta[i] << "," << R.K[i] << "," << R.N[i] << "," << R.Z[i] << "," << R.T[i] << ","
                  << R.M_cr[i] << "," << R.a_p[i] << "\n";
            }
        }
    }

    // HTML (давление в МПа для удобства)
    if (!out_html.empty()){
        std::vector<double> Pgas_MPa; Pgas_MPa.reserve(N);
        for (double Fg : R.P_gas) Pgas_MPa.push_back(Fg / 1e6); // Н -> МПа*м² условно, но визуально сопоставимо
        save_forces_html(out_html, R.alpha_deg, Pgas_MPa, R.F_in, R.F_sum, R.N, R.K, R.Z, R.T, R.M_cr, R, p);
    #ifdef _WIN32
        if (auto_open_html){
            ShellExecuteA(nullptr, "open", out_html.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        }
    #endif
    }

    std::ostringstream ss;
    ss << "Forces computed on " << N << " samples.\n"
       << "F_sum|max = " << R.Fsum_abs_max << " N;  "
       << "N|max = "     << R.N_abs_max    << " N;  "
       << "T|max = "     << R.T_abs_max    << " N;  "
       << "M|max = "     << R.M_abs_max    << " N·m\n";
    R.summary = ss.str();

    return R;
}