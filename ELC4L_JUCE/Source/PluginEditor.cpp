//-------------------------------------------------------------------------------------------------------
// ELC4L - Elite 4-Band Compressor + Limiter
// Plugin Editor Implementation - iZotope Ozone Style UI
//-------------------------------------------------------------------------------------------------------

#include "PluginEditor.h"

//==============================================================================
ELC4LAudioProcessorEditor::ELC4LAudioProcessorEditor(ELC4LAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    // LookAndFeel 설정
    setLookAndFeel(&customLookAndFeel);
    
    // 마우스 이벤트 버블링 활성화
    setInterceptsMouseClicks(true, true);
    
    // 헤더 바
    addAndMakeVisible(headerBar);
    
    // 스펙트럼 분석기 설정
    spectrumAnalyzer.setInterceptsMouseClicks(true, true);
    addAndMakeVisible(spectrumAnalyzer);
    
    // 크로스오버 변경 콜백
    spectrumAnalyzer.onCrossoverChanged = [this](int index, float freq) {
        juce::Slider* sliders[] = { &xover1Slider, &xover2Slider, &xover3Slider };
        if (index >= 0 && index < 3) {
            sliders[index]->setValue(freq, juce::sendNotificationSync);
        }
    };
    
    // 숨겨진 크로스오버 슬라이더 (스펙트럼과 연동)
    auto setupXoverSlider = [this](juce::Slider& slider) {
        slider.setRange(20.0, 20000.0);
        slider.setSkewFactorFromMidPoint(1000.0);
        slider.setVisible(false);
        addChildComponent(slider);
    };
    
    setupXoverSlider(xover1Slider);
    setupXoverSlider(xover2Slider);
    setupXoverSlider(xover3Slider);
    
    xover1Attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "xover1", xover1Slider);
    xover2Attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "xover2", xover2Slider);
    xover3Attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "xover3", xover3Slider);
    
    // HPF 패널
    hpfPanel.attachToParameters(audioProcessor.getAPVTS());
    hpfPanel.setInterceptsMouseClicks(true, true);
    addAndMakeVisible(hpfPanel);
    
    // 밴드 섹션 생성
    for (int i = 0; i < 4; ++i) {
        bandSections[i] = std::make_unique<ELC4L::BandSection>(i);
        bandSections[i]->attachToParameters(audioProcessor.getAPVTS());
        bandSections[i]->setInterceptsMouseClicks(true, true);
        addAndMakeVisible(*bandSections[i]);
    }
    
    setupBandCallbacks();
    
    // 리미터 섹션 설정
    limiterSection.attachToParameters(audioProcessor.getAPVTS());
    limiterSection.onBypassChanged = [this](bool state) {
        audioProcessor.setLimiterBypass(state);
    };
    limiterSection.setInterceptsMouseClicks(true, true);
    addAndMakeVisible(limiterSection);
    
    // IO 미터 패널
    ioMeters.setInterceptsMouseClicks(true, true);
    addAndMakeVisible(ioMeters);
    
    // 글로벌 정보 패널
    addAndMakeVisible(infoPanel);
    
    // 윈도우 크기 설정
    setSize(ELC4L::Layout::WindowW, ELC4L::Layout::WindowH);
    
    // 타이머 시작 (30fps 업데이트)
    startTimerHz(30);
}

ELC4LAudioProcessorEditor::~ELC4LAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

//==============================================================================
void ELC4LAudioProcessorEditor::paint(juce::Graphics& g)
{
    // 메인 배경 (Ozone 스타일 - 매우 어두운 다크)
    g.fillAll(ELC4L::Colours::bgDark);
    
    // 미묘한 비네팅 효과
    juce::ColourGradient vignette(juce::Colours::transparentBlack, getWidth() * 0.5f, getHeight() * 0.5f,
                                   juce::Colours::black.withAlpha(0.15f), 0.0f, 0.0f, true);
    g.setGradientFill(vignette);
    g.fillRect(getLocalBounds());
}

