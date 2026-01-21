//-------------------------------------------------------------------------------------------------------
// ELC4L VST3 - Plugin IDs and FUID definitions
//-------------------------------------------------------------------------------------------------------
#pragma once

#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/vst/vsttypes.h"

namespace ELC4L {

// Unique plugin identifiers
// ELC4L Processor UID
static const Steinberg::FUID kProcessorUID(0x454C4334, 0x4C505250, 0x4345535A, 0x30303031);
// ELC4L Controller UID  
static const Steinberg::FUID kControllerUID(0x454C4334, 0x4C435452, 0x4C303030, 0x31303031);

//-------------------------------------------------------------------------------------------------------
// Parameter IDs (must match VST2 version for compatibility)
//-------------------------------------------------------------------------------------------------------
enum ParamID : Steinberg::Vst::ParamID {
    kParamBand1Thresh = 0,
    kParamBand2Thresh,
    kParamBand3Thresh,
    kParamBand4Thresh,
    kParamBand1Makeup,
    kParamBand2Makeup,
    kParamBand3Makeup,
    kParamBand4Makeup,
    kParamXover1,
    kParamXover2,
    kParamXover3,
    kParamLimiterThresh,
    kParamLimiterCeiling,
    kParamLimiterRelease,
    kNumParams
};

// Default parameter values (normalized 0-1)
constexpr float kDefaultBandThresh = 0.75f;
constexpr float kDefaultBandMakeup = 0.5f;
constexpr float kDefaultXover1 = 0.20f;
constexpr float kDefaultXover2 = 0.50f;
constexpr float kDefaultXover3 = 0.80f;
constexpr float kDefaultLimiterThresh = 0.75f;
constexpr float kDefaultLimiterCeiling = 0.9583f;
constexpr float kDefaultLimiterRelease = 0.3f;

// Frequency range (Hz)
constexpr float kMinFreq = 20.0f;
constexpr float kMaxFreq = 20000.0f;

} // namespace ELC4L
