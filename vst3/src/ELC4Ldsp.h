//-------------------------------------------------------------------------------------------------------
// ELC4L VST3 - DSP Core (shared between VST2 and VST3)
// - 4-Band Linkwitz-Riley Crossover
// - LA-2A Style Opto Compressor
// - Lookahead Brickwall Limiter
//-------------------------------------------------------------------------------------------------------
#pragma once

#include <cmath>
#include <algorithm>

namespace ELC4L {

// FFT/Spectrum constants
constexpr int kFftSize = 4096;
constexpr int kSpectrumBins = 512;
constexpr int kDisplayBins = 128;
constexpr int kFftHopSize = 1024;

//-------------------------------------------------------------------------------------------------------
// Lookahead Limiter (Brickwall with lookahead for transparent limiting)
//-------------------------------------------------------------------------------------------------------
struct LookaheadLimiter {
    static constexpr int kLookaheadSamples = 64;
    
    float delayL[kLookaheadSamples];
    float delayR[kLookaheadSamples];
    int delayIndex;
    
    float envelope;
    float attackCoeff;
    float releaseCoeff;
    float fastReleaseCoeff;
    float slowReleaseCoeff;
    
    float threshold;
    float ceiling;
    float makeupGain;
    float sampleRate;
    float lastGain;
    float gainReductionDb;
    float releaseMs;
    
    LookaheadLimiter() 
        : delayIndex(0)
        , envelope(0.0f)
        , attackCoeff(0.0f)
        , releaseCoeff(0.0f)
        , fastReleaseCoeff(0.0f)
        , slowReleaseCoeff(0.0f)
        , threshold(0.5f)
        , ceiling(0.891f)
        , makeupGain(1.0f)
        , sampleRate(44100.0f)
        , lastGain(1.0f)
        , gainReductionDb(0.0f)
        , releaseMs(100.0f)
    {
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
        threshold = powf(10.0f, threshDb / 20.0f);
        updateMakeupGain();
    }
    
    void setCeiling(float ceilingDb) {
        ceiling = powf(10.0f, ceilingDb / 20.0f);
        updateMakeupGain();
    }
    
    void setRelease(float relMs) {
        releaseMs = relMs;
        if (releaseMs < 10.0f) releaseMs = 10.0f;
        if (releaseMs > 500.0f) releaseMs = 500.0f;
        updateCoefficients();
    }
    
    void updateCoefficients() {
        float attackMs = 0.1f;
        float fastReleaseMs = releaseMs * 0.4f;
        float slowReleaseMs = releaseMs * 4.0f;
        attackCoeff = expf(-1.0f / (sampleRate * attackMs / 1000.0f));
        releaseCoeff = expf(-1.0f / (sampleRate * releaseMs / 1000.0f));
        fastReleaseCoeff = expf(-1.0f / (sampleRate * fastReleaseMs / 1000.0f));
        slowReleaseCoeff = expf(-1.0f / (sampleRate * slowReleaseMs / 1000.0f));
    }
    
    void updateMakeupGain() {
        makeupGain = ceiling / threshold;
        if (makeupGain > 4.0f) makeupGain = 4.0f;
    }
    
    void process(float& left, float& right) {
        float delayedL = delayL[delayIndex];
        float delayedR = delayR[delayIndex];
        
        delayL[delayIndex] = left;
        delayR[delayIndex] = right;
        delayIndex = (delayIndex + 1) % kLookaheadSamples;
        
        float peakL = fabsf(left);
        float peakR = fabsf(right);
        float peak = (peakL > peakR) ? peakL : peakR;
        
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
        
        float gain = 1.0f;
        if (envelope > threshold) {
            gain = threshold / envelope;
        }
        
        float outL = delayedL * gain * makeupGain;
        float outR = delayedR * gain * makeupGain;
        
        if (outL > ceiling) outL = ceiling;
        if (outL < -ceiling) outL = -ceiling;
        if (outR > ceiling) outR = ceiling;
        if (outR < -ceiling) outR = -ceiling;
        
        left = outL;
        right = outR;

        lastGain = gain;
        gainReductionDb = (gain > 1.0e-9f) ? (-20.0f * log10f(gain)) : 60.0f;
    }

