//-------------------------------------------------------------------------------------------------------
// ELC4L - Elite 4-Band Compressor + Limiter
// DSP Modules (Ported from original VST2 implementation)
// JUCE Compatible Version
//-------------------------------------------------------------------------------------------------------

#pragma once

#include <JuceHeader.h>
#include <cmath>
#include <algorithm>

namespace ELC4L {

//=======================================================================
// 파라미터 인덱스
//=======================================================================
enum Parameters {
    kParamBand1Thresh = 0,  // 밴드 1 스레숄드 (dB)
    kParamBand2Thresh,
    kParamBand3Thresh,
    kParamBand4Thresh,
    kParamBand1Makeup,      // 밴드 1 메이크업 (dB)
    kParamBand2Makeup,
    kParamBand3Makeup,
    kParamBand4Makeup,
    kParamXover1,           // 크로스오버 1 (Hz)
    kParamXover2,
    kParamXover3,
    kParamLimiterThresh,    // 리미터 스레숄드 (dB)
    kParamLimiterCeiling,   // 리미터 실링 (dB)
    kParamLimiterRelease,   // 리미터 릴리즈 (ms)
    kParamSidechainFreq,    // 사이드체인 HPF 주파수
    kParamSidechainActive,  // 사이드체인 ON/OFF
    kParamBand1Mode,        // 0=Stereo, 1=M/S
    kParamBand2Mode,
    kParamBand3Mode,
    kParamBand4Mode,
    kNumParams
};

// 기본 파라미터 값 (normalized 0-1)
constexpr float kDefaultBandThresh = 0.75f;    // ~-9 dB
constexpr float kDefaultBandMakeup = 0.5f;     // 0 dB
constexpr float kDefaultXover1 = 0.20f;        // ~80 Hz
constexpr float kDefaultXover2 = 0.50f;        // ~630 Hz
constexpr float kDefaultXover3 = 0.80f;        // ~5 kHz
constexpr float kDefaultLimiterThresh = 0.75f; // -6 dB
constexpr float kDefaultLimiterCeiling = 0.9583f; // -1 dB
constexpr float kDefaultLimiterRelease = 0.3f; // ~120ms

// 주파수 범위
constexpr float kMinFreq = 20.0f;
constexpr float kMaxFreq = 20000.0f;

// 스펙트럼 분석기 설정
constexpr int kFftSize = 4096;
constexpr int kSpectrumBins = 512;
constexpr int kDisplayBins = 128;
constexpr int kFftHopSize = 1024;

//=======================================================================
// 테이프 새츄레이터 (Algebraic Sigmoid)
//=======================================================================
struct TapeSaturator {
    // x / sqrt(1 + x^2) 기반 소프트 클리퍼 + 바이어스
    inline float process(float input, float drive, float bias) {
        float x = input * drive + bias;
        float saturated = x / std::sqrt(1.0f + x * x);
        
        // 바이어스로 인한 DC 오프셋 제거
        float biasCurve = bias / std::sqrt(1.0f + bias * bias);
        return (saturated - biasCurve);
    }
};

//=======================================================================
// 폴리페이즈 FIR 오버샘플러 (4x)
// 32-tap Linear Phase FIR / 4 Phases
//=======================================================================
class PolyphaseOversampler {
public:
    PolyphaseOversampler() { reset(); }

    void reset() {
        for (int i = 0; i < kTapLength; ++i) state[i] = 0.0f;
        ptr = 0;
    }

    // 업샘플 1 -> 4
    void processUpsample(float input, float* outBuffer4x) {
        state[ptr] = input;
        for (int phase = 0; phase < 4; ++phase) {
            float sum = 0.0f;
            const float* coeffs = &kCoeffs[phase * kTapsPerPhase];
            for (int i = 0; i < kTapsPerPhase; ++i) {
                int idx = (ptr - i + kTapLength) % kTapLength;
                sum += state[idx] * coeffs[i];
            }
            outBuffer4x[phase] = sum;
        }
        ptr = (ptr + 1) % kTapLength;
    }

