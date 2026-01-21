//-------------------------------------------------------------------------------------------------------
// ELC4L VST3 - Edit Controller Implementation
//-------------------------------------------------------------------------------------------------------

#include "ELC4Lcontroller.h"
#include "ELC4Leditor.h"
#include "base/source/fstreamer.h"
#include "pluginterfaces/base/ustring.h"

namespace ELC4L {

using namespace Steinberg;
using namespace Steinberg::Vst;

//-------------------------------------------------------------------------------------------------------
tresult PLUGIN_API ELC4LController::initialize(FUnknown* context) {
    tresult result = EditController::initialize(context);
    if (result != kResultOk) return result;
    
    // Band 1-4 Threshold
    parameters.addParameter(STR16("Band 1 Threshold"), STR16("dB"), 0, kDefaultBandThresh,
        ParameterInfo::kCanAutomate, kParamBand1Thresh);
    parameters.addParameter(STR16("Band 2 Threshold"), STR16("dB"), 0, kDefaultBandThresh,
        ParameterInfo::kCanAutomate, kParamBand2Thresh);
    parameters.addParameter(STR16("Band 3 Threshold"), STR16("dB"), 0, kDefaultBandThresh,
        ParameterInfo::kCanAutomate, kParamBand3Thresh);
    parameters.addParameter(STR16("Band 4 Threshold"), STR16("dB"), 0, kDefaultBandThresh,
        ParameterInfo::kCanAutomate, kParamBand4Thresh);
    
    // Band 1-4 Makeup
    parameters.addParameter(STR16("Band 1 Makeup"), STR16("dB"), 0, kDefaultBandMakeup,
        ParameterInfo::kCanAutomate, kParamBand1Makeup);
    parameters.addParameter(STR16("Band 2 Makeup"), STR16("dB"), 0, kDefaultBandMakeup,
        ParameterInfo::kCanAutomate, kParamBand2Makeup);
    parameters.addParameter(STR16("Band 3 Makeup"), STR16("dB"), 0, kDefaultBandMakeup,
        ParameterInfo::kCanAutomate, kParamBand3Makeup);
    parameters.addParameter(STR16("Band 4 Makeup"), STR16("dB"), 0, kDefaultBandMakeup,
        ParameterInfo::kCanAutomate, kParamBand4Makeup);
    
    // Crossover frequencies
    parameters.addParameter(STR16("Crossover 1"), STR16("Hz"), 0, kDefaultXover1,
        ParameterInfo::kCanAutomate, kParamXover1);
    parameters.addParameter(STR16("Crossover 2"), STR16("Hz"), 0, kDefaultXover2,
        ParameterInfo::kCanAutomate, kParamXover2);
    parameters.addParameter(STR16("Crossover 3"), STR16("Hz"), 0, kDefaultXover3,
        ParameterInfo::kCanAutomate, kParamXover3);
    
    // Limiter
    parameters.addParameter(STR16("Limiter Threshold"), STR16("dB"), 0, kDefaultLimiterThresh,
        ParameterInfo::kCanAutomate, kParamLimiterThresh);
    parameters.addParameter(STR16("Limiter Ceiling"), STR16("dB"), 0, kDefaultLimiterCeiling,
        ParameterInfo::kCanAutomate, kParamLimiterCeiling);
    parameters.addParameter(STR16("Limiter Release"), STR16("ms"), 0, kDefaultLimiterRelease,
        ParameterInfo::kCanAutomate, kParamLimiterRelease);
    
    return kResultOk;
}

//-------------------------------------------------------------------------------------------------------
tresult PLUGIN_API ELC4LController::setComponentState(IBStream* state) {
    if (!state) return kResultFalse;
    
    IBStreamer streamer(state, kLittleEndian);
    
    for (int i = 0; i < kNumParams; ++i) {
        float value;
        if (!streamer.readFloat(value)) return kResultFalse;
        setParamNormalized(static_cast<ParamID>(i), value);
    }
    
    return kResultOk;
}

//-------------------------------------------------------------------------------------------------------
IPlugView* PLUGIN_API ELC4LController::createView(FIDString name) {
    if (FIDStringsEqual(name, ViewType::kEditor)) {
        return new ELC4LEditor(this, nullptr, nullptr);
    }
    return nullptr;
}

} // namespace ELC4L
