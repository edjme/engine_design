#ifndef IND_DIAGR_H
#define IND_DIAGR_H

#include <vector>
#include <string>
#include "core/IND_Diagr/input_data/ind_diag_input.h"
#include "core/common/ind_diag_common_types.h"

// Построение индикаторной диаграммы P–V.
// samples_per_segment: типовое число точек на «гладком» участке (N>=8).
// Если указаны пути — сохраняет CSV ("V,P") и HTML; на Windows auto_open_html может открыть HTML.
IndicatorResults build_indicator_PV(const IndParams &p,
                                    int samples_per_segment = 400,
                                    const std::string &output_csv_path = "",
                                    const std::string &output_html_path = "",
                                    bool auto_open_html = false);

#endif // IND_DIAGR_H