    // 다운샘플 4 -> 1
    float processDownsample(const float* inBuffer4x) {
        return inBuffer4x[0] * 0.1f + inBuffer4x[1] * 0.4f + 
               inBuffer4x[2] * 0.4f + inBuffer4x[3] * 0.1f;
    }

    static constexpr int kTapsPerPhase = 8;
    static constexpr int kTapLength = 32;

private:
    float state[kTapLength];
    int ptr = 0;
    
    // Kaiser 윈도우 싱크 계수
    static constexpr float kCoeffs[32] = {
        // Phase 0
        -0.002f, 0.005f, -0.012f, 0.025f, 0.965f, 0.025f, -0.012f, 0.005f,
        // Phase 1
        -0.004f, 0.010f, -0.025f, 0.060f, 0.880f, 0.090f, -0.025f, 0.010f,
        // Phase 2
        -0.005f, 0.015f, -0.040f, 0.120f, 0.750f, 0.180f, -0.040f, 0.015f,
        // Phase 3
        -0.004f, 0.012f, -0.045f, 0.220f, 0.550f, 0.280f, -0.045f, 0.012f
    };
};

//=======================================================================
// Lookahead 리미터 (VST2와 완전히 동일한 알고리즘)
// - 64 샘플 lookahead (~1.5ms @ 44.1kHz)
// - ARC 스타일 듀얼 릴리즈
// - 심플하고 투명한 리미팅
//=======================================================================
struct LookaheadLimiter {
    static constexpr int kLookaheadSamples = 64;  // ~1.5ms at 44.1kHz
    
    // 딜레이 버퍼
    float delayL[kLookaheadSamples];
    float delayR[kLookaheadSamples];
    int delayIndex = 0;
    
    // 엔벨로프 팔로워
    float envelope = 0.0f;
    float attackCoeff = 0.0f;
    float releaseCoeff = 0.0f;
    float fastReleaseCoeff = 0.0f;
    float slowReleaseCoeff = 0.0f;
    
    // 설정
    float threshold = 0.5f;
    float ceiling = 0.891f;  // -1 dB
    float makeupGain = 1.0f;
    float sampleRate = 44100.0f;
    float lastGain = 1.0f;
    float gainReductionDb = 0.0f;
    float releaseMs = 100.0f;
    
    LookaheadLimiter() {
        reset();
        updateCoefficients();
    }
    
    void reset() {
        for (int i = 0; i < kLookaheadSamples; ++i) {
            delayL[i] = 0.0f;
            delayR[i] = 0.0f;
        }
        delayIndex = 0;
        envelope = 0.0f;
        lastGain = 1.0f;
        gainReductionDb = 0.0f;
    }
    
    void setSampleRate(float sr) {
        sampleRate = sr;
        updateCoefficients();
    }
    
    void setThreshold(float threshDb) {
        threshold = std::pow(10.0f, threshDb / 20.0f);
        updateMakeupGain();
    }
    
    void setCeiling(float ceilingDb) {
        ceiling = std::pow(10.0f, ceilingDb / 20.0f);
        updateMakeupGain();
    }
    
    void setRelease(float relMs) {
        releaseMs = relMs;
        if (releaseMs < 10.0f) releaseMs = 10.0f;
        if (releaseMs > 500.0f) releaseMs = 500.0f;
        updateCoefficients();
    }
    
    void updateCoefficients() {
        // VST2와 완전히 동일한 계수
        float attackMs = 0.1f;
        float fastReleaseMs = releaseMs * 0.4f;
        float slowReleaseMs = releaseMs * 4.0f;
        
        attackCoeff = std::exp(-1.0f / (sampleRate * attackMs / 1000.0f));
        releaseCoeff = std::exp(-1.0f / (sampleRate * releaseMs / 1000.0f));
        fastReleaseCoeff = std::exp(-1.0f / (sampleRate * fastReleaseMs / 1000.0f));
        slowReleaseCoeff = std::exp(-1.0f / (sampleRate * slowReleaseMs / 1000.0f));
    }
    
