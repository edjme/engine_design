#pragma once
#include "KSMModel.h"

class DeaxialKSMModel : public KSMModel {
public:
    explicit DeaxialKSMModel(const EngineParams& params);
    
    void calculate(CylinderResults& results, 
                   const std::vector<double>& alpha,
                   double phaseShift) const override;
    
    bool hasSideCylinder() const override { return false; }
    int cylindersPerBlock() const override { return 1; }
};