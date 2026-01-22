//-------------------------------------------------------------------------------------------------------
// ELC4L - Elite 4-Band Compressor + Limiter
// Plugin Editor Header - iZotope Ozone Style UI
//-------------------------------------------------------------------------------------------------------

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "UI/CustomLookAndFeel.h"
#include "UI/SpectrumAnalyzer.h"
#include "UI/Meters.h"
#include "UI/BandSection.h"
#include "UI/LimiterSection.h"

//==============================================================================
// Ozone 스타일 헤더 바
//==============================================================================
class HeaderBar : public juce::Component
{
public:
    HeaderBar() {
        // 로고/타이틀
        titleLabel.setText("ELC4L", juce::dontSendNotification);
        titleLabel.setFont(juce::Font(juce::FontOptions().withHeight(16.0f).withStyle("Bold")));
        titleLabel.setColour(juce::Label::textColourId, ELC4L::Colours::accentCyan);
        addAndMakeVisible(titleLabel);
        
        // 서브타이틀
        subtitleLabel.setText("Multiband Dynamics", juce::dontSendNotification);
        subtitleLabel.setFont(juce::Font(juce::FontOptions().withHeight(10.0f)));
        subtitleLabel.setColour(juce::Label::textColourId, ELC4L::Colours::textDim);
        addAndMakeVisible(subtitleLabel);
        
        // 버전
        versionLabel.setText("v2.0.0", juce::dontSendNotification);
        versionLabel.setFont(juce::Font(juce::FontOptions().withHeight(9.0f)));
        versionLabel.setColour(juce::Label::textColourId, ELC4L::Colours::textDim);
        addAndMakeVisible(versionLabel);
    }
    
    void paint(juce::Graphics& g) override {
        // 배경
        g.setColour(ELC4L::Colours::bgHeader);
        g.fillRect(getLocalBounds());
        
        // 하단 라인
        g.setColour(ELC4L::Colours::border);
        g.fillRect(0, getHeight() - 1, getWidth(), 1);
        
        // 상단 악센트 라인 (시안)
        g.setColour(ELC4L::Colours::accentCyan);
        g.fillRect(0, 0, getWidth(), 2);
    }
    
    void resized() override {
        auto bounds = getLocalBounds().reduced(12, 6);
        titleLabel.setBounds(bounds.removeFromLeft(55));
        bounds.removeFromLeft(5);
        subtitleLabel.setBounds(bounds.removeFromLeft(140));
        versionLabel.setBounds(bounds.removeFromRight(50));
    }
    
private:
    juce::Label titleLabel, subtitleLabel, versionLabel;
};

//==============================================================================
// HPF 컨트롤 패널 (Ozone 스타일)
//==============================================================================
class HPFPanel : public juce::Component
{
public:
    HPFPanel() {
        // HPF 활성화 버튼
        hpfButton.setButtonText("HPF");
        hpfButton.setClickingTogglesState(true);
        hpfButton.setColour(juce::TextButton::buttonColourId, ELC4L::Colours::btnInactive);
        hpfButton.setColour(juce::TextButton::buttonOnColourId, ELC4L::Colours::accentCyan);
        addAndMakeVisible(hpfButton);
        
        // HPF 주파수 슬라이더
        hpfSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        hpfSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 55, 20);
        hpfSlider.setRange(20.0, 500.0, 1.0);
        hpfSlider.setValue(80.0);
        hpfSlider.setTextValueSuffix(" Hz");
        hpfSlider.setColour(juce::Slider::textBoxTextColourId, ELC4L::Colours::textValue);
        hpfSlider.setColour(juce::Slider::textBoxBackgroundColourId, ELC4L::Colours::bgInput);
        hpfSlider.setColour(juce::Slider::textBoxOutlineColourId, ELC4L::Colours::border);
        addAndMakeVisible(hpfSlider);
        
        // 레이블
        hpfLabel.setText("Sidechain HPF", juce::dontSendNotification);
        hpfLabel.setFont(juce::Font(juce::FontOptions().withHeight(9.5f)));
        hpfLabel.setColour(juce::Label::textColourId, ELC4L::Colours::textDim);
        addAndMakeVisible(hpfLabel);
    }
    
    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat();
        g.setColour(ELC4L::Colours::bgModule);
        g.fillRoundedRectangle(bounds, 3.0f);
        g.setColour(ELC4L::Colours::border);
        g.drawRoundedRectangle(bounds.reduced(0.5f), 3.0f, 1.0f);
    }
    
    void resized() override {
        auto bounds = getLocalBounds().reduced(10, 6);
        auto topRow = bounds.removeFromTop(14);
        hpfLabel.setBounds(topRow);
        
        bounds.removeFromTop(4);
        auto row = bounds.removeFromTop(24);
        hpfButton.setBounds(row.removeFromLeft(42));
        row.removeFromLeft(8);
        hpfSlider.setBounds(row);
    }
    
    void attachToParameters(juce::AudioProcessorValueTreeState& apvts) {
        hpfBtnAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            apvts, "sidechainActive", hpfButton);
        hpfSliderAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts, "sidechainFreq", hpfSlider);
    }
    
