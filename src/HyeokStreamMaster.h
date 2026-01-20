//-------------------------------------------------------------------------------------------------------
// ELC4L - Elite 4-Band Compressor + Limiter
// Premium VST2 Plugin with Pro-Q style interface
//-------------------------------------------------------------------------------------------------------

#pragma once

#ifndef __HyeokStreamMaster__
#define __HyeokStreamMaster__

#include "audioeffectx.h"
#include <cmath>
#include <algorithm>

//-------------------------------------------------------------------------------------------------------
// Parameter indices
//-------------------------------------------------------------------------------------------------------
enum HyeokParameters {
    kParamBand1Thresh = 0,  // Band 1 threshold (dB)
    kParamBand2Thresh,      // Band 2 threshold (dB)
    kParamBand3Thresh,      // Band 3 threshold (dB)
    kParamBand4Thresh,      // Band 4 threshold (dB)
    kParamBand1Makeup,      // Band 1 makeup (dB)
    kParamBand2Makeup,      // Band 2 makeup (dB)
    kParamBand3Makeup,      // Band 3 makeup (dB)
    kParamBand4Makeup,      // Band 4 makeup (dB)
    kParamXover1,           // Crossover 1 (Hz)
    kParamXover2,           // Crossover 2 (Hz)
    kParamXover3,           // Crossover 3 (Hz)
    kParamLimiterThresh,    // Limiter threshold (dB)
    kParamLimiterCeiling,   // Limiter output ceiling (dB)
    kParamLimiterRelease,   // Limiter release time (ms)
    kNumParams
};

//-------------------------------------------------------------------------------------------------------
// Plugin constants
//-------------------------------------------------------------------------------------------------------
constexpr VstInt32 kUniqueID = CCONST('E', 'L', 'C', '4');  // 'ELC4'
constexpr VstInt32 kVersion = 2000;  // 2.0.0.0
constexpr VstInt32 kNumPrograms = 1;

// Default parameter values (normalized 0-1)
constexpr float kDefaultBandThresh = 0.75f;   // ~-9 dB
constexpr float kDefaultBandMakeup = 0.5f;    // 0 dB
constexpr float kDefaultXover1 = 0.20f;       // ~80 Hz
constexpr float kDefaultXover2 = 0.50f;       // ~630 Hz
constexpr float kDefaultXover3 = 0.80f;       // ~5 kHz
constexpr float kDefaultLimiterThresh = 0.75f;   // -6 dB threshold
constexpr float kDefaultLimiterCeiling = 0.9583f; // -1 dB ceiling
constexpr float kDefaultLimiterRelease = 0.3f;    // ~120ms release

// Frequency range (Hz)
constexpr float kMinFreq = 20.0f;
constexpr float kMaxFreq = 20000.0f;

// High-Resolution Spectrum Analyzer (Pro-Q style)
constexpr int kFftSize = 4096;         // 4096-point FFT (~10Hz resolution at 44.1kHz)
constexpr int kSpectrumBins = 512;     // Internal processing bins
constexpr int kDisplayBins = 128;      // Smooth display points for Bezier curves
constexpr int kFftHopSize = 1024;      // Hop size for 75% overlap

//-------------------------------------------------------------------------------------------------------
// Lookahead Limiter (Brickwall with lookahead for transparent limiting)
//-------------------------------------------------------------------------------------------------------
struct LookaheadLimiter {
    static constexpr int kLookaheadSamples = 64;  // ~1.5ms at 44.1kHz
    
    // Delay buffers
    float delayL[kLookaheadSamples];
    float delayR[kLookaheadSamples];
    int delayIndex;
    
    // Envelope follower
    float envelope;
    float attackCoeff;
    float releaseCoeff;
    float fastReleaseCoeff;
    float slowReleaseCoeff;
    
    // Settings
    float threshold;    // Linear threshold
    float ceiling;      // Linear ceiling (output)
    float makeupGain;   // Automatic makeup gain
    float sampleRate;
    float lastGain;
    float gainReductionDb;
    float releaseMs;    // User-adjustable release time
    
