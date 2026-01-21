//-------------------------------------------------------------------------------------------------------
// HyeokStreamMaster - VST2 Plugin for OBS
// Core plugin implementation with 4-Band Opto Compressor + Limiter + LUFS
//-------------------------------------------------------------------------------------------------------

#include "HyeokStreamMaster.h"
#include "HyeokStreamEditor.h"
#include <cstdio>
#include <cstring>

// 32-tap Polyphase FIR Coefficients (Kaiser Windowed Sinc)
const float PolyphaseOversampler::kCoeffs[32] = {
    // Phase 0
    -0.002f, 0.005f, -0.012f, 0.025f, 0.965f, 0.025f, -0.012f, 0.005f,
    // Phase 1
    -0.004f, 0.010f, -0.025f, 0.060f, 0.880f, 0.090f, -0.025f, 0.010f,
    // Phase 2
    -0.005f, 0.015f, -0.040f, 0.120f, 0.750f, 0.180f, -0.040f, 0.015f,
    // Phase 3
    -0.004f, 0.012f, -0.045f, 0.220f, 0.550f, 0.280f, -0.045f, 0.012f
};

//-------------------------------------------------------------------------------------------------------
// HyeokStreamMaster implementation
//-------------------------------------------------------------------------------------------------------
HyeokStreamMaster::HyeokStreamMaster(audioMasterCallback audioMaster)
    : AudioEffectX(audioMaster, kNumPrograms, kNumParams)
{
    setUniqueID(kUniqueID);
    setNumInputs(2);
    setNumOutputs(2);
    canProcessReplacing();
    
    setInitialDelay(LookaheadLimiter::kLookaheadSamples);
    
    cEffect.flags |= effFlagsHasEditor;
    
    strcpy(programName, "Default");
    
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

    for (int i = 0; i < kSpectrumBins; ++i) {
        spectrumIn[i] = -90.0f;
        spectrumOut[i] = -90.0f;
    }
    for (int i = 0; i < kDisplayBins; ++i) {
        displayIn[i] = -90.0f;
        displayOut[i] = -90.0f;
    }
    for (int i = 0; i < kFftSize; ++i) {
        fftBufferIn[i] = 0.0f;
        fftBufferOut[i] = 0.0f;
    }
    fftWritePos = 0;
    fftInitialized = false;
    inputDb = -120.0f;
    outputDb = -120.0f;
    for (int i = 0; i < 4; ++i) {
        bandGrDb[i] = 0.0f;
        bandMute[i] = false;
        bandSolo[i] = false;
        bandDelta[i] = false;
        bandBypass[i] = false;
    }
    limiterGrDb = 0.0f;
    limiterBypass = false;
    
    initFFT();  // Initialize FFT tables
    
    updateCompressors();
    updateFrequencies();
    updateLimiter();
    
    setEditor(new HyeokStreamEditor(this));
}

HyeokStreamMaster::~HyeokStreamMaster() {
}