    void updateMakeupGain() {
        makeupGain = ceiling / threshold;
        if (makeupGain > 4.0f) makeupGain = 4.0f;
    }
    
    // VST2와 완전히 동일한 process 함수
    void process(float& left, float& right) {
        // 딜레이된 샘플 가져오기
        float delayedL = delayL[delayIndex];
        float delayedR = delayR[delayIndex];
        
        // 현재 샘플 딜레이 버퍼에 저장
        delayL[delayIndex] = left;
        delayR[delayIndex] = right;
        delayIndex = (delayIndex + 1) % kLookaheadSamples;
        
        // 피크 감지
        float peakL = std::abs(left);
        float peakR = std::abs(right);
        float peak = (peakL > peakR) ? peakL : peakR;
        
        // 듀얼 엔벨로프 (VST2 ARC 스타일)
        float fastEnv = envelope;
        float slowEnv = envelope;
        
        if (peak > fastEnv) {
            fastEnv = attackCoeff * fastEnv + (1.0f - attackCoeff) * peak;
        } else {
            fastEnv = fastReleaseCoeff * fastEnv + (1.0f - fastReleaseCoeff) * peak;
        }
        
        if (peak > slowEnv) {
            slowEnv = attackCoeff * slowEnv + (1.0f - attackCoeff) * peak;
        } else {
            slowEnv = slowReleaseCoeff * slowEnv + (1.0f - slowReleaseCoeff) * peak;
        }
        
        envelope = (fastEnv > slowEnv) ? fastEnv : slowEnv;
        
        // 게인 계산
        float gain = 1.0f;
        if (envelope > threshold) {
            gain = threshold / envelope;
        }
        
        // 출력 적용
        float outL = delayedL * gain * makeupGain;
        float outR = delayedR * gain * makeupGain;
        
        // 하드 클리핑
        if (outL > ceiling) outL = ceiling;
        if (outL < -ceiling) outL = -ceiling;
        if (outR > ceiling) outR = ceiling;
        if (outR < -ceiling) outR = -ceiling;
        
        left = outL;
        right = outR;

        lastGain = gain;
        gainReductionDb = (gain > 1.0e-9f) ? (-20.0f * std::log10(gain)) : 60.0f;
    }

    float getGainReductionDb() const { return gainReductionDb; }
};

//=======================================================================
// LA-2A 스타일 옵토 컴프레서 (밴드별)
//=======================================================================
struct OptoCompressor {
    float sampleRate = 44100.0f;
    float envelope = 0.0f;
    float fastEnvelope = 0.0f;
    float slowEnvelope = 0.0f;
    float attackCoeff = 0.0f;
    float fastReleaseCoeff = 0.0f;
    float slowReleaseCoeff = 0.0f;
    float threshold = 1.0f;
    float makeupGain = 1.0f;
    float lastGain = 1.0f;
    float gainReductionDb = 0.0f;
    float peakHold = 0.0f;
    float peakDecay = 0.0f;
    
    // 튜브 새츄레이션 파라미터
    float saturationDrive = 0.3f;
    bool saturationEnabled = true;
    float currentGain = 1.0f;

    // 오버샘플링 + 새츄레이터
    TapeSaturator saturator;
    PolyphaseOversampler oversamplerL;
    PolyphaseOversampler oversamplerR;
    float upBufferL[4];
    float upBufferR[4];

    // 사이드체인 HPF 상태
    bool sidechainEnabled = false;
    float scFilterCoeff = 0.0f;
    float scFilterState = 0.0f;

    // LA-2A 상수
    static constexpr float kMinRatio = 3.0f;
    static constexpr float kMaxRatio = 100.0f;
    static constexpr float kKneeDb = 10.0f;
    static constexpr float kAttackMs = 10.0f;
    static constexpr float kFastReleaseMs = 60.0f;
    static constexpr float kSlowReleaseBase = 500.0f;
    static constexpr float kSlowReleaseMax = 5000.0f;

    OptoCompressor() { updateCoefficients(); }

