#include "common_types.h"

// Конструктор по умолчанию
EngineParams::EngineParams()
    : step_alpha(10.0), end_alpha(360.0), radcrank(0.020),
      lyambda(0.3), n(4800.0), gamma(0.0),
      gammaPric(0.0), dezaxial(0.0) {}