    LookaheadLimiter() 
        : delayIndex(0)
        , envelope(0.0f)
        , attackCoeff(0.0f)
        , releaseCoeff(0.0f)
        , fastReleaseCoeff(0.0f)
        , slowReleaseCoeff(0.0f)
        , threshold(0.5f)
        , ceiling(0.891f)  // -1 dB
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
        // ARC-style dual release based on user setting
        float fastReleaseMs = releaseMs * 0.4f;     // Fast = 40% of setting
        float slowReleaseMs = releaseMs * 4.0f;     // Slow = 400% of setting
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
// LA-2A Style Opto Compressor (per-band)
// - Variable ratio (soft-knee, ~3:1 to infinity based on input level)
// - Program-dependent attack (~10ms) and dual-release (60ms fast + 1-15s slow)
// - RMS detection for natural opto response
// - Tube saturation emulation (T4B optical cell + 12AX7 tube stage)
//-------------------------------------------------------------------------------------------------------
struct OptoCompressor {
    float sampleRate;
    float envelope;
    float fastEnvelope;      // Fast release envelope
    float slowEnvelope;      // Slow release envelope (LA-2A dual time constant)
    float attackCoeff;
    float fastReleaseCoeff;  // ~60ms fast release
    float slowReleaseCoeff;  // ~1-15s slow release (program dependent)
    float threshold;
    float makeupGain;
    float lastGain;
    float gainReductionDb;
    float peakHold;          // Peak hold for ratio calculation
    float peakDecay;         // Peak decay coefficient
    
    // Tube saturation parameters
    float saturationDrive;   // 0.0 = clean, 1.0 = fully saturated
    bool saturationEnabled;

    // LA-2A constants
    static constexpr float kMinRatio = 3.0f;     // Low level ratio (~3:1)
    static constexpr float kMaxRatio = 100.0f;   // High level ratio (limiting)
    static constexpr float kKneeDb = 10.0f;      // Wide soft-knee for opto
    static constexpr float kAttackMs = 10.0f;    // LA-2A attack ~10ms
    static constexpr float kFastReleaseMs = 60.0f;   // Fast release ~60ms
    static constexpr float kSlowReleaseBase = 500.0f; // Base slow release ~500ms
    static constexpr float kSlowReleaseMax = 5000.0f; // Max slow release ~5s

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
        , saturationDrive(0.3f)    // Default subtle saturation
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
    
    // LA-2A style tube saturation (12AX7 + T4B optical cell emulation)
    // Soft asymmetric clipping with even and odd harmonics
    inline float applyTubeSaturation(float sample) const {
        if (!saturationEnabled || saturationDrive < 0.001f) return sample;
        
        // Input scaling based on drive
        float drive = 1.0f + saturationDrive * 4.0f;  // 1x to 5x gain
        float x = sample * drive;
        
        // Tube saturation model:
        // - Soft clipping with asymmetry (more positive compression)
        // - Even harmonics from triode asymmetry
        // - Gentle limiting at extremes
        
        // Asymmetric soft clipping (triode-like)
        float pos = x >= 0.0f ? x : 0.0f;
        float neg = x < 0.0f ? x : 0.0f;
        
        // Positive half: softer clipping (more headroom)
        float satPos = pos / (1.0f + 0.3f * pos * pos);
        
        // Negative half: slightly harder clipping (typical triode behavior)
        float satNeg = neg / (1.0f + 0.4f * neg * neg);
        
        float saturated = satPos + satNeg;
        
        // Add subtle even harmonics (2nd harmonic injection)
        float evenHarmonic = 0.05f * saturationDrive * saturated * saturated;
        saturated += evenHarmonic;
        
        // Output scaling to maintain level
        saturated /= drive * 0.7f;
        
        // Blend dry/wet based on drive amount
        float wet = saturationDrive * 0.7f;  // Max 70% wet
        return sample * (1.0f - wet) + saturated * wet;
    }

