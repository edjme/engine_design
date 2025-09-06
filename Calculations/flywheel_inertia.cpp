// Calculations/flywheel_inertia.cpp
#include "Calculations/flywheel_inertia.h"
#include <cmath>
#include <sstream>
#include <algorithm>
#include <fstream>

#ifdef _WIN32
#include <windows.h>
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// интеграл трапециями по α (в градусах) → работа в Дж
static inline double trap_int_deg_rad(const std::vector<double> &Xdeg,
                                      const std::vector<double> &Y)
{
    if (Xdeg.size() < 2 || Xdeg.size() != Y.size())
        return 0.0;
    double A = 0.0;
    for (size_t k = 1; k < Xdeg.size(); ++k)
    {
        const double a0 = Xdeg[k - 1] * M_PI / 180.0;
        const double a1 = Xdeg[k] * M_PI / 180.0;
        A += 0.5 * (Y[k - 1] + Y[k]) * (a1 - a0);
    }
    return A;
}

static void save_csv(const std::string &path,
                     const std::vector<double> &alpha,
                     const std::vector<double> &M,
                     const std::vector<double> &Mminus,
                     const std::vector<double> &Acum)
{
    if (path.empty())
        return;
    std::ofstream f(path);
    if (!f)
        return;
    f << "alpha_deg,M_Nm,M_minus_Ms_Nm,A_cum_J\n";
    const size_t N = alpha.size();
    for (size_t i = 0; i < N; ++i)
        f << alpha[i] << "," << M[i] << "," << Mminus[i] << "," << Acum[i] << "\n";
}

