//-------------------------------------------------------------------------------------------------------
// ELC4L VST3 - Audio Processor
//-------------------------------------------------------------------------------------------------------
#pragma once

#include "public.sdk/source/vst/vstaudioeffect.h"
#include "ELC4Lids.h"
#include "ELC4Ldsp.h"

namespace ELC4L {

class ELC4LProcessor : public Steinberg::Vst::AudioEffect {
public:
    ELC4LProcessor();
    ~ELC4LProcessor() override;
    
    static Steinberg::FUnknown* createInstance(void*) {
        return static_cast<Steinberg::Vst::IAudioProcessor*>(new ELC4LProcessor());
    }
    
    // AudioEffect overrides
    Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown* context) override;
    Steinberg::tresult PLUGIN_API terminate() override;
    Steinberg::tresult PLUGIN_API setActive(Steinberg::TBool state) override;
    Steinberg::tresult PLUGIN_API setupProcessing(Steinberg::Vst::ProcessSetup& setup) override;
    Steinberg::tresult PLUGIN_API process(Steinberg::Vst::ProcessData& data) override;
    Steinberg::tresult PLUGIN_API setState(Steinberg::IBStream* state) override;
    Steinberg::tresult PLUGIN_API getState(Steinberg::IBStream* state) override;
    Steinberg::tresult PLUGIN_API setBusArrangements(
        Steinberg::Vst::SpeakerArrangement* inputs, Steinberg::int32 numIns,
        Steinberg::Vst::SpeakerArrangement* outputs, Steinberg::int32 numOuts) override;
    Steinberg::uint32 PLUGIN_API getLatencySamples() override;

private:
    void updateParameters();
    void updateCompressors();
    void updateFrequencies();
    void updateLimiter();
    
    // Parameters (normalized 0-1)
    float parameters[kNumParams];
    
    // DSP components
    FourBandCrossover crossover;
    OptoCompressor bandComps[4];
    LookaheadLimiter limiter;
    LufsMeter lufsMeter;
    
    // Bypass/monitoring state
    bool bandMute[4];
    bool bandSolo[4];
    bool bandDelta[4];
    bool bandBypass[4];
    bool limiterBypass;
    
    // Metering
    float inputDb;
    float outputDb;
    float bandGrDb[4];
    float limiterGrDb;
    
    float sampleRate;
};

} // namespace ELC4L