    void updateCoefficients() {
        attackCoeff = expf(-1.0f / (sampleRate * kAttackMs / 1000.0f));
        fastReleaseCoeff = expf(-1.0f / (sampleRate * kFastReleaseMs / 1000.0f));
        slowReleaseCoeff = expf(-1.0f / (sampleRate * kSlowReleaseBase / 1000.0f));
        peakDecay = expf(-1.0f / (sampleRate * 0.5f));  // 500ms peak decay
    }

    void process(float& left, float& right) {
        // RMS detection (LA-2A is RMS-based)
        float level = 0.5f * (left * left + right * right);
        float detector = sqrtf(level + 1.0e-12f);

        // Track peak for dynamic ratio calculation
        if (detector > peakHold) {
            peakHold = detector;
        } else {
            peakHold = peakDecay * peakHold + (1.0f - peakDecay) * detector;
        }

        // Dual time constant envelope (LA-2A style)
        // Fast envelope
        if (detector > fastEnvelope) {
            fastEnvelope = attackCoeff * fastEnvelope + (1.0f - attackCoeff) * detector;
        } else {
            fastEnvelope = fastReleaseCoeff * fastEnvelope + (1.0f - fastReleaseCoeff) * detector;
        }

        // Slow envelope with program-dependent release
        float overDb = 20.0f * log10f((peakHold / threshold) + 1.0e-12f);
        if (overDb < 0.0f) overDb = 0.0f;
        
        // More compression = slower release (LA-2A characteristic)
        float releaseScale = 1.0f + (overDb * 0.15f);  // Scale release by compression amount
        if (releaseScale > 10.0f) releaseScale = 10.0f;
        float slowRelMs = kSlowReleaseBase * releaseScale;
        if (slowRelMs > kSlowReleaseMax) slowRelMs = kSlowReleaseMax;
        float dynamicSlowCoeff = expf(-1.0f / (sampleRate * slowRelMs / 1000.0f));

        if (detector > slowEnvelope) {
            slowEnvelope = attackCoeff * slowEnvelope + (1.0f - attackCoeff) * detector;
        } else {
            slowEnvelope = dynamicSlowCoeff * slowEnvelope + (1.0f - dynamicSlowCoeff) * detector;
        }

        // Combined envelope (weighted average - LA-2A uses both)
        envelope = 0.3f * fastEnvelope + 0.7f * slowEnvelope;

        // Calculate dB values
        float levelDb = 20.0f * log10f(envelope + 1.0e-12f);
        float threshDb = 20.0f * log10f(threshold + 1.0e-12f);
        float overThresh = levelDb - threshDb;

        // LA-2A variable ratio: higher input = higher ratio
        // At threshold: ~3:1, at +20dB over threshold: ~infinity:1
        float dynamicRatio = kMinRatio;
        if (overThresh > 0.0f) {
            float ratioBlend = overThresh / 20.0f;  // 0-1 over 20dB range
            if (ratioBlend > 1.0f) ratioBlend = 1.0f;
            // Exponential curve for ratio increase
            dynamicRatio = kMinRatio + (kMaxRatio - kMinRatio) * (ratioBlend * ratioBlend);
        }

        // Soft-knee gain reduction calculation
        float grDb = 0.0f;
        if (overThresh <= -kKneeDb * 0.5f) {
            grDb = 0.0f;
        } else if (overThresh >= kKneeDb * 0.5f) {
            grDb = overThresh - (overThresh / dynamicRatio);
        } else {
            // Quadratic interpolation in knee region
            float x = overThresh + kKneeDb * 0.5f;
            grDb = (x * x) * (1.0f - 1.0f / dynamicRatio) / (2.0f * kKneeDb);
        }

        float gain = powf(10.0f, -grDb / 20.0f);

        // Smooth gain changes to avoid pumping
        float gainSmooth = 0.995f;
        gain = gainSmooth * lastGain + (1.0f - gainSmooth) * gain;

        left *= gain * makeupGain;
        right *= gain * makeupGain;
        
        // Apply LA-2A style tube saturation after compression
        left = applyTubeSaturation(left);
        right = applyTubeSaturation(right);

        lastGain = gain;
        gainReductionDb = grDb;
    }