//==============================================================================
void ELC4LAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    
    // 헤더 바 (상단)
    headerBar.setBounds(bounds.removeFromTop(ELC4L::Layout::HeaderH));
    
    // 메인 영역
    bounds = bounds.reduced(ELC4L::Layout::Margin);
    
    // 상단 영역 (스펙트럼 + 미터)
    auto topArea = bounds.removeFromTop(ELC4L::Layout::SpectrumH);
    
    // 우측 미터/정보 패널 (더 넓게)
    auto rightPanel = topArea.removeFromRight(ELC4L::Layout::MeterPanelW);
    
    // IO 미터 (왼쪽)
    auto ioMeterArea = rightPanel.removeFromLeft(70);
    ioMeters.setBounds(ioMeterArea);
    
    rightPanel.removeFromLeft(6);
    
    // 정보 패널 (오른쪽)
    infoPanel.setBounds(rightPanel);
    
    // 스펙트럼 영역
    topArea.removeFromRight(8);
    spectrumAnalyzer.setBounds(topArea);
    
    // 간격
    bounds.removeFromTop(8);
    
    // HPF 패널 (스펙트럼 아래 왼쪽)
    auto hpfArea = bounds.removeFromTop(48);
    hpfPanel.setBounds(hpfArea.removeFromLeft(280));
    
    // 간격
    bounds.removeFromTop(8);
    
    // 하단 컨트롤 영역
    int bandWidth = ELC4L::Layout::BandWidth;
    int bandGap = ELC4L::Layout::BandGap;
    int bandHeight = bounds.getHeight();
    
    int startX = bounds.getX();
    
    // 4개 밴드 섹션
    for (int i = 0; i < 4; ++i) {
        bandSections[i]->setBounds(startX + i * (bandWidth + bandGap), 
                                   bounds.getY(), bandWidth, bandHeight);
    }
    
    // 리미터 섹션
    int limiterX = startX + 4 * (bandWidth + bandGap) + 12;
    int limiterWidth = bounds.getWidth() - (limiterX - startX);
    limiterSection.setBounds(limiterX, bounds.getY(), 
                             juce::jmin(limiterWidth, ELC4L::Layout::LimiterW), bandHeight);
}

//==============================================================================
void ELC4LAudioProcessorEditor::timerCallback()
{
    updateMeters();
}

//==============================================================================
void ELC4LAudioProcessorEditor::setupBandCallbacks()
{
    for (int i = 0; i < 4; ++i) {
        bandSections[i]->onMuteChanged = [this](int band, bool state) {
            audioProcessor.setBandMute(band, state);
        };
        
        bandSections[i]->onSoloChanged = [this](int band, bool state) {
            audioProcessor.setBandSolo(band, state);
        };
        
        bandSections[i]->onDeltaChanged = [this](int band, bool state) {
            audioProcessor.setBandDelta(band, state);
        };
        
        bandSections[i]->onBypassChanged = [this](int band, bool state) {
            audioProcessor.setBandBypass(band, state);
        };
    }
}

//==============================================================================
void ELC4LAudioProcessorEditor::updateMeters()
{
    // 스펙트럼 데이터 업데이트
    spectrumAnalyzer.setSpectrumData(audioProcessor.getSpectrumIn(),
                                      audioProcessor.getSpectrumOut(),
                                      ELC4LAudioProcessor::getSpectrumSize());
    
    // 크로스오버 주파수 업데이트
    spectrumAnalyzer.setCrossoverFrequencies(audioProcessor.getXover1Hz(),
                                              audioProcessor.getXover2Hz(),
                                              audioProcessor.getXover3Hz());
    
    // 밴드별 GR 미터 업데이트
    float totalGr = 0.0f;
    for (int i = 0; i < 4; ++i) {
        float bandGr = audioProcessor.getBandGrDb(i);
        bandSections[i]->setGainReduction(bandGr);
        totalGr = juce::jmax(totalGr, bandGr);
        
        // 버튼 상태 동기화
        bandSections[i]->setMuted(audioProcessor.getBandMute(i));
        bandSections[i]->setSoloed(audioProcessor.getBandSolo(i));
        bandSections[i]->setDelta(audioProcessor.getBandDelta(i));
        bandSections[i]->setBypassed(audioProcessor.getBandBypass(i));
    }
    
    // 리미터 GR 포함
    float limiterGr = audioProcessor.getLimiterGrDb();
    totalGr = juce::jmax(totalGr, limiterGr);
    
    // 리미터 GR 미터 업데이트
    limiterSection.setGainReduction(limiterGr);
    limiterSection.setBypassed(audioProcessor.getLimiterBypass());
    
    // IO 미터 업데이트
    ioMeters.setInputLevel(audioProcessor.getInputDb());
    ioMeters.setOutputLevel(audioProcessor.getOutputDb());
    ioMeters.setLufsLevel(audioProcessor.getLufsMomentary());
    
    // 글로벌 정보 패널 업데이트
    infoPanel.setValues(audioProcessor.getLufsMomentary(),
                        audioProcessor.getOutputDb(),
                        totalGr);
}