//-------------------------------------------------------------------------------------------------------
// Audio processing
//-------------------------------------------------------------------------------------------------------
void HyeokStreamMaster::processReplacing(float** inputs, float** outputs, VstInt32 sampleFrames) {
    float* inL = inputs[0];
    float* inR = inputs[1];
    float* outL = outputs[0];
    float* outR = outputs[1];
    
    // Check if any band is in Solo mode
    bool anySolo = bandSolo[0] || bandSolo[1] || bandSolo[2] || bandSolo[3];
    
    for (VstInt32 i = 0; i < sampleFrames; ++i) {
        float b1L, b1R, b2L, b2R, b3L, b3R, b4L, b4R;

        // 1. Split into 4 bands
        dsp.processSample(inL[i], inR[i], b1L, b1R, b2L, b2R, b3L, b3R, b4L, b4R);

        // Store uncompressed band signals for Delta calculation
        float bandInputL[4] = { b1L, b2L, b3L, b4L };
        float bandInputR[4] = { b1R, b2R, b3R, b4R };

        // [MOVED UP] Calculate makeup gains FIRST (needed for bypass logic)
        float makeupGains[4];
        for (int b = 0; b < 4; ++b) {
            float makeupDb = normalizedToCompMakeupDb(parameters[kParamBand1Makeup + b]);
            makeupGains[b] = dbToLinear(makeupDb);
        }

        // 2. Process each band (Modified Bypass Logic)
        // Band 1
        if (!bandBypass[0]) {
            bandComps[0].process(b1L, b1R); // Active: Comp + Makeup + Sat
        } else {
            b1L *= makeupGains[0];          // Bypass: Makeup Only (Volume Match)
            b1R *= makeupGains[0];
        }

        // Band 2
        if (!bandBypass[1]) {
            bandComps[1].process(b2L, b2R);
        } else {
            b2L *= makeupGains[1];
            b2R *= makeupGains[1];
        }

        // Band 3
        if (!bandBypass[2]) {
            bandComps[2].process(b3L, b3R);
        } else {
            b3L *= makeupGains[2];
            b3R *= makeupGains[2];
        }

        // Band 4
        if (!bandBypass[3]) {
            bandComps[3].process(b4L, b4R);
        } else {
            b4L *= makeupGains[3];
            b4R *= makeupGains[3];
        }
        
        // Band output arrays
        float bandOutL[4] = { b1L, b2L, b3L, b4L };
        float bandOutR[4] = { b1R, b2R, b3R, b4R };

        // 3. Apply Delta/Mute/Solo logic
        float mixL = 0.0f;
        float mixR = 0.0f;
        
        for (int b = 0; b < 4; ++b) {
            float outBandL, outBandR;
            
            if (bandDelta[b]) {
                // Delta Listen: play only the amount of reduction introduced by the compressor
                float reductionAmount = 0.0f;
                if (!bandBypass[b]) {
                    reductionAmount = 1.0f - bandComps[b].getCurrentGain();
                }

                outBandL = bandInputL[b] * reductionAmount;
                outBandR = bandInputR[b] * reductionAmount;
            } else {
                outBandL = bandOutL[b];
                outBandR = bandOutR[b];
            }
            
            // Solo/Mute logic
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

        // 4. Apply limiter
        if (!limiterBypass) {
            limiter.process(mixL, mixR);
        }
        lufsMeter.process(mixL, mixR);

        updateMeters(inL[i], inR[i], mixL, mixR);
        bandGrDb[0] = bandComps[0].getGainReductionDb();
        bandGrDb[1] = bandComps[1].getGainReductionDb();
        bandGrDb[2] = bandComps[2].getGainReductionDb();
        bandGrDb[3] = bandComps[3].getGainReductionDb();
        limiterGrDb = limiter.getGainReductionDb();

        outL[i] = mixL;
        outR[i] = mixR;
    }
}

//-------------------------------------------------------------------------------------------------------
// Parameters
//-------------------------------------------------------------------------------------------------------
void HyeokStreamMaster::setParameter(VstInt32 index, float value) {
    if (index < 0 || index >= kNumParams)
        return;
    
    if (value < 0.0f) value = 0.0f;
    if (value > 1.0f) value = 1.0f;
    
    parameters[index] = value;
    
    switch (index) {
        case kParamBand1Thresh:
        case kParamBand2Thresh:
        case kParamBand3Thresh:
        case kParamBand4Thresh:
        case kParamBand1Makeup:
        case kParamBand2Makeup:
        case kParamBand3Makeup:
        case kParamBand4Makeup:
            updateCompressors();
            break;
        case kParamXover1:
        case kParamXover2:
        case kParamXover3:
            updateFrequencies();
            break;
        case kParamLimiterThresh:
        case kParamLimiterCeiling:
        case kParamLimiterRelease:
            updateLimiter();
            break;
    }
    
    if (editor) {
        ((HyeokStreamEditor*)editor)->updateParameter(index);
    }
}

float HyeokStreamMaster::getParameter(VstInt32 index) {
    if (index < 0 || index >= kNumParams)
        return 0.0f;
    return parameters[index];
}

void HyeokStreamMaster::getParameterLabel(VstInt32 index, char* label) {
    switch (index) {
        case kParamBand1Thresh:
        case kParamBand2Thresh:
        case kParamBand3Thresh:
        case kParamBand4Thresh:
        case kParamBand1Makeup:
        case kParamBand2Makeup:
        case kParamBand3Makeup:
        case kParamBand4Makeup:
        case kParamLimiterThresh:
        case kParamLimiterCeiling:
            strcpy(label, "dB");
            break;
        case kParamLimiterRelease:
            strcpy(label, "ms");
            break;
        case kParamXover1:
        case kParamXover2:
        case kParamXover3:
            strcpy(label, "Hz");
            break;
        default:
            *label = 0;
    }
}

void HyeokStreamMaster::getParameterDisplay(VstInt32 index, char* text) {
    switch (index) {
        case kParamBand1Thresh:
        case kParamBand2Thresh:
        case kParamBand3Thresh:
        case kParamBand4Thresh:
            sprintf(text, "%.1f", normalizedToCompThreshDb(parameters[index]));
            break;
        case kParamBand1Makeup:
        case kParamBand2Makeup:
        case kParamBand3Makeup:
        case kParamBand4Makeup:
            sprintf(text, "%.1f", normalizedToCompMakeupDb(parameters[index]));
            break;
        case kParamXover1:
            sprintf(text, "%.0f", normalizedToFrequency(parameters[kParamXover1]));
            break;
        case kParamXover2:
            sprintf(text, "%.0f", normalizedToFrequency(parameters[kParamXover2]));
            break;
        case kParamXover3:
            sprintf(text, "%.0f", normalizedToFrequency(parameters[kParamXover3]));
            break;
        case kParamLimiterThresh:
            sprintf(text, "%.1f", normalizedToLimiterDb(parameters[kParamLimiterThresh]));
            break;
        case kParamLimiterCeiling:
            sprintf(text, "%.1f", normalizedToLimiterDb(parameters[kParamLimiterCeiling]));
            break;
        case kParamLimiterRelease:
            sprintf(text, "%.0f", normalizedToLimiterReleaseMs(parameters[kParamLimiterRelease]));
            break;
        default:
            *text = 0;
    }
}

void HyeokStreamMaster::getParameterName(VstInt32 index, char* text) {
    switch (index) {
        case kParamBand1Thresh:
            strcpy(text, "B1 Thresh");
            break;
        case kParamBand2Thresh:
            strcpy(text, "B2 Thresh");
            break;
        case kParamBand3Thresh:
            strcpy(text, "B3 Thresh");
            break;
        case kParamBand4Thresh:
            strcpy(text, "B4 Thresh");
            break;
        case kParamBand1Makeup:
            strcpy(text, "B1 Makeup");
            break;
        case kParamBand2Makeup:
            strcpy(text, "B2 Makeup");
            break;
        case kParamBand3Makeup:
            strcpy(text, "B3 Makeup");
            break;
        case kParamBand4Makeup:
            strcpy(text, "B4 Makeup");
            break;
        case kParamXover1:
            strcpy(text, "Xover1");
            break;
        case kParamXover2:
            strcpy(text, "Xover2");
            break;
        case kParamXover3:
            strcpy(text, "Xover3");
            break;
        case kParamLimiterThresh:
            strcpy(text, "Limiter Thresh");
            break;
        case kParamLimiterCeiling:
            strcpy(text, "Limiter Ceil");
            break;
        case kParamLimiterRelease:
            strcpy(text, "Limiter Rel");
            break;
        default:
            *text = 0;
    }
}

//-------------------------------------------------------------------------------------------------------
// Program
//-------------------------------------------------------------------------------------------------------
void HyeokStreamMaster::setProgramName(char* name) {
    strncpy(programName, name, kVstMaxProgNameLen);
    programName[kVstMaxProgNameLen] = 0;
}

void HyeokStreamMaster::getProgramName(char* name) {
    strcpy(name, programName);
}

//-------------------------------------------------------------------------------------------------------
// Info
//-------------------------------------------------------------------------------------------------------
bool HyeokStreamMaster::getEffectName(char* name) {
    strcpy(name, "ELC4L");
    return true;
}

bool HyeokStreamMaster::getVendorString(char* text) {
    strcpy(text, "HyeokStream Audio");
    return true;
}

bool HyeokStreamMaster::getProductString(char* text) {
    strcpy(text, "ELC4L - Elite 4-Band Compressor + Limiter");
    return true;
}

VstInt32 HyeokStreamMaster::getVendorVersion() {
    return kVersion;
}

VstInt32 HyeokStreamMaster::canDo(char* text) {
    if (strcmp(text, "receiveVstEvents") == 0) return -1;
    if (strcmp(text, "receiveVstMidiEvent") == 0) return -1;
    if (strcmp(text, "sendVstEvents") == 0) return -1;
    if (strcmp(text, "sendVstMidiEvent") == 0) return -1;
    return 0;
}

//-------------------------------------------------------------------------------------------------------
// State
//-------------------------------------------------------------------------------------------------------
void HyeokStreamMaster::setSampleRate(float sampleRate) {
    AudioEffectX::setSampleRate(sampleRate);
    dsp.setSampleRate(sampleRate);
    for (int i = 0; i < 4; ++i) {
        bandComps[i].setSampleRate(sampleRate);
    }
    limiter.setSampleRate(sampleRate);
    lufsMeter.setSampleRate(sampleRate);
}

void HyeokStreamMaster::suspend() {
    dsp.reset();
    for (int i = 0; i < 4; ++i) {
        bandComps[i].reset();
    }
    limiter.reset();
    lufsMeter.reset();
}

void HyeokStreamMaster::resume() {
    AudioEffectX::resume();
    dsp.reset();
    for (int i = 0; i < 4; ++i) {
        bandComps[i].reset();
    }
    limiter.reset();
    lufsMeter.reset();
}

//-------------------------------------------------------------------------------------------------------
// Private helpers
//-------------------------------------------------------------------------------------------------------
void HyeokStreamMaster::updateCompressors() {
    const float threshDb[4] = {
        normalizedToCompThreshDb(parameters[kParamBand1Thresh]),
        normalizedToCompThreshDb(parameters[kParamBand2Thresh]),
        normalizedToCompThreshDb(parameters[kParamBand3Thresh]),
        normalizedToCompThreshDb(parameters[kParamBand4Thresh])
    };

    const float makeupDb[4] = {
        normalizedToCompMakeupDb(parameters[kParamBand1Makeup]),
        normalizedToCompMakeupDb(parameters[kParamBand2Makeup]),
        normalizedToCompMakeupDb(parameters[kParamBand3Makeup]),
        normalizedToCompMakeupDb(parameters[kParamBand4Makeup])
    };

    for (int i = 0; i < 4; ++i) {
        bandComps[i].setThresholdDb(threshDb[i]);
        bandComps[i].setMakeupDb(makeupDb[i]);
        bandComps[i].updateCoefficients();
    }
}

void HyeokStreamMaster::updateFrequencies() {
    float x1 = normalizedToFrequency(parameters[kParamXover1]);
    float x2 = normalizedToFrequency(parameters[kParamXover2]);
    float x3 = normalizedToFrequency(parameters[kParamXover3]);

    if (x1 >= x2) x1 = x2 * 0.85f;
    if (x2 >= x3) x2 = x3 * 0.85f;

    if (x1 < kMinFreq) x1 = kMinFreq;
    if (x3 > kMaxFreq) x3 = kMaxFreq;

    dsp.setXover1(x1);
    dsp.setXover2(x2);
    dsp.setXover3(x3);
}

void HyeokStreamMaster::updateLimiter() {
    float threshDb = normalizedToLimiterDb(parameters[kParamLimiterThresh]);
    float ceilingDb = normalizedToLimiterDb(parameters[kParamLimiterCeiling]);
    float releaseMs = normalizedToLimiterReleaseMs(parameters[kParamLimiterRelease]);
    
    limiter.setThreshold(threshDb);
    limiter.setCeiling(ceilingDb);
    limiter.setRelease(releaseMs);
}

//-------------------------------------------------------------------------------------------------------
// FFT Initialization - Pre-compute Blackman-Harris window and bit-reversal table
//-------------------------------------------------------------------------------------------------------
void HyeokStreamMaster::initFFT() {
    // Blackman-Harris Window (better sidelobe rejection than Hann)
    const float a0 = 0.35875f;
    const float a1 = 0.48829f;
    const float a2 = 0.14128f;
    const float a3 = 0.01168f;
    const float pi = 3.14159265359f;
    
    for (int i = 0; i < kFftSize; ++i) {
        float t = (float)i / (float)(kFftSize - 1);
        fftWindow[i] = a0 - a1 * cosf(2.0f * pi * t) + a2 * cosf(4.0f * pi * t) - a3 * cosf(6.0f * pi * t);
        fftReal[i] = 0.0f;
        fftImag[i] = 0.0f;
    }

    // Bit Reversal Table for Cooley-Tukey FFT
    int levels = 0;
    int n = kFftSize;
    while ((1 << levels) < n) levels++;
    
    for (int i = 0; i < n; ++i) {
        int rev = 0, val = i;
        for (int j = 0; j < levels; ++j) {
            rev = (rev << 1) | (val & 1);
            val >>= 1;
        }
        fftBitReverse[i] = rev;
    }
    
    fftInitialized = true;
}

//-------------------------------------------------------------------------------------------------------
// Cooley-Tukey FFT - O(N log N) instead of O(N^2)
//-------------------------------------------------------------------------------------------------------
void HyeokStreamMaster::performFFT(float* real, float* imag, int n) {
    // Bit-reversal permutation
    for (int i = 0; i < n; ++i) {
        if (i < fftBitReverse[i]) {
            float tmpR = real[i];
            float tmpI = imag[i];
            real[i] = real[fftBitReverse[i]];
            imag[i] = imag[fftBitReverse[i]];
            real[fftBitReverse[i]] = tmpR;
            imag[fftBitReverse[i]] = tmpI;
        }
    }

    // Butterfly operations
    for (int len = 2; len <= n; len <<= 1) {
        float ang = -2.0f * 3.14159265359f / len;
        float wlenR = cosf(ang);
        float wlenI = sinf(ang);
        
        for (int i = 0; i < n; i += len) {
            float wR = 1.0f, wI = 0.0f;
            int half = len >> 1;
            
            for (int j = 0; j < half; ++j) {
                float uR = real[i + j];
                float uI = imag[i + j];
                float vR = real[i + j + half] * wR - imag[i + j + half] * wI;
                float vI = real[i + j + half] * wI + imag[i + j + half] * wR;
                
                real[i + j] = uR + vR;
                imag[i + j] = uI + vI;
                real[i + j + half] = uR - vR;
                imag[i + j + half] = uI - vI;
                
                float tmpW = wR * wlenR - wI * wlenI;
                wI = wR * wlenI + wI * wlenR;
                wR = tmpW;
            }
        }
    }
}

//-------------------------------------------------------------------------------------------------------
// Spectrum Computation - Pro-Q 3 style with tilt correction and log mapping
//-------------------------------------------------------------------------------------------------------
void HyeokStreamMaster::computeSpectrum(const float* input, float* output) {
    // 1. Apply window and copy to FFT buffers
    for (int i = 0; i < kFftSize; ++i) {
        fftReal[i] = input[i] * fftWindow[i];
        fftImag[i] = 0.0f;
    }

    // 2. Perform FFT
    performFFT(fftReal, fftImag, kFftSize);

    // 3. Log-scale bin mapping with Pink Noise tilt correction
    const float sr = (sampleRate > 0.0f) ? sampleRate : 44100.0f;
    const float invN = 2.0f / (float)kFftSize;
    const float minLogFreq = log10f(20.0f);
    const float maxLogFreq = log10f(20000.0f);
    const float logRange = maxLogFreq - minLogFreq;

    for (int bin = 0; bin < kSpectrumBins; ++bin) {
        // Map display bin to frequency (logarithmic scale for better low-end resolution)
        float t = (float)bin / (float)(kSpectrumBins - 1);
        float logFreq = minLogFreq + t * logRange;
        float freq = powf(10.0f, logFreq);
        
        // Find corresponding FFT bin
        int fftBin = (int)(freq * kFftSize / sr);
        if (fftBin < 1) fftBin = 1;
        if (fftBin >= kFftSize / 2) fftBin = kFftSize / 2 - 1;

        // [FIX] Average nearby bins' MAGNITUDE (not complex values!)
        // Summing complex values causes phase cancellation - especially bad at high frequencies
        // where spread is larger. We must compute magnitude FIRST, then average.
        float sumMag = 0.0f;
        int avgCount = 0;
        
        // Spread: narrow at low freq, wider at high freq for noise reduction
        // Reduced from 8 to 4 at high frequencies to preserve detail
        int spread = (fftBin < 30) ? 1 : (fftBin < 100 ? 2 : (fftBin < 400 ? 3 : 4));
        
        for (int k = -spread; k <= spread; ++k) {
            int idx = fftBin + k;
            if (idx >= 1 && idx < kFftSize / 2) {
                float re = fftReal[idx];
                float im = fftImag[idx];
                // [KEY FIX] Compute magnitude for EACH bin before summing
                // This preserves energy and avoids phase cancellation
                sumMag += sqrtf(re * re + im * im);
                avgCount++;
            }
        }

        // Compute average magnitude
        float mag = 0.0f;
        if (avgCount > 0) {
            mag = (sumMag / (float)avgCount) * invN;
        } else {
            // Fallback: use single bin
            float re = fftReal[fftBin];
            float im = fftImag[fftBin];
            mag = sqrtf(re * re + im * im) * invN;
        }

        // Convert to dB
        float db = 20.0f * log10f(mag + 1.0e-9f);

        // [KEY] Pink Noise Tilt Correction (+3dB/Octave)
        // Makes the spectrum look balanced like Pro-Q 3
        if (freq > 20.0f) {
            db += 3.0f * log2f(freq / 1000.0f);
        }

        // Clamp range
        if (db < -90.0f) db = -90.0f;
        if (db > 6.0f) db = 6.0f;

        // [KEY] Asymmetric smoothing (fast attack, slow release)
        // Slightly faster response at high frequencies
        float attackCoeff = (freq > 4000.0f) ? 0.50f : 0.35f;
        float releaseCoeff = (freq > 4000.0f) ? 0.88f : 0.92f;
        
        if (db > output[bin]) {
            output[bin] = (1.0f - attackCoeff) * output[bin] + attackCoeff * db;
        } else {
            output[bin] = releaseCoeff * output[bin] + (1.0f - releaseCoeff) * db;
        }
    }
}

//-------------------------------------------------------------------------------------------------------
// Update Display Buffers (Downsample 512 bins → 128 Bezier-ready points)
//-------------------------------------------------------------------------------------------------------
void HyeokStreamMaster::updateDisplayBuffers() {
    const int srcBins = kSpectrumBins;
    const int dstBins = kDisplayBins;
    const int ratio = srcBins / dstBins;  // 512/128 = 4
    
    for (int d = 0; d < dstBins; ++d) {
        // Average multiple source bins for each display bin
        float sumIn = 0.0f, sumOut = 0.0f;
        int startBin = d * ratio;
        
        for (int k = 0; k < ratio; ++k) {
            int srcIdx = startBin + k;
            if (srcIdx < srcBins) {
                sumIn += spectrumIn[srcIdx];
                sumOut += spectrumOut[srcIdx];
            }
        }
        
        float avgIn = sumIn / (float)ratio;
        float avgOut = sumOut / (float)ratio;
        
        // Additional smoothing for Bezier-ready output
        displayIn[d] = displayIn[d] * 0.6f + avgIn * 0.4f;
        displayOut[d] = displayOut[d] * 0.6f + avgOut * 0.4f;
    }
}

//-------------------------------------------------------------------------------------------------------
// Meter Update with Hop-based FFT (75% overlap for smooth updates)
//-------------------------------------------------------------------------------------------------------
void HyeokStreamMaster::updateMeters(float inL, float inR, float outL, float outR) {
    // Level meters
    float inRms = 0.5f * (inL * inL + inR * inR);
    float outRms = 0.5f * (outL * outL + outR * outR);
    float inDb = 10.0f * log10f(inRms + 1.0e-12f);
    float outDb = 10.0f * log10f(outRms + 1.0e-12f);

    inputDb = inputDb * 0.9f + inDb * 0.1f;
    outputDb = outputDb * 0.9f + outDb * 0.1f;

    // Add samples to FFT buffer
    float inMono = 0.5f * (inL + inR);
    float outMono = 0.5f * (outL + outR);

    fftBufferIn[fftWritePos] = inMono;
    fftBufferOut[fftWritePos] = outMono;
    fftWritePos++;

    // Perform FFT with hop (75% overlap for smooth updates)
    if (fftWritePos >= kFftSize) {
        computeSpectrum(fftBufferIn, spectrumIn);
        computeSpectrum(fftBufferOut, spectrumOut);
        
        // Update Bezier-ready display buffers
        updateDisplayBuffers();
        
        // Shift buffer by hop size (keep 75% of data)
        for (int i = 0; i < kFftSize - kFftHopSize; ++i) {
            fftBufferIn[i] = fftBufferIn[i + kFftHopSize];
            fftBufferOut[i] = fftBufferOut[i + kFftHopSize];
        }
        fftWritePos = kFftSize - kFftHopSize;
    }
}