private:
    juce::TextButton hpfButton;
    juce::Slider hpfSlider;
    juce::Label hpfLabel;
    
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> hpfBtnAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> hpfSliderAttachment;
};

//==============================================================================
// 글로벌 수치 표시 패널 (Ozone 스타일 - 오른쪽 영역)
//==============================================================================
class GlobalInfoPanel : public juce::Component, public juce::Timer
{
public:
    GlobalInfoPanel() {
        startTimerHz(20);
    }
    
    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds();
        
        // 배경
        g.setColour(ELC4L::Colours::bgModule);
        g.fillRoundedRectangle(bounds.toFloat(), 3.0f);
        g.setColour(ELC4L::Colours::border);
        g.drawRoundedRectangle(bounds.toFloat().reduced(0.5f), 3.0f, 1.0f);
        
        bounds = bounds.reduced(10, 8);
        
        // LUFS 표시 (큰 숫자)
        auto lufsSection = bounds.removeFromTop(50);
        g.setColour(ELC4L::Colours::textDim);
        g.setFont(juce::Font(juce::FontOptions().withHeight(9.0f)));
        g.drawText("INTEGRATED LUFS", lufsSection.removeFromTop(12), juce::Justification::centredLeft, false);
        
        g.setColour(ELC4L::Colours::textValue);
        g.setFont(juce::Font(juce::FontOptions().withHeight(26.0f).withStyle("Bold")));
        g.drawText(juce::String(lufsValue, 1), lufsSection, juce::Justification::centred, false);
        
        bounds.removeFromTop(6);
        
        // Peak 표시
        auto peakSection = bounds.removeFromTop(38);
        g.setColour(ELC4L::Colours::textDim);
        g.setFont(juce::Font(juce::FontOptions().withHeight(9.0f)));
        g.drawText("PEAK", peakSection.removeFromTop(12), juce::Justification::centredLeft, false);
        
        juce::Colour peakColour = peakValue > -0.5f ? ELC4L::Colours::clipRed : 
                                  (peakValue > -3.0f ? ELC4L::Colours::meterOrange : ELC4L::Colours::textValue);
        g.setColour(peakColour);
        g.setFont(juce::Font(juce::FontOptions().withHeight(18.0f).withStyle("Bold")));
        g.drawText(juce::String(peakValue, 1) + " dB", peakSection, juce::Justification::centred, false);
        
        bounds.removeFromTop(6);
        
        // Total GR 표시
        auto grSection = bounds.removeFromTop(38);
        g.setColour(ELC4L::Colours::textDim);
        g.setFont(juce::Font(juce::FontOptions().withHeight(9.0f)));
        g.drawText("GAIN REDUCTION", grSection.removeFromTop(12), juce::Justification::centredLeft, false);
        
        g.setColour(ELC4L::Colours::accentCyan);
        g.setFont(juce::Font(juce::FontOptions().withHeight(18.0f).withStyle("Bold")));
        g.drawText("-" + juce::String(grValue, 1) + " dB", grSection, juce::Justification::centred, false);
    }
    
    void timerCallback() override {
        repaint();
    }
    
    void setValues(float lufs, float peak, float gr) {
        lufsValue = lufs;
        peakValue = peak;
        grValue = gr;
    }
    
private:
    float lufsValue = -24.0f;
    float peakValue = -6.0f;
    float grValue = 0.0f;
};

//==============================================================================
// 메인 에디터 클래스
//==============================================================================
class ELC4LAudioProcessorEditor : public juce::AudioProcessorEditor,
                                   private juce::Timer
{
public:
    ELC4LAudioProcessorEditor(ELC4LAudioProcessor&);
    ~ELC4LAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    
    // 마우스 이벤트가 올바른 컴포넌트로 전달되도록 보장
    bool hitTest(int x, int y) override { return true; }

private:
    void timerCallback() override;
    void setupBandCallbacks();
    void updateMeters();
    
    ELC4LAudioProcessor& audioProcessor;
    
    // LookAndFeel
    ELC4L::CustomLookAndFeel customLookAndFeel;
    
    // 헤더
    HeaderBar headerBar;
    
    // 스펙트럼 분석기
    ELC4L::SpectrumAnalyzer spectrumAnalyzer;
    
    // 크로스오버 슬라이더 (숨김)
    juce::Slider xover1Slider, xover2Slider, xover3Slider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> xover1Attachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> xover2Attachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> xover3Attachment;
    
    // HPF 패널
    HPFPanel hpfPanel;
    
    // 밴드 섹션
    std::unique_ptr<ELC4L::BandSection> bandSections[4];
    
    // 리미터 섹션
    ELC4L::LimiterSection limiterSection;
    
    // IO 미터 패널
    ELC4L::IOMetersPanel ioMeters;
    
    // 글로벌 정보 패널
    GlobalInfoPanel infoPanel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ELC4LAudioProcessorEditor)
};