    void reset() {
        envelope = 0.0f;
        fastEnvelope = 0.0f;
        slowEnvelope = 0.0f;
        lastGain = 1.0f;
        gainReductionDb = 0.0f;
        peakHold = 0.0f;
        oversamplerL.reset();
        oversamplerR.reset();
        scFilterState = 0.0f;
    }

    void setSampleRate(float sr) {
        sampleRate = sr;
        updateCoefficients();
    }

    void setThresholdDb(float db) {
        threshold = std::pow(10.0f, db / 20.0f);
    }

    void setMakeupDb(float db) {
        makeupGain = std::pow(10.0f, db / 20.0f);
    }
    
    void setSaturationDrive(float drive) {
        saturationDrive = juce::jlimit(0.0f, 1.0f, drive);
    }
    
    void setSaturationEnabled(bool enabled) {
        saturationEnabled = enabled;
    }

    void setSidechainEnabled(bool enabled) { sidechainEnabled = enabled; }
    
    void setSidechainFreq(float fc) {
        if (fc <= 0.0f || sampleRate <= 0.0f) { scFilterCoeff = 0.0f; return; }
        scFilterCoeff = std::exp(-2.0f * juce::MathConstants<float>::pi * fc / sampleRate);
    }

    void updateCoefficients() {
        attackCoeff = std::exp(-1.0f / (sampleRate * kAttackMs / 1000.0f));
        fastReleaseCoeff = std::exp(-1.0f / (sampleRate * kFastReleaseMs / 1000.0f));
        slowReleaseCoeff = std::exp(-1.0f / (sampleRate * kSlowReleaseBase / 1000.0f));
        peakDecay = std::exp(-1.0f / (sampleRate * 0.5f));
    }

    void process(float& left, float& right) {
        // 사이드체인 HPF
        float monoIn = 0.5f * (left + right);
        float detectorSignal = monoIn;

        if (sidechainEnabled) {
            scFilterState = scFilterState * scFilterCoeff + monoIn * (1.0f - scFilterCoeff);
            detectorSignal = monoIn - scFilterState;
        }

        // RMS 감지
        float level = detectorSignal * detectorSignal;
        float detector = std::sqrt(level + 1.0e-12f);

        // 피크 트래킹
        if (detector > peakHold) {
            peakHold = detector;
        } else {
            peakHold = peakDecay * peakHold + (1.0f - peakDecay) * detector;
        }

        // 듀얼 엔벨로프 (LA-2A 스타일)
        if (detector > fastEnvelope) {
            fastEnvelope = attackCoeff * fastEnvelope + (1.0f - attackCoeff) * detector;
        } else {
            fastEnvelope = fastReleaseCoeff * fastEnvelope + (1.0f - fastReleaseCoeff) * detector;
        }

        // 프로그램 의존 슬로우 릴리즈
        float overDb = 20.0f * std::log10((peakHold / threshold) + 1.0e-12f);
        if (overDb < 0.0f) overDb = 0.0f;
        float releaseScale = juce::jlimit(1.0f, 10.0f, 1.0f + (overDb * 0.15f));
        float slowRelMs = std::min(kSlowReleaseBase * releaseScale, kSlowReleaseMax);
        float dynamicSlowCoeff = std::exp(-1.0f / (sampleRate * slowRelMs / 1000.0f));

        if (detector > slowEnvelope) {
            slowEnvelope = attackCoeff * slowEnvelope + (1.0f - attackCoeff) * detector;
        } else {
            slowEnvelope = dynamicSlowCoeff * slowEnvelope + (1.0f - dynamicSlowCoeff) * detector;
        }

        // 복합 엔벨로프
        envelope = 0.3f * fastEnvelope + 0.7f * slowEnvelope;

        // 게인 계산 (가변 레이시오)
        float levelDb = 20.0f * std::log10(envelope + 1.0e-12f);
        float threshDb = 20.0f * std::log10(threshold + 1.0e-12f);
        float overThresh = levelDb - threshDb;

        float dynamicRatio = kMinRatio;
        if (overThresh > 0.0f) {
            float ratioBlend = juce::jlimit(0.0f, 1.0f, overThresh / 20.0f);
            dynamicRatio = kMinRatio + (kMaxRatio - kMinRatio) * (ratioBlend * ratioBlend);
        }

        // 소프트 니
        float grDb = 0.0f;
        if (overThresh <= -kKneeDb * 0.5f) {
            grDb = 0.0f;
        } else if (overThresh >= kKneeDb * 0.5f) {
            grDb = overThresh - (overThresh / dynamicRatio);
        } else {
            float x = overThresh + kKneeDb * 0.5f;
            grDb = (x * x) * (1.0f - 1.0f / dynamicRatio) / (2.0f * kKneeDb);
        }

        float gain = std::pow(10.0f, -grDb / 20.0f);
        float gainSmooth = 0.995f;
        gain = gainSmooth * lastGain + (1.0f - gainSmooth) * gain;

        currentGain = gain;

        left *= gain * makeupGain;
        right *= gain * makeupGain;

        // 4x 오버샘플링 + 테이프 새츄레이션
        if (saturationEnabled) {
            float drive = 1.0f + saturationDrive * 3.0f; 
            float bias = saturationDrive * 0.1f;

            // 좌채널
            oversamplerL.processUpsample(left, upBufferL);
            for (int i = 0; i < 4; ++i) 
                upBufferL[i] = saturator.process(upBufferL[i], drive, bias);
            left = oversamplerL.processDownsample(upBufferL) / drive;

            // 우채널
            oversamplerR.processUpsample(right, upBufferR);
            for (int i = 0; i < 4; ++i) 
                upBufferR[i] = saturator.process(upBufferR[i], drive, bias);
            right = oversamplerR.processDownsample(upBufferR) / drive;
        }

        lastGain = gain;
        gainReductionDb = grDb;
    }

