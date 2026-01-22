//-------------------------------------------------------------------------------------------------------
// ELC4L - Elite 4-Band Compressor + Limiter
// Plugin Processor Implementation
//-------------------------------------------------------------------------------------------------------

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
ELC4LAudioProcessor::ELC4LAudioProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMETERS", createParameterLayout())
{
    // 파라미터 리스너 등록
    apvts.addParameterListener("band1Thresh", this);
    apvts.addParameterListener("band2Thresh", this);
    apvts.addParameterListener("band3Thresh", this);
    apvts.addParameterListener("band4Thresh", this);
    apvts.addParameterListener("band1Makeup", this);
    apvts.addParameterListener("band2Makeup", this);
    apvts.addParameterListener("band3Makeup", this);
    apvts.addParameterListener("band4Makeup", this);
    apvts.addParameterListener("xover1", this);
    apvts.addParameterListener("xover2", this);
    apvts.addParameterListener("xover3", this);
    apvts.addParameterListener("limiterThresh", this);
    apvts.addParameterListener("limiterCeiling", this);
    apvts.addParameterListener("limiterRelease", this);
    apvts.addParameterListener("sidechainFreq", this);
    apvts.addParameterListener("sidechainActive", this);

    // 원자적 변수 초기화
    for (int i = 0; i < 4; ++i) {
        bandGrDb[i].store(0.0f);
    }

    // 스펙트럼 초기화
    for (int i = 0; i < ELC4L::kDisplayBins; ++i) {
        spectrumIn[i] = -90.0f;
        spectrumOut[i] = -90.0f;
    }
}

ELC4LAudioProcessor::~ELC4LAudioProcessor()
{
    apvts.removeParameterListener("band1Thresh", this);
    apvts.removeParameterListener("band2Thresh", this);
    apvts.removeParameterListener("band3Thresh", this);
    apvts.removeParameterListener("band4Thresh", this);
    apvts.removeParameterListener("band1Makeup", this);
    apvts.removeParameterListener("band2Makeup", this);
    apvts.removeParameterListener("band3Makeup", this);
    apvts.removeParameterListener("band4Makeup", this);
    apvts.removeParameterListener("xover1", this);
    apvts.removeParameterListener("xover2", this);
    apvts.removeParameterListener("xover3", this);
    apvts.removeParameterListener("limiterThresh", this);
    apvts.removeParameterListener("limiterCeiling", this);
    apvts.removeParameterListener("limiterRelease", this);
    apvts.removeParameterListener("sidechainFreq", this);
    apvts.removeParameterListener("sidechainActive", this);
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout ELC4LAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // 밴드 컴프레서 파라미터
    for (int i = 1; i <= 4; ++i) {
        juce::String bandStr = juce::String(i);
        
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID("band" + bandStr + "Thresh", 1),
            "Band " + bandStr + " Threshold",
            juce::NormalisableRange<float>(-36.0f, 0.0f, 0.1f),
            -9.0f,
            juce::AudioParameterFloatAttributes().withLabel("dB")));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID("band" + bandStr + "Makeup", 1),
            "Band " + bandStr + " Makeup",
            juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f),
            0.0f,
            juce::AudioParameterFloatAttributes().withLabel("dB")));
    }

    // 크로스오버 주파수
    auto freqRange = juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f);
    freqRange.setSkewForCentre(1000.0f);

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("xover1", 1),
        "Crossover 1",
        freqRange, 120.0f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("xover2", 1),
        "Crossover 2",
        freqRange, 800.0f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("xover3", 1),
        "Crossover 3",
        freqRange, 4000.0f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")));

    // 리미터 파라미터
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("limiterThresh", 1),
        "Limiter Threshold",
        juce::NormalisableRange<float>(-24.0f, 0.0f, 0.1f),
        -6.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("limiterCeiling", 1),
        "Limiter Ceiling",
        juce::NormalisableRange<float>(-24.0f, 0.0f, 0.1f),
        -1.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("limiterRelease", 1),
        "Limiter Release",
        juce::NormalisableRange<float>(10.0f, 500.0f, 1.0f),
        120.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));

    // 사이드체인 HPF
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("sidechainFreq", 1),
        "Sidechain HPF",
        freqRange, 80.0f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("sidechainActive", 1),
        "Sidechain Active",
        false));

    return { params.begin(), params.end() };
}

