#include "ind_diag_common_types.h"

// Конструктор по умолчанию
IndParams::IndParams()
    : step_alpha(10.0), end_alpha(360.0), radcrank(0.020),
      lyambda(0.3), n(4800.0), epsilent(17), p_a(0.09 * 1000000),
      p_r(0.11 * 1000000), n_1(1.38), lymbda_z(2), ro(1.4), n_2(1.22), diam_cyl(0.12), tau(4) {}