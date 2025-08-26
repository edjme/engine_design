#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include "input.h"
using namespace std;

static const string CSV_PATH = "input.csv";

// Создаёт файл с дефолтными значениями, если он отсутствует
void createDefaultCSV() {
    ofstream out(CSV_PATH);
    out << "param,value\n";
    out << "diam_cyl,0.11\n";
    out << "stroke,0.12\n";
    out << "epsilent,12\n";
    out << "p_a,90000\n";
    out << "p_r,110000\n";
    out << "n_1,1.38\n";
    out << "n_2,1.22\n";
    out << "lymbda_z,2\n";
    out << "ro,1.35\n";
    out << "lyambda,0.3333333\n";
    out << "n,3900\n";
    out << "m_pd,110\n";
    out << "r,0.06\n";
    out << "leng_rod,0.18\n";
    out << "m_rod,2600\n";
    out << "m_2,1733.33\n";
    out << "w,408.407\n";
    out << "tau,4\n";
    out << "count_cyl,2\n";
    out << "gamma,3.1415926535\n";
    out << "diam_root_neck,0.07997\n";
    out << "diam_rod_neck,0.0715\n";
    out << "length_rod_neck,0.036465\n";
    out << "length_root_neck,0.05278\n";
    out << "depth_web,0.044\n";
    out << "fillet_rad,0.00352\n";
    out << "width_web,0.08701\n";
    out << "dist_axes,0.17725\n";
    out << "dist_web,0.26525\n";
    out << "rho_material,7850\n";
    out.close();
    cout << "Создан файл " << CSV_PATH << " с дефолтными значениями.\n";
}

Params input() {
    ifstream file(CSV_PATH);
    if (!file.is_open()) {
        cerr << "⚠ CSV-файл не найден. Создаю новый с дефолтными значениями...\n";
        createDefaultCSV();
        file.open(CSV_PATH);
    }

    map<string, double> values;
    string line;
    getline(file, line); // пропускаем заголовок

    while (getline(file, line)) {
        string key, valStr;
        stringstream ss(line);
        if (!getline(ss, key, ',')) continue;
        if (!getline(ss, valStr, ',')) continue;
        values[key] = stod(valStr);
    }
    file.close();

    Params p;
    p.diam_cyl         = values["diam_cyl"];
    p.stroke           = values["stroke"];
    p.epsilent         = values["epsilent"];
    p.p_a              = values["p_a"];
    p.p_r              = values["p_r"];
    p.n_1              = values["n_1"];
    p.n_2              = values["n_2"];
    p.lymbda_z         = values["lymbda_z"];
    p.ro               = values["ro"];
    p.lyambda          = values["lyambda"];
    p.n                = values["n"];
    p.m_pd             = values["m_pd"];
    p.r                = values["r"];
    p.leng_rod         = values["leng_rod"];
    p.m_rod            = values["m_rod"];
    p.m_2              = values["m_2"];
    p.w                = values["w"];
    p.tau              = values["tau"];
    p.count_cyl        = values["count_cyl"];
    p.gamma            = values["gamma"];
    p.diam_root_neck   = values["diam_root_neck"];
    p.diam_rod_neck    = values["diam_rod_neck"];
    p.length_rod_neck  = values["length_rod_neck"];
    p.length_root_neck = values["length_root_neck"];
    p.depth_web        = values["depth_web"];
    p.fillet_rad       = values["fillet_rad"];
    p.width_web        = values["width_web"];
    p.dist_axes        = values["dist_axes"];
    p.dist_web         = values["dist_web"];
    p.rho_material     = values["rho_material"];

    return p;
}