//==============================================================================
void ELC4LAudioProcessor::parameterChanged(const juce::String& parameterID, float /*newValue*/)
{
    if (parameterID.contains("Thresh") || parameterID.contains("Makeup") || 
        parameterID.contains("sidechain")) {
        updateCompressors();
    }
    else if (parameterID.contains("xover")) {
        updateFrequencies();
    }
    else if (parameterID.contains("limiter")) {
        updateLimiter();
    }
}

//==============================================================================
void ELC4LAudioProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/)
{
    float sr = static_cast<float>(sampleRate);
    
    crossover.setSampleRate(sr);
    for (int i = 0; i < 4; ++i) {
        bandComps[i].setSampleRate(sr);
    }
    limiter.setSampleRate(sr);
    lufsMeter.setSampleRate(sr);

    updateCompressors();
    updateFrequencies();
    updateLimiter();

    // 레이턴시 설정
    setLatencySamples(ELC4L::LookaheadLimiter::kLookaheadSamples);
}

void ELC4LAudioProcessor::releaseResources()
{
    crossover.reset();
    for (int i = 0; i < 4; ++i) {
        bandComps[i].reset();
    }
    limiter.reset();
    lufsMeter.reset();
}

bool ELC4LAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // 스테레오만 지원
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    return true;
}