    float getGainReductionDb() const { return gainReductionDb; }
};

//-------------------------------------------------------------------------------------------------------
// LA-2A Style Opto Compressor
//-------------------------------------------------------------------------------------------------------
struct OptoCompressor {
    float sampleRate;
    float envelope;
    float fastEnvelope;
    float slowEnvelope;
    float attackCoeff;
    float fastReleaseCoeff;
    float slowReleaseCoeff;
    float threshold;
    float makeupGain;
    float lastGain;
    float gainReductionDb;
    float peakHold;
    float peakDecay;
    
    float saturationDrive;
    bool saturationEnabled;

    static constexpr float kMinRatio = 3.0f;
    static constexpr float kMaxRatio = 100.0f;
    static constexpr float kKneeDb = 10.0f;
    static constexpr float kAttackMs = 10.0f;
    static constexpr float kFastReleaseMs = 60.0f;
    static constexpr float kSlowReleaseBase = 500.0f;
    static constexpr float kSlowReleaseMax = 5000.0f;

    OptoCompressor()
        : sampleRate(44100.0f)
        , envelope(0.0f)
        , fastEnvelope(0.0f)
        , slowEnvelope(0.0f)
        , attackCoeff(0.0f)
        , fastReleaseCoeff(0.0f)
        , slowReleaseCoeff(0.0f)
        , threshold(1.0f)
        , makeupGain(1.0f)
        , lastGain(1.0f)
        , gainReductionDb(0.0f)
        , peakHold(0.0f)
        , peakDecay(0.0f)
        , saturationDrive(0.3f)
        , saturationEnabled(true)
    {
        updateCoefficients();
    }

    void reset() {
        envelope = 0.0f;
        fastEnvelope = 0.0f;
        slowEnvelope = 0.0f;
        lastGain = 1.0f;
        gainReductionDb = 0.0f;
        peakHold = 0.0f;
    }

    void setSampleRate(float sr) {
        sampleRate = sr;
        updateCoefficients();
    }

    void setThresholdDb(float db) {
        threshold = powf(10.0f, db / 20.0f);
    }

    void setMakeupDb(float db) {
        makeupGain = powf(10.0f, db / 20.0f);
    }
    
    void setSaturationDrive(float drive) {
        saturationDrive = (drive < 0.0f) ? 0.0f : (drive > 1.0f) ? 1.0f : drive;
    }
    
    void setSaturationEnabled(bool enabled) {
        saturationEnabled = enabled;
    }
    
    inline float applyTubeSaturation(float sample) const {
        if (!saturationEnabled || saturationDrive < 0.001f) return sample;
        
        float drive = 1.0f + saturationDrive * 4.0f;
        float x = sample * drive;
        
        float pos = x >= 0.0f ? x : 0.0f;
        float neg = x < 0.0f ? x : 0.0f;
        
        float satPos = pos / (1.0f + 0.3f * pos * pos);
        float satNeg = neg / (1.0f + 0.4f * neg * neg);
        
        float saturated = satPos + satNeg;
        float evenHarmonic = 0.05f * saturationDrive * saturated * saturated;
        saturated += evenHarmonic;
        saturated /= drive * 0.7f;
        
        float wet = saturationDrive * 0.7f;
        return sample * (1.0f - wet) + saturated * wet;
    }

    void updateCoefficients() {
        attackCoeff = expf(-1.0f / (sampleRate * kAttackMs / 1000.0f));
        fastReleaseCoeff = expf(-1.0f / (sampleRate * kFastReleaseMs / 1000.0f));
        slowReleaseCoeff = expf(-1.0f / (sampleRate * kSlowReleaseBase / 1000.0f));
        peakDecay = expf(-1.0f / (sampleRate * 0.5f));
    }

