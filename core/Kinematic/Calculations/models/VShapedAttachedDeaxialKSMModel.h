#pragma once
#include "KSMModel.h"

class VShapedAttachedDeaxialKSMModel : public KSMModel {
public:
    explicit VShapedAttachedDeaxialKSMModel(const EngineParams& params);
    
    void calculate(CylinderResults& results, 
                   const std::vector<double>& alpha,
                   double phaseShift) const override;
    
    bool hasSideCylinder() const override { return true; }
    int cylindersPerBlock() const override { return 2; } // Два ряда
};