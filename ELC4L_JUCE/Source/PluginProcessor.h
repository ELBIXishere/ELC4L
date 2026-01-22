//-------------------------------------------------------------------------------------------------------
// ELC4L - Elite 4-Band Compressor + Limiter
// Plugin Processor Header
//-------------------------------------------------------------------------------------------------------

#pragma once

#include <JuceHeader.h>
#include "DSP/DSPModules.h"

class ELC4LAudioProcessor : public juce::AudioProcessor,
                            public juce::AudioProcessorValueTreeState::Listener
{
public:
    ELC4LAudioProcessor();
    ~ELC4LAudioProcessor() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    //==============================================================================
    // 파라미터 변경 리스너
    void parameterChanged(const juce::String& parameterID, float newValue) override;

    // 파라미터 트리
    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }

    //==============================================================================
    // 미터링 데이터 (스레드 안전)
    float getInputDb() const { return inputDb.load(); }
    float getOutputDb() const { return outputDb.load(); }
    float getBandGrDb(int band) const { return (band >= 0 && band < 4) ? bandGrDb[band].load() : 0.0f; }
    float getLimiterGrDb() const { return limiterGrDb.load(); }
    float getLufsMomentary() const { return lufsMomentary.load(); }
    
    // 스펙트럼 데이터
    const float* getSpectrumIn() const { return spectrumIn; }
    const float* getSpectrumOut() const { return spectrumOut; }
    static constexpr int getSpectrumSize() { return ELC4L::kDisplayBins; }
    
    // 크로스오버 주파수
    float getXover1Hz() const { return crossover.xover1; }
    float getXover2Hz() const { return crossover.xover2; }
    float getXover3Hz() const { return crossover.xover3; }

    //==============================================================================
    // 밴드 모니터링 상태
    bool getBandMute(int band) const { return (band >= 0 && band < 4) ? bandMute[band] : false; }
    bool getBandSolo(int band) const { return (band >= 0 && band < 4) ? bandSolo[band] : false; }
    bool getBandDelta(int band) const { return (band >= 0 && band < 4) ? bandDelta[band] : false; }
    bool getBandBypass(int band) const { return (band >= 0 && band < 4) ? bandBypass[band] : false; }
    bool getLimiterBypass() const { return limiterBypass; }

    void setBandMute(int band, bool state) { if (band >= 0 && band < 4) bandMute[band] = state; }
    void setBandSolo(int band, bool state) { if (band >= 0 && band < 4) bandSolo[band] = state; }
    void setBandDelta(int band, bool state) { if (band >= 0 && band < 4) bandDelta[band] = state; }
    void setBandBypass(int band, bool state) { if (band >= 0 && band < 4) bandBypass[band] = state; }
    void setLimiterBypass(bool state) { limiterBypass = state; }

    void toggleBandMute(int band) { if (band >= 0 && band < 4) bandMute[band] = !bandMute[band]; }
    void toggleBandSolo(int band) { if (band >= 0 && band < 4) bandSolo[band] = !bandSolo[band]; }
    void toggleBandDelta(int band) { if (band >= 0 && band < 4) bandDelta[band] = !bandDelta[band]; }
    void toggleBandBypass(int band) { if (band >= 0 && band < 4) bandBypass[band] = !bandBypass[band]; }
    void toggleLimiterBypass() { limiterBypass = !limiterBypass; }

private:
    //==============================================================================
    juce::AudioProcessorValueTreeState apvts;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    void updateCompressors();
    void updateFrequencies();
    void updateLimiter();

    //==============================================================================
    // DSP 모듈
    ELC4L::CrossoverDSP crossover;
    ELC4L::OptoCompressor bandComps[4];
    ELC4L::LookaheadLimiter limiter;
    ELC4L::LufsMeter lufsMeter;

    // 미터링 데이터 (atomic)
    std::atomic<float> inputDb{-120.0f};
    std::atomic<float> outputDb{-120.0f};
    std::atomic<float> bandGrDb[4];
    std::atomic<float> limiterGrDb{0.0f};
    std::atomic<float> lufsMomentary{-120.0f};

    // 스펙트럼 분석기
    juce::dsp::FFT fft{12};  // 4096 포인트
    juce::dsp::WindowingFunction<float> window{ELC4L::kFftSize, 
        juce::dsp::WindowingFunction<float>::blackmanHarris};
    float fftBufferIn[ELC4L::kFftSize] = {};
    float fftBufferOut[ELC4L::kFftSize] = {};
    float spectrumIn[ELC4L::kDisplayBins] = {};
    float spectrumOut[ELC4L::kDisplayBins] = {};
    int fftWritePos = 0;

    void computeSpectrum(const float* input, float* output);

    // 밴드 모니터링 상태
    bool bandMute[4] = { false, false, false, false };
    bool bandSolo[4] = { false, false, false, false };
    bool bandDelta[4] = { false, false, false, false };
    bool bandBypass[4] = { false, false, false, false };
    bool limiterBypass = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ELC4LAudioProcessor)
};