    void process(float& left, float& right) {
        float level = 0.5f * (left * left + right * right);
        float detector = sqrtf(level + 1.0e-12f);

        if (detector > peakHold) {
            peakHold = detector;
        } else {
            peakHold = peakDecay * peakHold + (1.0f - peakDecay) * detector;
        }

        if (detector > fastEnvelope) {
            fastEnvelope = attackCoeff * fastEnvelope + (1.0f - attackCoeff) * detector;
        } else {
            fastEnvelope = fastReleaseCoeff * fastEnvelope + (1.0f - fastReleaseCoeff) * detector;
        }

        float overDb = 20.0f * log10f((peakHold / threshold) + 1.0e-12f);
        if (overDb < 0.0f) overDb = 0.0f;
        
        float releaseScale = 1.0f + (overDb * 0.15f);
        if (releaseScale > 10.0f) releaseScale = 10.0f;
        float slowRelMs = kSlowReleaseBase * releaseScale;
        if (slowRelMs > kSlowReleaseMax) slowRelMs = kSlowReleaseMax;
        float dynamicSlowCoeff = expf(-1.0f / (sampleRate * slowRelMs / 1000.0f));

        if (detector > slowEnvelope) {
            slowEnvelope = attackCoeff * slowEnvelope + (1.0f - attackCoeff) * detector;
        } else {
            slowEnvelope = dynamicSlowCoeff * slowEnvelope + (1.0f - dynamicSlowCoeff) * detector;
        }

        envelope = 0.3f * fastEnvelope + 0.7f * slowEnvelope;

        float levelDb = 20.0f * log10f(envelope + 1.0e-12f);
        float threshDb = 20.0f * log10f(threshold + 1.0e-12f);
        float overThresh = levelDb - threshDb;

        float dynamicRatio = kMinRatio;
        if (overThresh > 0.0f) {
            float ratioBlend = overThresh / 20.0f;
            if (ratioBlend > 1.0f) ratioBlend = 1.0f;
            dynamicRatio = kMinRatio + (kMaxRatio - kMinRatio) * (ratioBlend * ratioBlend);
        }

        float grDb = 0.0f;
        if (overThresh <= -kKneeDb * 0.5f) {
            grDb = 0.0f;
        } else if (overThresh >= kKneeDb * 0.5f) {
            grDb = overThresh - (overThresh / dynamicRatio);
        } else {
            float x = overThresh + kKneeDb * 0.5f;
            grDb = (x * x) * (1.0f - 1.0f / dynamicRatio) / (2.0f * kKneeDb);
        }

        float gain = powf(10.0f, -grDb / 20.0f);

        float gainSmooth = 0.995f;
        gain = gainSmooth * lastGain + (1.0f - gainSmooth) * gain;

        left *= gain * makeupGain;
        right *= gain * makeupGain;
        
        left = applyTubeSaturation(left);
        right = applyTubeSaturation(right);

        lastGain = gain;
        gainReductionDb = grDb;
    }

    float getGainReductionDb() const { return gainReductionDb; }
};

//-------------------------------------------------------------------------------------------------------
// 4-Band Linkwitz-Riley Crossover
//-------------------------------------------------------------------------------------------------------
struct FourBandCrossover {
    struct BiquadCoeffs {
        double b0, b1, b2;
        double a1, a2;
    };
    
    struct BiquadState {
        double x1[2], x2[2];
        double y1[2], y2[2];
        
        BiquadState() { reset(); }
        
        void reset() {
            x1[0] = x1[1] = x2[0] = x2[1] = 0.0;
            y1[0] = y1[1] = y2[0] = y2[1] = 0.0;
        }
    };
    
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

    float sampleRate;
    float xover1;
    float xover2;
    float xover3;
    
    FourBandCrossover() 
        : sampleRate(44100.0f)
        , xover1(120.0f)
        , xover2(800.0f)
        , xover3(4000.0f)
    {
        updateCoefficients();
    }
    
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
        const double pi = 3.14159265358979323846;
        const double w0 = 2.0 * pi * freq / sr;
        const double cosw0 = cos(w0);
        const double sinw0 = sin(w0);
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
        const double pi = 3.14159265358979323846;
        const double w0 = 2.0 * pi * freq / sr;
        const double cosw0 = cos(w0);
        const double sinw0 = sin(w0);
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
        double lp1L = processBiquad(inL, 0, lowpass1a, lp1StateA);
        double lp1R = processBiquad(inR, 1, lowpass1a, lp1StateA);
        double band1OutL = processBiquad(lp1L, 0, lowpass1b, lp1StateB);
        double band1OutR = processBiquad(lp1R, 1, lowpass1b, lp1StateB);

