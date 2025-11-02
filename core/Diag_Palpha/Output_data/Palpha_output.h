#ifndef Palpha_OUTPUT_H
#define Palpha_OUTPUT_H

#include <string>
#include "core/Diag_Palpha/Calculations/calc_Palpha.h"
#include "core/Diag_Palpha/Input_data/Palpha_input.h"

class IndUnwrapOutput
{
public:
    static void exportWithMenu(const PAlphaResults &results, const UnwrapParams &params);
};

#endif