    float getGainReductionDb() const { return gainReductionDb; }
};

//-------------------------------------------------------------------------------------------------------
// LUFS Meter (momentary, approximate)
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
        // 400 ms momentary window
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
// HyeokStreamDSP - Linkwitz-Riley 4th-order Crossover (4-band)
//-------------------------------------------------------------------------------------------------------
struct HyeokStreamDSP {
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
    
    HyeokStreamDSP() 
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
    
    void setXover1(float freq) {
        xover1 = freq;
        updateCoefficients();
    }
    
    void setXover2(float freq) {
        xover2 = freq;
        updateCoefficients();
    }

    void setXover3(float freq) {
        xover3 = freq;
        updateCoefficients();
    }
    
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
        const double w0 = 2.0 * 3.14159265358979323846 * freq / sr;
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
        const double w0 = 2.0 * 3.14159265358979323846 * freq / sr;
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
        lp1StateA.reset();
        lp1StateB.reset();
        hp1StateA.reset();
        hp1StateB.reset();
        lp2StateA.reset();
        lp2StateB.reset();
        hp2StateA.reset();
        hp2StateB.reset();
        lp3StateA.reset();
        lp3StateB.reset();
        hp3StateA.reset();
        hp3StateB.reset();
    }
};

//-------------------------------------------------------------------------------------------------------
// Forward declaration
//-------------------------------------------------------------------------------------------------------
class HyeokStreamEditor;

//-------------------------------------------------------------------------------------------------------
// HyeokStreamMaster - Main plugin class
//-------------------------------------------------------------------------------------------------------
class HyeokStreamMaster : public AudioEffectX {
public:
    HyeokStreamMaster(audioMasterCallback audioMaster);
    virtual ~HyeokStreamMaster();
    
    virtual void processReplacing(float** inputs, float** outputs, VstInt32 sampleFrames) override;
    
    virtual void setParameter(VstInt32 index, float value) override;
    virtual float getParameter(VstInt32 index) override;
    virtual void getParameterLabel(VstInt32 index, char* label) override;
    virtual void getParameterDisplay(VstInt32 index, char* text) override;
    virtual void getParameterName(VstInt32 index, char* text) override;
    
    virtual void setProgramName(char* name) override;
    virtual void getProgramName(char* name) override;
    
    virtual bool getEffectName(char* name) override;
    virtual bool getVendorString(char* text) override;
    virtual bool getProductString(char* text) override;
    virtual VstInt32 getVendorVersion() override;
    virtual VstInt32 canDo(char* text) override;
    
    virtual void setSampleRate(float sampleRate) override;
    virtual void suspend() override;
    virtual void resume() override;
    
    virtual VstInt32 getGetTailSize() override { return LookaheadLimiter::kLookaheadSamples; }

    float getParameterValue(VstInt32 index) const { return parameters[index]; }
    float getLufsMomentary() const { return lufsMeter.getMomentary(); }
    const float* getSpectrumIn() const { return spectrumIn; }
    const float* getSpectrumOut() const { return spectrumOut; }
    const float* getDisplayIn() const { return displayIn; }    // Bezier-ready display buffer
    const float* getDisplayOut() const { return displayOut; }  // Bezier-ready display buffer
    float getInputDb() const { return inputDb; }
    float getOutputDb() const { return outputDb; }
    float getBandGrDb(int band) const { return bandGrDb[band]; }
    float getLimiterGrDb() const { return limiterGrDb; }
    
    // Get crossover frequencies for UI display
    float getXover1Hz() const { return dsp.xover1; }
    float getXover2Hz() const { return dsp.xover2; }
    float getXover3Hz() const { return dsp.xover3; }
    
    // Get crossover frequency by index (0, 1, 2)
    float getCrossoverFreq(int index) const {
        switch (index) {
            case 0: return dsp.xover1;
            case 1: return dsp.xover2;
            case 2: return dsp.xover3;
            default: return 1000.0f;
        }
    }
    
