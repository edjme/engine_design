#include "Calculations/vds_crankpin.h"
#include <cmath>
#include <fstream>
#include <sstream>
#include <algorithm>

#ifdef _WIN32
  #include <windows.h>
#endif

namespace {
constexpr double PI = 3.1415926535897932384626433832795;

inline void pad_range(double& lo,double& hi){
    double d = hi - lo;
    if (d <= 0.0) d = (std::fabs(hi) + std::fabs(lo) + 1.0);
    lo -= 0.05 * d;
    hi += 0.05 * d;
}

// --- HTML: одна XY-диаграмма (ВДС шейки) ---
void save_html_crankpin(const std::string& path,
                        const std::vector<double>& Zs,
                        const std::vector<double>& Ts,
                        const std::vector<double>& Adeg,
                        double Pc_prime,
                        const Params& p,
                        double Fp)
{
    auto minmax = [](const std::vector<double>& v){
        if (v.empty()) return std::pair<double,double>(0.0, 1.0);
        auto it = std::minmax_element(v.begin(), v.end());
        return std::pair<double,double>(*it.first, *it.second);
    };
    auto [Zmin,Zmax] = minmax(Zs);
    auto [Tmin,Tmax] = minmax(Ts);

    double Xlo = std::min(Zmin, 0.0), Xhi = std::max(Zmax, 0.0);
    double Ylo = std::min(Tmin, 0.0), Yhi = std::max(Tmax, 0.0);
    pad_range(Xlo, Xhi);
    pad_range(Ylo, Yhi);

    std::ofstream f(path);
    if(!f) return;

    auto dump = [&](const std::vector<double>& v){
        f << "[";
        for(size_t i=0;i<v.size();++i){ if(i) f<<","; f<<v[i]; }
        f << "]";
    };

    const double m2_eff = p.m_2 * Fp;
    const double omega  = (p.w != 0.0 ? p.w : (2.0 * PI * p.n / 60.0));

    f <<
R"(<!doctype html><html lang="ru"><head><meta charset="utf-8">
<title>ВДС шатунной шейки</title>
<style>
body{font-family:system-ui,-apple-system,Segoe UI,Roboto,Arial,sans-serif;margin:0;padding:16px;background:#0b0d0f;color:#e8e8e8;}
.card{max-width:1100px;margin:auto;background:#121417;border:1px solid #23262d;border-radius:12px;padding:16px;box-shadow:0 10px 30px rgba(0,0,0,.35);}
.box{background:#0e1114;border:1px solid #23262d;border-radius:8px;padding:10px}
canvas{width:100%;height:540px;background:#0e1114;border-radius:8px}
.badge{display:inline-block;background:#1f2937;color:#cbd5e1;border:1px solid #334155;padding:4px 8px;border-radius:999px;margin-right:8px}
.small{font-size:12px;color:#aab}
</style></head><body>
<div class="card">
  <h2>Векторная диаграмма сил шатунной шейки</h2>
  <div class="small" style="margin:10px 0">
    <span class="badge">r=)" << p.r << R"( м</span>
    <span class="badge">ω=)" << omega << R"( рад/с</span>
    <span class="badge">D=)" << p.diam_cyl << R"( м</span>
    <span class="badge">Fₚ=πD²/4=)" << Fp << R"( м²</span>
    <span class="badge">m₂(уд.)=)" << p.m_2 << R"( кг/м² → m₂=Fₚ·m₂=)" << m2_eff << R"( кг</span>
    <span class="badge">P′c=)" << Pc_prime << R"( Н</span>
    <span class="badge">Построение по (X=Z+P′c, Y=T)</span>
  </div>
  <div class="box"><canvas id="c"></canvas></div>
</div>
<script>
)";

    auto outArr = [&](const char* name, const std::vector<double>& v){
        f<<"const "<<name<<"=";
        dump(v); f<<";\n";
    };
    outArr("Zs", Zs);
    outArr("Ts", Ts);
    outArr("A", Adeg);

    f << "const R={xmin:"<<Xlo<<",xmax:"<<Xhi<<",ymin:"<<Ylo<<",ymax:"<<Yhi<<"};\n";

    f <<
R"(const c=document.getElementById('c'), ctx=c.getContext('2d');
const W=c.width,H=c.height, L=60,Rm=20,T=20,B=50, w=W-L-Rm,h=H-T-B;
function xMap(x){ return L + (x-R.xmin)/(R.xmax-R.xmin)*w; }
function yMap(y){ return T + (1-(y-R.ymin)/(R.ymax-R.ymin))*h; }

ctx.fillStyle='#0e1114'; ctx.fillRect(0,0,W,H);
ctx.strokeStyle='#334155'; ctx.strokeRect(L,T,w,h);

// сетка
ctx.strokeStyle='#293241'; ctx.beginPath();
for(let i=1;i<6;i++){ let x=L+i*w/6; ctx.moveTo(x,T); ctx.lineTo(x,T+h); }
for(let j=1;j<5;j++){ let y=T+j*h/5; ctx.moveTo(L,y); ctx.lineTo(L+w,y); }
ctx.stroke();

// оси X=0, Y=0
ctx.setLineDash([4,4]); ctx.strokeStyle='#475569'; ctx.beginPath();
if(0>=R.xmin && 0<=R.xmax){ let x=xMap(0); ctx.moveTo(x,T); ctx.lineTo(x,T+h); }
if(0>=R.ymin && 0<=R.ymax){ let y=yMap(0); ctx.moveTo(L,y); ctx.lineTo(L+w,y); }
ctx.stroke(); ctx.setLineDash([]);

// подписи
ctx.fillStyle='#cbd5e1'; ctx.font='12px system-ui'; ctx.textAlign='center';
ctx.fillText('X = Z + P′c (Н)', L+w/2, H-20);
ctx.save(); ctx.translate(18, T+h/2); ctx.rotate(-Math.PI/2);
ctx.fillText('Y = T (Н)', 0,0); ctx.restore();

// кривая и маркеры (каждые 10°)
ctx.strokeStyle='#60a5fa'; ctx.lineWidth=2; ctx.beginPath();
for(let i=0;i<Zs.length;i++){ let x=xMap(Zs[i]), y=yMap(Ts[i]); if(i===0) ctx.moveTo(x,y); else ctx.lineTo(x,y); }
ctx.stroke();
ctx.fillStyle='#93c5fd';
for(let i=0;i<A.length;i++){ if(Math.round(A[i])%10===0){ let x=xMap(Zs[i]), y=yMap(Ts[i]); ctx.beginPath(); ctx.arc(x,y,2,0,6.283); ctx.fill(); } }
</script></body></html>
)";
}
} // namespace

// --- основной расчёт ---
VDSCrankpinResults build_vds_crankpin(const Params& p,
                                      const ForcesResults& fr,
                                      const std::string& out_csv,
                                      const std::string& out_html,
                                      bool auto_open_html)
{
    VDSCrankpinResults R{};

    // Площадь поршня и m2
    const double Fp = PI * p.diam_cyl * p.diam_cyl / 4.0; // м²
    R.m2_eff   = p.m_2 * Fp;                              // кг

    // Угловая скорость (если p.w=0 — берём из n)
    const double omega = (p.w != 0.0 ? p.w : (2.0 * PI * p.n / 60.0));
    R.Pc_prime = R.m2_eff * p.r * omega * omega;          // Н

    const size_t N = fr.alpha_deg.size();
    R.alpha_deg = fr.alpha_deg;
    R.Z_shifted.resize(N);
    R.T_same.resize(N);

    for (size_t i=0;i<N;++i){
        R.Z_shifted[i] = fr.Z[i] + R.Pc_prime; // смещение по X: Z → Z+P′c
        R.T_same[i]    = fr.T[i];              // по Y без изменений
    }

    // CSV
    if (!out_csv.empty()){
        std::ofstream f(out_csv);
        if (f){
            f << "alpha_deg,Z_plus_Pc_N,T_N,Pc_prime_N\n";
            for (size_t i=0;i<N;++i){
                f << R.alpha_deg[i] << "," << R.Z_shifted[i] << "," << R.T_same[i]
                  << "," << R.Pc_prime << "\n";
            }
        }
    }

    // HTML
    if (!out_html.empty()){
        save_html_crankpin(out_html, R.Z_shifted, R.T_same, R.alpha_deg, R.Pc_prime, p, Fp);
    #ifdef _WIN32
        if (auto_open_html) {
            ShellExecuteA(nullptr, "open", out_html.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        }
    #endif
    }

    std::ostringstream ss;
    ss << "VDS crankpin built: points="<<N
       << ", m2_eff="<<R.m2_eff<<" kg, Pc'="<<R.Pc_prime<<" N. "
       << "Построение в осях (Z+P′c, T).";
    R.summary = ss.str();

    return R;
}
