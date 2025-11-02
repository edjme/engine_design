#include "calc_Palpha.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>

using namespace std;

// ===============================================================
// Вспомогательные функции
// ===============================================================
constexpr double PI = 3.14159265358979323846;
inline double deg2rad(double d) { return d * PI / 180.0; }

static bool loadKinematicCSV(const string &path, vector<double> &alpha, vector<double> &stroke)
{
    ifstream f(path);
    if (!f.is_open())
        return false;

    string line;
    bool headerSkipped = false;
    while (getline(f, line))
    {
        if (line.find("alpha") != string::npos)
        {
            headerSkipped = true;
            continue;
        }
        if (!headerSkipped)
            continue;

        stringstream ss(line);
        string a_str, s_str;
        getline(ss, a_str, ';');
        getline(ss, s_str, ';');
        if (!a_str.empty() && !s_str.empty())
        {
            alpha.push_back(stod(a_str));
            stroke.push_back(stod(s_str));
        }
    }
    return !alpha.empty();
}

static bool loadIndicatorCSV(const string &path, vector<double> &pressure)
{
    ifstream f(path);
    if (!f.is_open())
        return false;

    string line;
    bool headerSkipped = false;
    while (getline(f, line))
    {
        if (line.find("P[Па]") != string::npos || line.find("P_Pa") != string::npos)
        {
            headerSkipped = true;
            continue;
        }
        if (!headerSkipped)
            continue;

        stringstream ss(line);
        string p_str;
        getline(ss, p_str, ';');
        if (!p_str.empty())
        {
            pressure.push_back(stod(p_str));
        }
    }
    return !pressure.empty();
}

// ===============================================================
// Генерация HTML
// ===============================================================
static string generateHTML(const vector<double> &alpha, const vector<double> &pressure)
{
    if (alpha.empty() || pressure.empty())
        return "<!-- Нет данных для построения -->";

    auto [a_min_it, a_max_it] = minmax_element(alpha.begin(), alpha.end());
    auto [p_min_it, p_max_it] = minmax_element(pressure.begin(), pressure.end());
    double Amin = *a_min_it;
    double Amax = *a_max_it;
    double Pmin = *p_min_it * 1e-6;
    double Pmax = *p_max_it * 1e-6;

    ostringstream html;
    html << R"(<!DOCTYPE html><html lang="ru"><head><meta charset="utf-8">
<title>Развёртка индикаторной диаграммы P(α)</title>
<style>
body{background:#0d1117;color:#e2e8f0;font-family:Arial;padding:20px;}
canvas{width:100%;height:500px;border:1px solid #334155;border-radius:8px;background:#0f172a;}
h1{font-size:20px;margin-bottom:10px;}
</style></head><body>
<h1>Развёртка индикаторной диаграммы P(α)</h1>
<canvas id='chart' width='1000' height='500'></canvas>
<script>
const alpha=[)";

    for (size_t i = 0; i < alpha.size(); ++i)
    {
        if (i)
            html << ",";
        html << alpha[i];
    }

    html << "];const P=[";
    for (size_t i = 0; i < pressure.size(); ++i)
    {
        if (i)
            html << ",";
        html << pressure[i] * 1e-6;
    }

    html << R"(];
const c=document.getElementById('chart');
const ctx=c.getContext('2d');
const W=c.width,H=c.height;
const padL=60,padR=20,padT=20,padB=40;
const w=W-padL-padR,h=H-padT-padB;
const Amin=)"
         << Amin << ",Amax=" << Amax
         << ",Pmin=" << Pmin << ",Pmax=" << Pmax << R"(;
function x(a){return padL+(a-Amin)/(Amax-Amin)*w;}
function y(p){return padT+(1-(p-Pmin)/(Pmax-Pmin))*h;}
ctx.strokeStyle='#334155';
ctx.strokeRect(padL,padT,w,h);
ctx.beginPath();
for(let i=0;i<alpha.length;i++){
  let X=x(alpha[i]),Y=y(P[i]);
  if(i==0)ctx.moveTo(X,Y);else ctx.lineTo(X,Y);
}
ctx.strokeStyle='#60a5fa';ctx.lineWidth=2;ctx.stroke();
ctx.fillStyle='#94a3b8';ctx.font='12px Arial';
ctx.fillText('α (град)',W/2-20,H-10);
ctx.save();ctx.translate(10,H/2+40);ctx.rotate(-Math.PI/2);
ctx.fillText('P (МПа)',0,0);ctx.restore();
</script></body></html>)";

    return html.str();
}

// ===============================================================
// Основная функция
// ===============================================================
PAlphaResults build_P_alpha(const UnwrapParams &params)
{
    PAlphaResults R;
    vector<double> alpha, stroke, pressures;

    // -----------------------------------------------------------
    // Режим 1: Загрузка из файлов
    // -----------------------------------------------------------
    if (params.useFiles)
    {
        bool ok1 = loadKinematicCSV(params.kinPath, alpha, stroke);
        bool ok2 = loadIndicatorCSV(params.indPath, pressures);

        if (!ok1 || !ok2)
        {
            cerr << "⚠️ Не удалось загрузить один или оба файла. Используются дефолтные данные.\n";
        }
        else
        {
            R.alpha_deg = alpha;
            R.P_alpha = pressures;
            R.htmlContent = generateHTML(alpha, pressures);
            R.summary = "Данные успешно загружены из файлов.";
            return R;
        }
    }

    // -----------------------------------------------------------
    // Режим 2: Ввод вручную или дефолт
    // -----------------------------------------------------------
    double step = params.step_alpha;
    double limit = params.end_alpha;
    size_t N = static_cast<size_t>(limit / step) + 1;

    alpha.resize(N);
    pressures.resize(N);

    for (size_t i = 0; i < N; ++i)
    {
        alpha[i] = i * step;

        // простая модель давления P(α)
        double deg = alpha[i];
        if (deg < 180.0)
            pressures[i] = 1e5; // впуск
        else if (deg < 360.0)
            pressures[i] = 1e5 * pow(8.0, (deg - 180.0) / 180.0); // сжатие
        else if (deg < 540.0)
            pressures[i] = 6e6 * exp(-0.01 * (deg - 360.0)); // расширение
        else
            pressures[i] = 1.2e5; // выпуск
    }

    R.alpha_deg = alpha;
    R.P_alpha = pressures;
    R.htmlContent = generateHTML(alpha, pressures);
    R.summary = "Рассчитано по встроенной модели.";

    return R;
}
