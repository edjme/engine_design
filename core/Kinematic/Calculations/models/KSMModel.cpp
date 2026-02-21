#include "KSMModel.h"
#include "AxialKSMModel.h"
#include "DeaxialKSMModel.h"
#include "VShapedKSMModel.h"
#include "VShapedDeaxialKSMModel.h"
#include "VShapedAttachedDeaxialKSMModel.h"
#include "VShapedAttachedKSMModel.h"
std::unique_ptr<KSMModel> KSMModel::create(KSMType type, 
                                           const EngineParams& params) {
    switch (type) {
        case KSMType::Axial:
            return std::make_unique<AxialKSMModel>(params);
            
        case KSMType::Deaxial:
            return std::make_unique<DeaxialKSMModel>(params);
            
        case KSMType::VShaped:
            return std::make_unique<VShapedKSMModel>(params);
            
        case KSMType::VShapedDeaxial:
            return std::make_unique<VShapedDeaxialKSMModel>(params);
            
        case KSMType::VShapedAttached:
            return std::make_unique<VShapedAttachedKSMModel>(params);
            
        case KSMType::VShapedAttachedDeaxial:
            return std::make_unique<VShapedAttachedDeaxialKSMModel>(params);
            
        default:
            throw std::runtime_error("Unknown KSM type");
    }
}