        double hp1L = processBiquad(inL, 0, highpass1a, hp1StateA);
        double hp1R = processBiquad(inR, 1, highpass1a, hp1StateA);
        double highFrom1L = processBiquad(hp1L, 0, highpass1b, hp1StateB);
        double highFrom1R = processBiquad(hp1R, 1, highpass1b, hp1StateB);

        double lp2L = processBiquad(highFrom1L, 0, lowpass2a, lp2StateA);
        double lp2R = processBiquad(highFrom1R, 1, lowpass2a, lp2StateA);
        double band2OutL = processBiquad(lp2L, 0, lowpass2b, lp2StateB);
        double band2OutR = processBiquad(lp2R, 1, lowpass2b, lp2StateB);

        double hp2L = processBiquad(highFrom1L, 0, highpass2a, hp2StateA);
        double hp2R = processBiquad(highFrom1R, 1, highpass2a, hp2StateA);
        double highFrom2L = processBiquad(hp2L, 0, highpass2b, hp2StateB);
        double highFrom2R = processBiquad(hp2R, 1, highpass2b, hp2StateB);

        double lp3L = processBiquad(highFrom2L, 0, lowpass3a, lp3StateA);
        double lp3R = processBiquad(highFrom2R, 1, lowpass3a, lp3StateA);
        double band3OutL = processBiquad(lp3L, 0, lowpass3b, lp3StateB);
        double band3OutR = processBiquad(lp3R, 1, lowpass3b, lp3StateB);

        double hp3L = processBiquad(highFrom2L, 0, highpass3a, hp3StateA);
        double hp3R = processBiquad(highFrom2R, 1, highpass3a, hp3StateA);
        double band4OutL = processBiquad(hp3L, 0, highpass3b, hp3StateB);
        double band4OutR = processBiquad(hp3R, 1, highpass3b, hp3StateB);

        band1L = (float)band1OutL;
        band1R = (float)band1OutR;
        band2L = (float)band2OutL;
        band2R = (float)band2OutR;
        band3L = (float)band3OutL;
        band3R = (float)band3OutR;
        band4L = (float)band4OutL;
        band4R = (float)band4OutR;
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

//-------------------------------------------------------------------------------------------------------
// LUFS Meter
//-------------------------------------------------------------------------------------------------------
struct LufsMeter {
    float sampleRate;
    float momentaryEnergy;
    float momentaryCoeff;
    float momentaryLufs;

    LufsMeter()
        : sampleRate(44100.0f)
        , momentaryEnergy(0.0f)
        , momentaryCoeff(0.0f)
        , momentaryLufs(-120.0f)
    {
        updateCoefficients();
    }

    void reset() {
        momentaryEnergy = 0.0f;
        momentaryLufs = -120.0f;
    }

    void setSampleRate(float sr) {
        sampleRate = sr;
        updateCoefficients();
    }

    void updateCoefficients() {
        float tau = 0.4f;
        momentaryCoeff = expf(-1.0f / (sampleRate * tau));
    }

    void process(float left, float right) {
        float energy = 0.5f * (left * left + right * right);
        momentaryEnergy = momentaryCoeff * momentaryEnergy + (1.0f - momentaryCoeff) * energy;
        float safeEnergy = (momentaryEnergy > 1.0e-12f) ? momentaryEnergy : 1.0e-12f;
        momentaryLufs = -0.691f + 10.0f * log10f(safeEnergy);
    }

    float getMomentary() const { return momentaryLufs; }
};

//-------------------------------------------------------------------------------------------------------
// Utility functions
//-------------------------------------------------------------------------------------------------------
inline float normalizedToFrequency(float normalized) {
    constexpr float kMinFreq = 20.0f;
    constexpr float kMaxFreq = 20000.0f;
    return kMinFreq * powf(kMaxFreq / kMinFreq, normalized);
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
    return 10.0f + normalized * 490.0f; // 10ms to 500ms
}

inline float dbToLinear(float db) {
    return powf(10.0f, db / 20.0f);
}

} // namespace ELC4L