//==============================================================================
void ELC4LAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    auto* inL = buffer.getReadPointer(0);
    auto* inR = buffer.getReadPointer(1);
    auto* outL = buffer.getWritePointer(0);
    auto* outR = buffer.getWritePointer(1);
    
    int numSamples = buffer.getNumSamples();

    // 솔로 체크
    bool anySolo = bandSolo[0] || bandSolo[1] || bandSolo[2] || bandSolo[3];

    // 메이크업 게인 미리 계산
    float makeupGains[4];
    for (int b = 0; b < 4; ++b) {
        float makeupDb = *apvts.getRawParameterValue("band" + juce::String(b + 1) + "Makeup");
        makeupGains[b] = ELC4L::dbToLinear(makeupDb);
    }

    for (int i = 0; i < numSamples; ++i) {
        float b1L, b1R, b2L, b2R, b3L, b3R, b4L, b4R;

        // 1. 4밴드로 분리
        crossover.processSample(inL[i], inR[i], b1L, b1R, b2L, b2R, b3L, b3R, b4L, b4R);

        // 델타용 원본 저장
        float bandInputL[4] = { b1L, b2L, b3L, b4L };
        float bandInputR[4] = { b1R, b2R, b3R, b4R };

        // 2. 밴드별 컴프레서 처리
        float* bands[4][2] = { {&b1L, &b1R}, {&b2L, &b2R}, {&b3L, &b3R}, {&b4L, &b4R} };
        
        for (int b = 0; b < 4; ++b) {
            if (!bandBypass[b]) {
                bandComps[b].process(*bands[b][0], *bands[b][1]);
            } else {
                *bands[b][0] *= makeupGains[b];
                *bands[b][1] *= makeupGains[b];
            }
        }
        
        float bandOutL[4] = { b1L, b2L, b3L, b4L };
        float bandOutR[4] = { b1R, b2R, b3R, b4R };

        // 3. 델타/뮤트/솔로 로직
        float mixL = 0.0f;
        float mixR = 0.0f;
        
        for (int b = 0; b < 4; ++b) {
            float outBandL, outBandR;
            
            if (bandDelta[b]) {
                // 델타: 컴프레서가 줄인 양만 재생
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
            
            // 솔로/뮤트
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

        // 4. 리미터 적용
        if (!limiterBypass) {
            limiter.process(mixL, mixR);
        }
        lufsMeter.process(mixL, mixR);

        outL[i] = mixL;
        outR[i] = mixR;

        // 스펙트럼 분석기 버퍼 채우기
        float inMono = 0.5f * (inL[i] + inR[i]);
        float outMono = 0.5f * (mixL + mixR);
        fftBufferIn[fftWritePos] = inMono;
        fftBufferOut[fftWritePos] = outMono;
        fftWritePos++;

        if (fftWritePos >= ELC4L::kFftSize) {
            computeSpectrum(fftBufferIn, spectrumIn);
            computeSpectrum(fftBufferOut, spectrumOut);
            
            // 버퍼 시프트 (75% 오버랩)
            for (int j = 0; j < ELC4L::kFftSize - ELC4L::kFftHopSize; ++j) {
                fftBufferIn[j] = fftBufferIn[j + ELC4L::kFftHopSize];
                fftBufferOut[j] = fftBufferOut[j + ELC4L::kFftHopSize];
            }
            fftWritePos = ELC4L::kFftSize - ELC4L::kFftHopSize;
        }
    }

    // 미터 업데이트
    float inRms = 0.0f, outRms = 0.0f;
    for (int i = 0; i < numSamples; ++i) {
        inRms += inL[i] * inL[i] + inR[i] * inR[i];
        outRms += outL[i] * outL[i] + outR[i] * outR[i];
    }
    inRms = std::sqrt(inRms / (2.0f * numSamples));
    outRms = std::sqrt(outRms / (2.0f * numSamples));

    inputDb.store(20.0f * std::log10(inRms + 1.0e-12f));
    outputDb.store(20.0f * std::log10(outRms + 1.0e-12f));

    for (int b = 0; b < 4; ++b) {
        bandGrDb[b].store(bandComps[b].getGainReductionDb());
    }
    limiterGrDb.store(limiter.getGainReductionDb());
    lufsMomentary.store(lufsMeter.getMomentary());
}

//==============================================================================
void ELC4LAudioProcessor::computeSpectrum(const float* input, float* output)
{
    // FFT 버퍼 준비
    std::array<float, ELC4L::kFftSize * 2> fftData;
    
    // 윈도우 적용
    for (int i = 0; i < ELC4L::kFftSize; ++i) {
        fftData[i] = input[i];
    }
    window.multiplyWithWindowingTable(fftData.data(), ELC4L::kFftSize);
    
    // 나머지 0으로 채우기
    for (int i = ELC4L::kFftSize; i < ELC4L::kFftSize * 2; ++i) {
        fftData[i] = 0.0f;
    }

    // FFT 수행
    fft.performFrequencyOnlyForwardTransform(fftData.data());

    // 로그 스케일 빈 매핑 + 핑크 노이즈 틸트 보정
    float sr = static_cast<float>(getSampleRate());
    if (sr <= 0.0f) sr = 44100.0f;
    
    const float minLogFreq = std::log10(20.0f);
    const float maxLogFreq = std::log10(20000.0f);
    const float logRange = maxLogFreq - minLogFreq;

    for (int bin = 0; bin < ELC4L::kDisplayBins; ++bin) {
        float t = static_cast<float>(bin) / static_cast<float>(ELC4L::kDisplayBins - 1);
        float logFreq = minLogFreq + t * logRange;
        float freq = std::pow(10.0f, logFreq);
        
        int fftBin = static_cast<int>(freq * ELC4L::kFftSize / sr);
        fftBin = juce::jlimit(1, ELC4L::kFftSize / 2 - 1, fftBin);

        // 주변 빈 평균
        float sumMag = 0.0f;
        int avgCount = 0;
        int spread = (fftBin < 30) ? 1 : (fftBin < 100 ? 2 : (fftBin < 400 ? 3 : 4));
        
        for (int k = -spread; k <= spread; ++k) {
            int idx = fftBin + k;
            if (idx >= 1 && idx < ELC4L::kFftSize / 2) {
                sumMag += fftData[idx];
                avgCount++;
            }
        }

        float mag = (avgCount > 0) ? (sumMag / avgCount) : fftData[fftBin];
        float db = 20.0f * std::log10(mag + 1.0e-9f);

        // 핑크 노이즈 틸트 보정 (+3dB/Octave)
        if (freq > 20.0f) {
            db += 3.0f * std::log2(freq / 1000.0f);
        }

        db = juce::jlimit(-90.0f, 6.0f, db);

        // 비대칭 스무딩
        float attackCoeff = (freq > 4000.0f) ? 0.50f : 0.35f;
        float releaseCoeff = (freq > 4000.0f) ? 0.88f : 0.92f;
        
        if (db > output[bin]) {
            output[bin] = (1.0f - attackCoeff) * output[bin] + attackCoeff * db;
        } else {
            output[bin] = releaseCoeff * output[bin] + (1.0f - releaseCoeff) * db;
        }
    }
}

//==============================================================================
void ELC4LAudioProcessor::updateCompressors()
{
    for (int i = 0; i < 4; ++i) {
        float threshDb = *apvts.getRawParameterValue("band" + juce::String(i + 1) + "Thresh");
        float makeupDb = *apvts.getRawParameterValue("band" + juce::String(i + 1) + "Makeup");
        bandComps[i].setThresholdDb(threshDb);
        bandComps[i].setMakeupDb(makeupDb);
        bandComps[i].updateCoefficients();
    }

    bool scActive = *apvts.getRawParameterValue("sidechainActive") > 0.5f;
    float scFreq = *apvts.getRawParameterValue("sidechainFreq");
    
    for (int i = 0; i < 4; ++i) {
        bandComps[i].setSidechainEnabled(scActive);
        bandComps[i].setSidechainFreq(scFreq);
    }
}

void ELC4LAudioProcessor::updateFrequencies()
{
    float x1 = *apvts.getRawParameterValue("xover1");
    float x2 = *apvts.getRawParameterValue("xover2");
    float x3 = *apvts.getRawParameterValue("xover3");

    // 크로스오버 유효성 검사
    if (x1 >= x2) x1 = x2 * 0.85f;
    if (x2 >= x3) x2 = x3 * 0.85f;

    x1 = juce::jlimit(ELC4L::kMinFreq, ELC4L::kMaxFreq, x1);
    x3 = juce::jlimit(ELC4L::kMinFreq, ELC4L::kMaxFreq, x3);

    crossover.setXover1(x1);
    crossover.setXover2(x2);
    crossover.setXover3(x3);
}

void ELC4LAudioProcessor::updateLimiter()
{
    float threshDb = *apvts.getRawParameterValue("limiterThresh");
    float ceilingDb = *apvts.getRawParameterValue("limiterCeiling");
    float releaseMs = *apvts.getRawParameterValue("limiterRelease");
    
    limiter.setThreshold(threshDb);
    limiter.setCeiling(ceilingDb);
    limiter.setRelease(releaseMs);
}

//==============================================================================
juce::AudioProcessorEditor* ELC4LAudioProcessor::createEditor()
{
    return new ELC4LAudioProcessorEditor(*this);
}

bool ELC4LAudioProcessor::hasEditor() const { return true; }

const juce::String ELC4LAudioProcessor::getName() const { return JucePlugin_Name; }
bool ELC4LAudioProcessor::acceptsMidi() const { return false; }
bool ELC4LAudioProcessor::producesMidi() const { return false; }
bool ELC4LAudioProcessor::isMidiEffect() const { return false; }
double ELC4LAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int ELC4LAudioProcessor::getNumPrograms() { return 1; }
int ELC4LAudioProcessor::getCurrentProgram() { return 0; }
void ELC4LAudioProcessor::setCurrentProgram(int) {}
const juce::String ELC4LAudioProcessor::getProgramName(int) { return {}; }
void ELC4LAudioProcessor::changeProgramName(int, const juce::String&) {}

//==============================================================================
void ELC4LAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void ELC4LAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr) {
        if (xmlState->hasTagName(apvts.state.getType())) {
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
        }
    }
}

//==============================================================================
// 플러그인 인스턴스 생성
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ELC4LAudioProcessor();
}