    // Band monitoring controls (Mute/Solo/Delta)
    bool getBandMute(int band) const { return (band >= 0 && band < 4) ? bandMute[band] : false; }
    bool getBandSolo(int band) const { return (band >= 0 && band < 4) ? bandSolo[band] : false; }
    bool getBandDelta(int band) const { return (band >= 0 && band < 4) ? bandDelta[band] : false; }
    void setBandMute(int band, bool state) { if (band >= 0 && band < 4) bandMute[band] = state; }
    void setBandSolo(int band, bool state) { if (band >= 0 && band < 4) bandSolo[band] = state; }
    void setBandDelta(int band, bool state) { if (band >= 0 && band < 4) bandDelta[band] = state; }
    void toggleBandMute(int band) { if (band >= 0 && band < 4) bandMute[band] = !bandMute[band]; }
    void toggleBandSolo(int band) { if (band >= 0 && band < 4) bandSolo[band] = !bandSolo[band]; }
    void toggleBandDelta(int band) { if (band >= 0 && band < 4) bandDelta[band] = !bandDelta[band]; }
    
    // Bypass controls
    bool getBandBypass(int band) const { return (band >= 0 && band < 4) ? bandBypass[band] : false; }
    bool getLimiterBypass() const { return limiterBypass; }
    void setBandBypass(int band, bool state) { if (band >= 0 && band < 4) bandBypass[band] = state; }
    void setLimiterBypass(bool state) { limiterBypass = state; }
    void toggleBandBypass(int band) { if (band >= 0 && band < 4) bandBypass[band] = !bandBypass[band]; }
    void toggleLimiterBypass() { limiterBypass = !limiterBypass; }

private:
    float normalizedToFrequency(float normalized) const {
        return kMinFreq * powf(kMaxFreq / kMinFreq, normalized);
    }
    
    float normalizedToCompThreshDb(float normalized) const {
        return (normalized - 1.0f) * 36.0f; // -36 to 0 dB
    }

    float normalizedToCompMakeupDb(float normalized) const {
        return (normalized - 0.5f) * 24.0f; // -12 to +12 dB
    }

    float normalizedToLimiterDb(float normalized) const {
        return (normalized - 1.0f) * 24.0f; // -24 to 0 dB
    }
    
    float normalizedToLimiterReleaseMs(float normalized) const {
        return 10.0f + normalized * 490.0f;  // 10ms to 500ms
    }
    
    float dbToLinear(float db) const {
        return powf(10.0f, db / 20.0f);
    }
    
    float parameters[kNumParams];
    char programName[kVstMaxProgNameLen + 1];
    
    HyeokStreamDSP dsp;
    OptoCompressor bandComps[4];
    LookaheadLimiter limiter;
    LufsMeter lufsMeter;

    // Meters
    float spectrumIn[kSpectrumBins];
    float spectrumOut[kSpectrumBins];
    float displayIn[kDisplayBins];       // Smoothed display buffer for Bezier
    float displayOut[kDisplayBins];      // Smoothed display buffer for Bezier
    float inputDb;
    float outputDb;
    float bandGrDb[4];
    float limiterGrDb;
    
    // Band monitoring state (Mute/Solo/Delta Listen)
    bool bandMute[4];
    bool bandSolo[4];
    bool bandDelta[4];
    
    // Bypass state (per-band + limiter)
    bool bandBypass[4];
    bool limiterBypass;

    // FFT buffers (4096-point high-resolution analyzer)
    float fftBufferIn[kFftSize];
    float fftBufferOut[kFftSize];
    float fftWindow[kFftSize];           // Pre-computed Blackman-Harris window
    float fftReal[kFftSize];             // FFT real part
    float fftImag[kFftSize];             // FFT imaginary part
    int fftBitReverse[kFftSize];         // Bit-reversal table for FFT
    int fftWritePos;
    bool fftInitialized;

    void updateCompressors();
    void updateFrequencies();
    void updateLimiter();
    void initFFT();                                           // Initialize FFT tables
    void performFFT(float* real, float* imag, int n);         // Cooley-Tukey FFT
    void computeSpectrum(const float* input, float* output);  // Compute spectrum with tilt
    void updateDisplayBuffers();                              // Downsample to Bezier-ready format
    void updateMeters(float inL, float inR, float outL, float outR);
};

#endif // __HyeokStreamMaster__
