#ifndef VDS_CRANKPIN_H
#define VDS_CRANKPIN_H

#include <vector>
#include <string>
#include "input_data/input.h"
#include "Calculations/forces_ksm.h"

// Результаты построения ВДС шатунной шейки.
// Координаты диаграммы: X = Z + P′c (Н), Y = T (Н).
struct VDSCrankpinResults {
    std::vector<double> alpha_deg;  // град
    std::vector<double> Z_shifted;  // Н  (Z + P′c)
    std::vector<double> T_same;     // Н  (T)
    double m2_eff = 0.0;            // кг, m₂ (уд.) * Fp
    double Pc_prime = 0.0;          // Н, m2_eff * r * ω²
    std::string summary;
};

// Построение ВДС шатунной шейки.
// out_csv/out_html можно не указывать, чтобы не сохранять файлы.
VDSCrankpinResults build_vds_crankpin(const Params& p,
                                      const ForcesResults& fr,
                                      const std::string& out_csv = {},
                                      const std::string& out_html = {},
                                      bool auto_open_html = true);

#endif // VDS_CRANKPIN_H
