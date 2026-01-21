//-------------------------------------------------------------------------------------------------------
// ELC4L VST3 - Audio Processor Implementation
//-------------------------------------------------------------------------------------------------------

#include "ELC4Lprocessor.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "base/source/fstreamer.h"

namespace ELC4L {

using namespace Steinberg;
using namespace Steinberg::Vst;

//-------------------------------------------------------------------------------------------------------
ELC4LProcessor::ELC4LProcessor()
    : sampleRate(44100.0f)
    , inputDb(-120.0f)
    , outputDb(-120.0f)
    , limiterGrDb(0.0f)
    , limiterBypass(false)
{
    setControllerClass(kControllerUID);
    
    // Initialize parameters to defaults
    parameters[kParamBand1Thresh] = kDefaultBandThresh;
    parameters[kParamBand2Thresh] = kDefaultBandThresh;
    parameters[kParamBand3Thresh] = kDefaultBandThresh;
    parameters[kParamBand4Thresh] = kDefaultBandThresh;
    parameters[kParamBand1Makeup] = kDefaultBandMakeup;
    parameters[kParamBand2Makeup] = kDefaultBandMakeup;
    parameters[kParamBand3Makeup] = kDefaultBandMakeup;
    parameters[kParamBand4Makeup] = kDefaultBandMakeup;
    parameters[kParamXover1] = kDefaultXover1;
    parameters[kParamXover2] = kDefaultXover2;
    parameters[kParamXover3] = kDefaultXover3;
    parameters[kParamLimiterThresh] = kDefaultLimiterThresh;
    parameters[kParamLimiterCeiling] = kDefaultLimiterCeiling;
    parameters[kParamLimiterRelease] = kDefaultLimiterRelease;
    
    for (int i = 0; i < 4; ++i) {
        bandMute[i] = false;
        bandSolo[i] = false;
        bandDelta[i] = false;
        bandBypass[i] = false;
        bandGrDb[i] = 0.0f;
    }
}

//-------------------------------------------------------------------------------------------------------
ELC4LProcessor::~ELC4LProcessor() {}

//-------------------------------------------------------------------------------------------------------
tresult PLUGIN_API ELC4LProcessor::initialize(FUnknown* context) {
    tresult result = AudioEffect::initialize(context);
    if (result != kResultOk) return result;
    
    // Add stereo input/output buses
    addAudioInput(STR16("Stereo In"), SpeakerArr::kStereo);
    addAudioOutput(STR16("Stereo Out"), SpeakerArr::kStereo);
    
    return kResultOk;
}

//-------------------------------------------------------------------------------------------------------
tresult PLUGIN_API ELC4LProcessor::terminate() {
    return AudioEffect::terminate();
}

//-------------------------------------------------------------------------------------------------------
tresult PLUGIN_API ELC4LProcessor::setActive(TBool state) {
    if (state) {
        // Reset DSP on activation
        crossover.reset();
        for (int i = 0; i < 4; ++i) {
            bandComps[i].reset();
        }
        limiter.reset();
        lufsMeter.reset();
    }
    return AudioEffect::setActive(state);
}

//-------------------------------------------------------------------------------------------------------
tresult PLUGIN_API ELC4LProcessor::setupProcessing(ProcessSetup& setup) {
    sampleRate = static_cast<float>(setup.sampleRate);
    
    crossover.setSampleRate(sampleRate);
    for (int i = 0; i < 4; ++i) {
        bandComps[i].setSampleRate(sampleRate);
    }
    limiter.setSampleRate(sampleRate);
    lufsMeter.setSampleRate(sampleRate);
    
    updateParameters();
    
    return AudioEffect::setupProcessing(setup);
}

//-------------------------------------------------------------------------------------------------------
uint32 PLUGIN_API ELC4LProcessor::getLatencySamples() {
    return LookaheadLimiter::kLookaheadSamples;
}

//-------------------------------------------------------------------------------------------------------
tresult PLUGIN_API ELC4LProcessor::setBusArrangements(
    SpeakerArrangement* inputs, int32 numIns,
    SpeakerArrangement* outputs, int32 numOuts) {
    
    if (numIns == 1 && numOuts == 1 && 
        inputs[0] == SpeakerArr::kStereo && 
        outputs[0] == SpeakerArr::kStereo) {
        return AudioEffect::setBusArrangements(inputs, numIns, outputs, numOuts);
    }
    return kResultFalse;
}

//-------------------------------------------------------------------------------------------------------
tresult PLUGIN_API ELC4LProcessor::process(ProcessData& data) {
    // Handle parameter changes
    if (data.inputParameterChanges) {
        int32 numParamsChanged = data.inputParameterChanges->getParameterCount();
        for (int32 i = 0; i < numParamsChanged; ++i) {
            IParamValueQueue* paramQueue = data.inputParameterChanges->getParameterData(i);
            if (paramQueue) {
                ParamValue value;
                int32 sampleOffset;
                int32 numPoints = paramQueue->getPointCount();
                if (numPoints > 0 && paramQueue->getPoint(numPoints - 1, sampleOffset, value) == kResultOk) {
                    Steinberg::Vst::ParamID id = paramQueue->getParameterId();
                    if (id < kNumParams) {
                        parameters[id] = static_cast<float>(value);
                    }
                }
            }
        }
        updateParameters();
    }
    
    // Process audio
    if (data.numInputs == 0 || data.numOutputs == 0) {
        return kResultOk;
    }
    
    int32 numChannels = data.inputs[0].numChannels;
    int32 numSamples = data.numSamples;
    
    if (numChannels < 2) {
        return kResultOk;
    }
    
    float* inL = data.inputs[0].channelBuffers32[0];
    float* inR = data.inputs[0].channelBuffers32[1];
    float* outL = data.outputs[0].channelBuffers32[0];
    float* outR = data.outputs[0].channelBuffers32[1];
    
    // Check for silence flags
    if (data.inputs[0].silenceFlags != 0) {
        data.outputs[0].silenceFlags = data.inputs[0].silenceFlags;
        for (int32 i = 0; i < numSamples; ++i) {
            outL[i] = 0.0f;
            outR[i] = 0.0f;
        }
        return kResultOk;
    }
    
    bool anySolo = bandSolo[0] || bandSolo[1] || bandSolo[2] || bandSolo[3];
    
    for (int32 i = 0; i < numSamples; ++i) {
        float b1L, b1R, b2L, b2R, b3L, b3R, b4L, b4R;
        
        // Split into 4 bands
        crossover.processSample(inL[i], inR[i], b1L, b1R, b2L, b2R, b3L, b3R, b4L, b4R);
        
        // Store uncompressed band signals for Delta
        float bandInputL[4] = { b1L, b2L, b3L, b4L };
        float bandInputR[4] = { b1R, b2R, b3R, b4R };
        
        // Process each band through compressor (with bypass check)
        if (!bandBypass[0]) bandComps[0].process(b1L, b1R);
        if (!bandBypass[1]) bandComps[1].process(b2L, b2R);
        if (!bandBypass[2]) bandComps[2].process(b3L, b3R);
        if (!bandBypass[3]) bandComps[3].process(b4L, b4R);
        
        float bandOutL[4] = { b1L, b2L, b3L, b4L };
        float bandOutR[4] = { b1R, b2R, b3R, b4R };
        
        // Get makeup gains for Delta
        float makeupGains[4];
        for (int b = 0; b < 4; ++b) {
            float makeupDb = normalizedToCompMakeupDb(parameters[kParamBand1Makeup + b]);
            makeupGains[b] = dbToLinear(makeupDb);
        }
        
        // Mix bands with Delta/Mute/Solo logic
        float mixL = 0.0f;
        float mixR = 0.0f;
        
        for (int b = 0; b < 4; ++b) {
            float outBandL, outBandR;
            
            if (bandDelta[b]) {
                float invMakeup = (makeupGains[b] > 1e-6f) ? (1.0f / makeupGains[b]) : 1.0f;
                outBandL = bandInputL[b] - (bandOutL[b] * invMakeup);
                outBandR = bandInputR[b] - (bandOutR[b] * invMakeup);
            } else {
                outBandL = bandOutL[b];
                outBandR = bandOutR[b];
            }
            
            bool playBand = false;
            if (anySolo) {
                playBand = bandSolo[b];
            } else {
                playBand = !bandMute[b];
            }
            
            if (playBand) {
                mixL += outBandL;
                mixR += outBandR;
            }
        }
        
        // Limiter
        if (!limiterBypass) {
            limiter.process(mixL, mixR);
        }
        
        outL[i] = mixL;
        outR[i] = mixR;
        
        // Update LUFS meter
        lufsMeter.process(mixL, mixR);
    }
    
    // Update GR meters
    for (int b = 0; b < 4; ++b) {
        bandGrDb[b] = bandComps[b].getGainReductionDb();
    }
    limiterGrDb = limiter.getGainReductionDb();
    
    return kResultOk;
}

//-------------------------------------------------------------------------------------------------------
tresult PLUGIN_API ELC4LProcessor::setState(IBStream* state) {
    IBStreamer streamer(state, kLittleEndian);
    
    for (int i = 0; i < kNumParams; ++i) {
        float value;
        if (!streamer.readFloat(value)) return kResultFalse;
        parameters[i] = value;
    }
    
    updateParameters();
    return kResultOk;
}

//-------------------------------------------------------------------------------------------------------
tresult PLUGIN_API ELC4LProcessor::getState(IBStream* state) {
    IBStreamer streamer(state, kLittleEndian);
    
    for (int i = 0; i < kNumParams; ++i) {
        streamer.writeFloat(parameters[i]);
    }
    
    return kResultOk;
}

//-------------------------------------------------------------------------------------------------------
void ELC4LProcessor::updateParameters() {
    updateFrequencies();
    updateCompressors();
    updateLimiter();
}

//-------------------------------------------------------------------------------------------------------
void ELC4LProcessor::updateFrequencies() {
    crossover.setXover1(normalizedToFrequency(parameters[kParamXover1]));
    crossover.setXover2(normalizedToFrequency(parameters[kParamXover2]));
    crossover.setXover3(normalizedToFrequency(parameters[kParamXover3]));
}

//-------------------------------------------------------------------------------------------------------
void ELC4LProcessor::updateCompressors() {
    for (int i = 0; i < 4; ++i) {
        float threshDb = normalizedToCompThreshDb(parameters[kParamBand1Thresh + i]);
        float makeupDb = normalizedToCompMakeupDb(parameters[kParamBand1Makeup + i]);
        bandComps[i].setThresholdDb(threshDb);
        bandComps[i].setMakeupDb(makeupDb);
    }
}

//-------------------------------------------------------------------------------------------------------
void ELC4LProcessor::updateLimiter() {
    float threshDb = normalizedToLimiterDb(parameters[kParamLimiterThresh]);
    float ceilingDb = normalizedToLimiterDb(parameters[kParamLimiterCeiling]);
    float releaseMs = normalizedToLimiterReleaseMs(parameters[kParamLimiterRelease]);
    
    limiter.setThreshold(threshDb);
    limiter.setCeiling(ceilingDb);
    limiter.setRelease(releaseMs);
}

} // namespace ELC4L