    float getGainReductionDb() const { return gainReductionDb; }
    float getCurrentGain() const { return currentGain; }
};

//=======================================================================
// LUFS 미터 (Momentary, 근사치)
//=======================================================================
struct LufsMeter {
    float sampleRate = 44100.0f;
    float momentaryEnergy = 0.0f;
    float momentaryCoeff = 0.0f;
    float momentaryLufs = -120.0f;

    LufsMeter() { updateCoefficients(); }

    void reset() {
        momentaryEnergy = 0.0f;
        momentaryLufs = -120.0f;
    }

    void setSampleRate(float sr) {
        sampleRate = sr;
        updateCoefficients();
    }

    void updateCoefficients() {
        float tau = 0.4f;  // 400ms 모멘터리 윈도우
        momentaryCoeff = std::exp(-1.0f / (sampleRate * tau));
    }

    void process(float left, float right) {
        float energy = 0.5f * (left * left + right * right);
        momentaryEnergy = momentaryCoeff * momentaryEnergy + (1.0f - momentaryCoeff) * energy;
        float safeEnergy = std::max(momentaryEnergy, 1.0e-12f);
        momentaryLufs = -0.691f + 10.0f * std::log10(safeEnergy);
    }

    float getMomentary() const { return momentaryLufs; }
};

//=======================================================================
// Linkwitz-Riley 4차 크로스오버 (4밴드)
//=======================================================================
struct CrossoverDSP {
    struct BiquadCoeffs {
        double b0, b1, b2;
        double a1, a2;
    };
    
    struct BiquadState {
        double x1[2] = {}, x2[2] = {};
        double y1[2] = {}, y2[2] = {};
        
        void reset() {
            x1[0] = x1[1] = x2[0] = x2[1] = 0.0;
            y1[0] = y1[1] = y2[0] = y2[1] = 0.0;
        }
    };
    
    // 3개 크로스오버 포인트에 대한 필터 쌍
    BiquadCoeffs lowpass1a, lowpass1b;
    BiquadCoeffs highpass1a, highpass1b;
    BiquadState lp1StateA, lp1StateB;
    BiquadState hp1StateA, hp1StateB;