static void save_html(const std::string &path,
                      const std::vector<double> &alpha,
                      const std::vector<double> &M,
                      double Ms,
                      const std::vector<double> &Acum)
{
    if (path.empty())
        return;
    std::ofstream f(path);
    if (!f)
        return;

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

    f <<
        R"(<!doctype html><html lang="ru"><head><meta charset="utf-8">
<title>Моменты и работа</title>
<style>
body{font-family:system-ui,Segoe UI,Roboto,Arial,sans-serif;background:#0b0d0f;color:#e5e7eb;margin:0;padding:16px}
.card{max-width:1200px;margin:auto;background:#121417;border:1px solid #23262d;border-radius:12px;padding:16px}
h1{margin:0 0 12px 0;font-size:20px}
.grid{display:grid;grid-template-columns:1fr;gap:16px}
.box{background:#0e1114;border:1px solid #23262d;border-radius:10px;padding:10px}
canvas{width:100%;height:360px;background:#0e1114;border-radius:8px}
.meta{font-size:12px;color:#aab}
</style></head><body>
<div class="card">
<h1>Крутящий момент и избыточная работа</h1>
<div class="grid">
  <div class="box"><canvas id="c1"></canvas></div>
  <div class="box"><canvas id="c2"></canvas></div>
</div>
</div>
<script>
const A = )";
    dump(alpha);
    f << ";\nconst M = ";
    dump(M);
    f << ";\nconst Ms = " << Ms << ";\nconst Acum = ";
    dump(Acum);
    f << ";\n";
    f <<
        R"(function plotM(){
 const c=document.getElementById('c1'),ctx=c.getContext('2d');
 const W=c.width,H=c.height,pL=56,pR=16,pT=20,pB=36,w=W-pL-pR,h=H-pT-pB;
 const Amin=A[0],Amax=A[A.length-1];
 let min=1e99,max=-1e99; for (let x of M){ if(x<min)min=x; if(x>max)max=x; }
 min=Math.min(min,Ms); max=Math.max(max,Ms); if(min===max){min-=1;max+=1;}
 const xMap=a=>pL+(a-Amin)/(Amax-Amin)*w;
 const yMap=y=>pT+(1-(y-min)/(max-min))*h;
 ctx.fillStyle='#0e1114';ctx.fillRect(0,0,W,H);
 ctx.strokeStyle='#334155';ctx.strokeRect(pL,pT,w,h);
 ctx.fillStyle='#cbd5e1';ctx.textAlign='center';ctx.font='12px system-ui';
 for(let i=0;i<=6;i++){let a=Amin+(Amax-Amin)*i/6;ctx.fillText(a.toFixed(0),pL+i*w/6,H-14);}
 ctx.save();ctx.translate(14,pT+h/2);ctx.rotate(-Math.PI/2);ctx.fillText('Н·м',0,0);ctx.restore();
 ctx.fillText('Крутящий момент M(α)',pL+w/2,pT-6);
 // M(α)
 ctx.strokeStyle='#60a5fa';ctx.beginPath();
 for(let i=0;i<A.length;i++){let x=xMap(A[i]), y=yMap(M[i]); if(i===0)ctx.moveTo(x,y);else ctx.lineTo(x,y);} ctx.stroke();
 // Ms
 ctx.strokeStyle='#fbbf24';ctx.beginPath();
 ctx.moveTo(xMap(Amin), yMap(Ms)); ctx.lineTo(xMap(Amax), yMap(Ms)); ctx.stroke();
}
function plotA(){
 const c=document.getElementById('c2'),ctx=c.getContext('2d');
 const W=c.width,H=c.height,pL=56,pR=16,pT=20,pB=36,w=W-pL-pR,h=H-pT-pB;
 const Amin=A[0],Amax=A[A.length-1];
 let min=1e99,max=-1e99; for (let x of Acum){ if(x<min)min=x; if(x>max)max=x; }
 if(min===max){min-=1;max+=1;}
 const xMap=a=>pL+(a-Amin)/(Amax-Amin)*w;
 const yMap=y=>pT+(1-(y-min)/(max-min))*h;
 ctx.fillStyle='#0e1114';ctx.fillRect(0,0,W,H);
 ctx.strokeStyle='#334155';ctx.strokeRect(pL,pT,w,h);
 ctx.fillStyle='#cbd5e1';ctx.textAlign='center';ctx.font='12px system-ui';
 for(let i=0;i<=6;i++){let a=Amin+(Amax-Amin)*i/6;ctx.fillText(a.toFixed(0),pL+i*w/6,H-14);}
 ctx.save();ctx.translate(14,pT+h/2);ctx.rotate(-Math.PI/2);ctx.fillText('Дж',0,0);ctx.restore();
 ctx.fillText('Накопленная избыточная работа A(α)',pL+w/2,pT-6);
 ctx.strokeStyle='#34d399';ctx.beginPath();
 for(let i=0;i<A.length;i++){let x=xMap(A[i]), y=yMap(Acum[i]); if(i===0)ctx.moveTo(x,y);else ctx.lineTo(x,y);} ctx.stroke();
}
plotM(); plotA();
</script></body></html>)";
#ifdef _WIN32
    ShellExecuteA(nullptr, "open", path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
#endif
}

FlywheelInertiaResults compute_flywheel_inertia(
    const Params &p,
    const ForcesResults &fr,
    double I_kv_cw,
    int cylinders,
    double delta,
    const std::string &out_csv,
    const std::string &out_html,
    bool auto_open_html)
{
    FlywheelInertiaResults R{};
    R.I_kv_cw = I_kv_cw;
    R.cylinders = std::max(1, cylinders);

    // берём δ из аргумента, иначе из Params, иначе 0.01
    double delta_used = (delta > 0.0) ? delta : (p.delta > 0.0 ? p.delta : 0.01);
    R.delta = delta_used;

    // базовые величины
    const double D = p.diam_cyl;
    const double Rcr = p.r;
    const double lam = p.lyambda; // λ = R/L
    const double w = p.w;

    R.Fp = M_PI * D * D / 4.0;
    R.M2 = p.m_2 * R.Fp;

    // ВНИМАНИЕ: правильная формула (0.5 + 0.25 λ²)
    const double m_eq = p.m_pd * (0.5 + 0.25 * lam * lam);
    R.M_eq = m_eq * R.Fp;

    R.I_mm_one = R.I_kv_cw + (R.M2 + R.M_eq) * Rcr * Rcr;

    // средний момент
    double Ms = 0.0;
    if (!fr.M_cr.empty())
    {
        for (double M : fr.M_cr)
            Ms += M;
        Ms /= (double)fr.M_cr.size();
    }
    R.Ms = Ms;

    // сформируем ряды и накопим работу
    const size_t N = fr.M_cr.size();
    R.alpha_deg = fr.alpha_deg;
    R.M = fr.M_cr;
    R.MminusMs.resize(N);
    R.A_cum.resize(N, 0.0);

    for (size_t i = 0; i < N; i++)
        R.MminusMs[i] = R.M[i] - Ms;

    double A = 0.0, A_min = 0.0, A_max = 0.0;
    for (size_t j = 1; j < N; ++j)
    {
        const double a0 = R.alpha_deg[j - 1] * M_PI / 180.0;
        const double a1 = R.alpha_deg[j] * M_PI / 180.0;
        A += 0.5 * (R.MminusMs[j - 1] + R.MminusMs[j]) * (a1 - a0);
        R.A_cum[j] = A;
        if (A < A_min)
            A_min = A;
        if (A > A_max)
            A_max = A;
    }
    R.dA = (A_max - A_min);

    R.I0_needed = (delta_used > 0.0 && w > 0.0) ? (R.dA / (delta_used * w * w)) : 0.0;
    R.I_fly = R.I0_needed - R.cylinders * R.I_mm_one;

    // Выгрузки
    save_csv(out_csv, R.alpha_deg, R.M, R.MminusMs, R.A_cum);
    save_html(out_html, R.alpha_deg, R.M, R.Ms, R.A_cum);
    (void)auto_open_html; // HTML уже открыт в save_html() под Windows

    // summary
    std::ostringstream ss;
    ss.setf(std::ios::fixed);
    ss.precision(6);
    ss << "Flywheel sizing\n"
       << "  Fp = " << R.Fp << " m^2\n"
       << "  M2 = " << R.M2 << " kg,  M_eq = " << R.M_eq << " kg\n"
       << "  I_kv+CW = " << R.I_kv_cw << " kg·m^2\n"
       << "  I_mm(one cyl) = " << R.I_mm_one << " kg·m^2\n"
       << "  Ms = " << R.Ms << " N·m,  ΔA = " << R.dA << " J\n"
       << "  δ = " << R.delta << ",  ω = " << w << " rad/s\n"
       << "  I0_needed = " << R.I0_needed << " kg·m^2\n"
       << "  cylinders = " << R.cylinders << "\n"
       << "  I_fly (required) = " << R.I_fly << " kg·m^2";
    R.summary = ss.str();

    return R;
}
