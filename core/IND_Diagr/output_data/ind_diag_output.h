#ifndef IND_DIAG_OUTPUT_H
#define IND_DIAG_OUTPUT_H

#include <string>
#include "core/common/ind_diag_common_types.h"

class IndDiagramOutput
{
public:
    static std::string generateFilename(const std::string &prefix = "indicator_results");

    static bool saveToCSV(const IndicatorResults &results,
                          const IndParams &params,
                          const std::string &filename = "");

    static bool saveSummary(const IndicatorResults &results,
                            const IndParams &params,
                            const std::string &filename = "");

    static void exportWithMenu(const IndicatorResults &results,
                               const IndParams &params);

private:
    static void writeHeader(std::ofstream &file, const IndParams &params);
};

#endif