    BiquadCoeffs lowpass2a, lowpass2b;
    BiquadCoeffs highpass2a, highpass2b;
    BiquadState lp2StateA, lp2StateB;
    BiquadState hp2StateA, hp2StateB;

    BiquadCoeffs lowpass3a, lowpass3b;
    BiquadCoeffs highpass3a, highpass3b;
    BiquadState lp3StateA, lp3StateB;
    BiquadState hp3StateA, hp3StateB;

    float sampleRate = 44100.0f;
    float xover1 = 120.0f;
    float xover2 = 800.0f;
    float xover3 = 4000.0f;
    
    CrossoverDSP() { updateCoefficients(); }
    
    void setSampleRate(float sr) {
        sampleRate = sr;
        updateCoefficients();
    }
    
    void setXover1(float freq) { xover1 = freq; updateCoefficients(); }
    void setXover2(float freq) { xover2 = freq; updateCoefficients(); }
    void setXover3(float freq) { xover3 = freq; updateCoefficients(); }
    
    void updateCoefficients() {
        calculateButterworthLP(lowpass1a, xover1, sampleRate);
        lowpass1b = lowpass1a;
        calculateButterworthHP(highpass1a, xover1, sampleRate);
        highpass1b = highpass1a;

        calculateButterworthLP(lowpass2a, xover2, sampleRate);
        lowpass2b = lowpass2a;
        calculateButterworthHP(highpass2a, xover2, sampleRate);
        highpass2b = highpass2a;

        calculateButterworthLP(lowpass3a, xover3, sampleRate);
        lowpass3b = lowpass3a;
        calculateButterworthHP(highpass3a, xover3, sampleRate);
        highpass3b = highpass3a;
    }
    
    void calculateButterworthLP(BiquadCoeffs& c, float freq, float sr) {
        const double w0 = 2.0 * juce::MathConstants<double>::pi * freq / sr;
        const double cosw0 = std::cos(w0);
        const double sinw0 = std::sin(w0);
        const double Q = 0.7071067811865476;
        const double alpha = sinw0 / (2.0 * Q);
        
        const double a0 = 1.0 + alpha;
        c.b0 = ((1.0 - cosw0) / 2.0) / a0;
        c.b1 = (1.0 - cosw0) / a0;
        c.b2 = ((1.0 - cosw0) / 2.0) / a0;
        c.a1 = (-2.0 * cosw0) / a0;
        c.a2 = (1.0 - alpha) / a0;
    }
    
    void calculateButterworthHP(BiquadCoeffs& c, float freq, float sr) {
        const double w0 = 2.0 * juce::MathConstants<double>::pi * freq / sr;
        const double cosw0 = std::cos(w0);
        const double sinw0 = std::sin(w0);
        const double Q = 0.7071067811865476;
        const double alpha = sinw0 / (2.0 * Q);
        
        const double a0 = 1.0 + alpha;
        c.b0 = ((1.0 + cosw0) / 2.0) / a0;
        c.b1 = (-(1.0 + cosw0)) / a0;
        c.b2 = ((1.0 + cosw0) / 2.0) / a0;
        c.a1 = (-2.0 * cosw0) / a0;
        c.a2 = (1.0 - alpha) / a0;
    }
    
    inline double processBiquad(double input, int channel, const BiquadCoeffs& c, BiquadState& s) {
        double output = c.b0 * input + c.b1 * s.x1[channel] + c.b2 * s.x2[channel]
                       - c.a1 * s.y1[channel] - c.a2 * s.y2[channel];
        
        s.x2[channel] = s.x1[channel];
        s.x1[channel] = input;
        s.y2[channel] = s.y1[channel];
        s.y1[channel] = output;
        
        return output;
    }
    
