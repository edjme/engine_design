#ifndef FORCES_KSM_H
#define FORCES_KSM_H

#include <vector>
#include <string>
#include "input_data/input.h"
#include "Calculations/diag_palpha.h"   // для PAlphaResults

struct ForcesResults {
    // сетка
    std::vector<double> alpha_deg;  // угол, град
    // кинематика поршня
    std::vector<double> a_p;        // ускорение поршня, м/с^2
    std::vector<double> beta;       // угол шатуна, рад

    // силы на оси цилиндра
    std::vector<double> P_gas;      // сила газов = P(α)*F_p, Н
    std::vector<double> F_in;       // сила инерции поступ. масс, Н (со знаком)
    std::vector<double> F_sum;      // суммарная по оси цилиндра, Н

    // разложение
    std::vector<double> K;          // сила вдоль шатуна, Н
    std::vector<double> N;          // боковая сила на цилиндр, Н
    std::vector<double> Z;          // радиальная сила на кривошип, Н
    std::vector<double> T;          // тангенциальная сила на кривошип, Н
    std::vector<double> M_cr;       // крутящий момент на валу = T*R, Н·м

    // характерные величины (по модулю)
    double F1_abs_max = 0.0;        // |F_in|max (1-го порядка нет отдельно — у нас точная кинематика)
    double Fsum_abs_max = 0.0;
    double N_abs_max = 0.0, K_abs_max = 0.0;
    double Z_abs_max = 0.0, T_abs_max = 0.0, M_abs_max = 0.0;

    // мета
    double F_p = 0.0;               // площадь поршня, м^2
    double m_rec = 0.0;             // масса поступ. частей, кг
    std::string summary;
};

// Главная функция расчёта и отрисовки.
// out_csv/out_html можно оставить пустыми, чтобы не сохранять соответствующий файл.
ForcesResults build_forces_ksm(const Params& p,
                               const PAlphaResults& palpha,
                               const std::string& out_csv,
                               const std::string& out_html,
                               bool auto_open_html = true);

#endif // FORCES_KSM_H
