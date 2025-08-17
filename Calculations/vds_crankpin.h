#ifndef VDS_CRANKPIN_H
#define VDS_CRANKPIN_H

#include <vector>
#include <string>
#include "input_data/input.h"          // Params
#include "Calculations/forces_ksm.h"   // ForcesResults: alpha_deg, T, Z

struct VDSCrankpinResults {
    std::vector<double> alpha_deg;  // сетка угла, град
    std::vector<double> Z_shifted;  // Z + P'c, Н
    std::vector<double> T_same;     // T, Н
    double Pc_prime = 0.0;          // Н (смещение по Z)
    double m2_eff   = 0.0;          // кг (m2 * Fp)
    std::string summary;
};

VDSCrankpinResults build_vds_crankpin(const Params& p,
                                      const ForcesResults& fr,
                                      const std::string& out_csv  = {},
                                      const std::string& out_html = {},
                                      bool auto_open_html = true);

#endif