    void processSample(float inL, float inR,
                       float& band1L, float& band1R,
                       float& band2L, float& band2R,
                       float& band3L, float& band3R,
                       float& band4L, float& band4R) {
        // 밴드 1: LPF1 -> LPF1 (저역)
        double lp1L = processBiquad(inL, 0, lowpass1a, lp1StateA);
        double lp1R = processBiquad(inR, 1, lowpass1a, lp1StateA);
        double band1OutL = processBiquad(lp1L, 0, lowpass1b, lp1StateB);
        double band1OutR = processBiquad(lp1R, 1, lowpass1b, lp1StateB);

        // 크로스오버 1 이상
        double hp1L = processBiquad(inL, 0, highpass1a, hp1StateA);
        double hp1R = processBiquad(inR, 1, highpass1a, hp1StateA);
        double highFrom1L = processBiquad(hp1L, 0, highpass1b, hp1StateB);
        double highFrom1R = processBiquad(hp1R, 1, highpass1b, hp1StateB);

        // 밴드 2: 크로스오버 1-2 사이
        double lp2L = processBiquad(highFrom1L, 0, lowpass2a, lp2StateA);
        double lp2R = processBiquad(highFrom1R, 1, lowpass2a, lp2StateA);
        double band2OutL = processBiquad(lp2L, 0, lowpass2b, lp2StateB);
        double band2OutR = processBiquad(lp2R, 1, lowpass2b, lp2StateB);

        // 크로스오버 2 이상
        double hp2L = processBiquad(highFrom1L, 0, highpass2a, hp2StateA);
        double hp2R = processBiquad(highFrom1R, 1, highpass2a, hp2StateA);
        double highFrom2L = processBiquad(hp2L, 0, highpass2b, hp2StateB);
        double highFrom2R = processBiquad(hp2R, 1, highpass2b, hp2StateB);

        // 밴드 3: 크로스오버 2-3 사이
        double lp3L = processBiquad(highFrom2L, 0, lowpass3a, lp3StateA);
        double lp3R = processBiquad(highFrom2R, 1, lowpass3a, lp3StateA);
        double band3OutL = processBiquad(lp3L, 0, lowpass3b, lp3StateB);
        double band3OutR = processBiquad(lp3R, 1, lowpass3b, lp3StateB);

        // 밴드 4: 크로스오버 3 이상 (고역)
        double hp3L = processBiquad(highFrom2L, 0, highpass3a, hp3StateA);
        double hp3R = processBiquad(highFrom2R, 1, highpass3a, hp3StateA);
        double band4OutL = processBiquad(hp3L, 0, highpass3b, hp3StateB);
        double band4OutR = processBiquad(hp3R, 1, highpass3b, hp3StateB);

        band1L = static_cast<float>(band1OutL);
        band1R = static_cast<float>(band1OutR);
        band2L = static_cast<float>(band2OutL);
        band2R = static_cast<float>(band2OutR);
        band3L = static_cast<float>(band3OutL);
        band3R = static_cast<float>(band3OutR);
        band4L = static_cast<float>(band4OutL);
        band4R = static_cast<float>(band4OutR);
    }
    
    void reset() {
        lp1StateA.reset(); lp1StateB.reset();
        hp1StateA.reset(); hp1StateB.reset();
        lp2StateA.reset(); lp2StateB.reset();
        hp2StateA.reset(); hp2StateB.reset();
        lp3StateA.reset(); lp3StateB.reset();
        hp3StateA.reset(); hp3StateB.reset();
    }
};

//=======================================================================
// 유틸리티 함수
//=======================================================================
inline float normalizedToFrequency(float normalized) {
    return kMinFreq * std::pow(kMaxFreq / kMinFreq, normalized);
}

inline float normalizedToCompThreshDb(float normalized) {
    return (normalized - 1.0f) * 36.0f; // -36 to 0 dB
}

inline float normalizedToCompMakeupDb(float normalized) {
    return (normalized - 0.5f) * 24.0f; // -12 to +12 dB
}

inline float normalizedToLimiterDb(float normalized) {
    return (normalized - 1.0f) * 24.0f; // -24 to 0 dB
}

inline float normalizedToLimiterReleaseMs(float normalized) {
    return 10.0f + normalized * 490.0f;  // 10ms to 500ms
}

inline float dbToLinear(float db) {
    return std::pow(10.0f, db / 20.0f);
}

} // namespace ELC4L
