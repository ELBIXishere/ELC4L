//-------------------------------------------------------------------------------------------------------
// ELC4L - Elite 4-Band Compressor + Limiter
// Limiter Section Implementation - Ozone Style
//-------------------------------------------------------------------------------------------------------

#include "LimiterSection.h"

namespace ELC4L {

//==============================================================================
LimiterSection::LimiterSection()
{
    // GR 미터 설정
    grMeter.setColour(Colours::limiter);
    addAndMakeVisible(grMeter);
    
    // 슬라이더 설정 - Threshold (노브)
    thresholdSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    thresholdSlider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    thresholdSlider.setColour(juce::Slider::rotarySliderFillColourId, Colours::limiter);
    thresholdSlider.setColour(juce::Slider::rotarySliderOutlineColourId, Colours::knobFill);
    thresholdSlider.setColour(juce::Slider::thumbColourId, Colours::knobPointer);
    thresholdSlider.addListener(this);
    addAndMakeVisible(thresholdSlider);
    
    // 슬라이더 설정 - Ceiling (노브)
    ceilingSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    ceilingSlider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    ceilingSlider.setColour(juce::Slider::rotarySliderFillColourId, Colours::limiter);
    ceilingSlider.setColour(juce::Slider::rotarySliderOutlineColourId, Colours::knobFill);
    ceilingSlider.setColour(juce::Slider::thumbColourId, Colours::knobPointer);
    ceilingSlider.addListener(this);
    addAndMakeVisible(ceilingSlider);
    
    // 슬라이더 설정 - Release (노브)
    releaseSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    releaseSlider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    releaseSlider.setColour(juce::Slider::rotarySliderFillColourId, Colours::accentCyan);
    releaseSlider.setColour(juce::Slider::rotarySliderOutlineColourId, Colours::knobFill);
    releaseSlider.setColour(juce::Slider::thumbColourId, Colours::knobPointer);
    releaseSlider.addListener(this);
    addAndMakeVisible(releaseSlider);
    
    // 레이블 설정
    for (auto* label : { &thresholdLabel, &ceilingLabel, &releaseLabel }) {
        label->setFont(juce::Font(juce::FontOptions().withHeight(9.0f)));
        label->setColour(juce::Label::textColourId, Colours::textDim);
        label->setJustificationType(juce::Justification::centred);
        addAndMakeVisible(label);
    }
    
    // 수치 표시 추가
    addAndMakeVisible(thresholdValue);
    addAndMakeVisible(ceilingValue);
    addAndMakeVisible(releaseValue);
    addAndMakeVisible(grValue);
    
    // Bypass 버튼
    bypassButton.setClickingTogglesState(true);
    bypassButton.setColour(juce::TextButton::buttonColourId, Colours::limiter.withAlpha(0.6f));
    bypassButton.setColour(juce::TextButton::buttonOnColourId, Colours::btnBypass);
    bypassButton.setColour(juce::TextButton::textColourOffId, Colours::textBright);
    bypassButton.setColour(juce::TextButton::textColourOnId, Colours::textDim);
    bypassButton.onClick = [this]() {
        bypassButton.setButtonText(bypassButton.getToggleState() ? "LIMITER OFF" : "LIMITER ON");
        if (onBypassChanged) onBypassChanged(bypassButton.getToggleState());
    };
    addAndMakeVisible(bypassButton);
    
    // True Peak 토글
    truePeakButton.setColour(juce::ToggleButton::textColourId, Colours::textDim);
    truePeakButton.setColour(juce::ToggleButton::tickColourId, Colours::accentCyan);
    truePeakButton.setToggleState(true, juce::dontSendNotification);
    addAndMakeVisible(truePeakButton);
}

//==============================================================================
void LimiterSection::sliderValueChanged(juce::Slider* slider)
{
    if (slider == &thresholdSlider) {
        thresholdValue.setValue((float)thresholdSlider.getValue());
    }
    else if (slider == &ceilingSlider) {
        ceilingValue.setValue((float)ceilingSlider.getValue());
    }
    else if (slider == &releaseSlider) {
        releaseValue.setValue((float)releaseSlider.getValue());
    }
}

//==============================================================================
void LimiterSection::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    
    // 메인 배경 (모듈 스타일)
    g.setColour(Colours::bgModule);
    g.fillRoundedRectangle(bounds.toFloat(), 4.0f);
    
    // 상단 리미터 색상 악센트
    auto accentLine = bounds.removeFromTop(3);
    g.setColour(Colours::limiter);
    g.fillRoundedRectangle(accentLine.reduced(1, 0).toFloat(), 1.5f);
    
    // 타이틀 영역
    auto titleArea = bounds.removeFromTop(24);
    g.setColour(Colours::limiter);
    g.setFont(juce::Font(juce::FontOptions().withHeight(12.0f).withStyle("Bold")));
    g.drawText("LIMITER", titleArea, juce::Justification::centred, false);
    
    // 테두리
    g.setColour(Colours::border);
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 4.0f, 1.0f);
}

//==============================================================================
void LimiterSection::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop(27);  // 타이틀 영역
    bounds = bounds.reduced(8, 4);
    
    // 상단 영역: GR 미터 + 수치
    auto topArea = bounds.removeFromTop(70);
    
    // GR 미터 (중앙)
    auto grArea = topArea.withSizeKeepingCentre(30, 50);
    grMeter.setBounds(grArea);
    
    // GR 수치 (미터 아래)
    grValue.setBounds(topArea.removeFromBottom(16).withSizeKeepingCentre(60, 16));
    
    bounds.removeFromTop(5);
    
    // 노브 영역 - 2행으로 배치
    int knobSize = 48;
    
    // 첫 번째 행: Threshold, Ceiling
    auto row1 = bounds.removeFromTop(75);
    int spacing = (row1.getWidth() - knobSize * 2) / 3;
    
    // Threshold
    auto thrArea = row1.removeFromLeft(spacing + knobSize);
    thrArea.removeFromLeft(spacing / 2);
    thresholdSlider.setBounds(thrArea.removeFromTop(knobSize).withSizeKeepingCentre(knobSize, knobSize));
    thrArea.removeFromTop(2);
    thresholdLabel.setBounds(thrArea.removeFromTop(12));
    thresholdValue.setBounds(thrArea.removeFromTop(14).withSizeKeepingCentre(55, 14));
    
    // Ceiling
    auto ceilArea = row1.removeFromLeft(spacing + knobSize);
    ceilingSlider.setBounds(ceilArea.removeFromTop(knobSize).withSizeKeepingCentre(knobSize, knobSize));
    ceilArea.removeFromTop(2);
    ceilingLabel.setBounds(ceilArea.removeFromTop(12));
    ceilingValue.setBounds(ceilArea.removeFromTop(14).withSizeKeepingCentre(55, 14));
    
    bounds.removeFromTop(5);
    
    // 두 번째 행: Release (중앙)
    auto row2 = bounds.removeFromTop(75);
    auto relArea = row2.withSizeKeepingCentre(knobSize + 30, 75);
    releaseSlider.setBounds(relArea.removeFromTop(knobSize).withSizeKeepingCentre(knobSize, knobSize));
    relArea.removeFromTop(2);
    releaseLabel.setBounds(relArea.removeFromTop(12));
    releaseValue.setBounds(relArea.removeFromTop(14).withSizeKeepingCentre(55, 14));
    
    bounds.removeFromTop(8);
    
    // True Peak 토글
    truePeakButton.setBounds(bounds.removeFromTop(20).withSizeKeepingCentre(100, 20));
    
    bounds.removeFromTop(5);
    
    // Bypass 버튼 (하단)
    bypassButton.setBounds(bounds.removeFromTop(28).reduced(10, 0));
}

//==============================================================================
void LimiterSection::attachToParameters(juce::AudioProcessorValueTreeState& apvts)
{
    thresholdAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, "limiterThresh", thresholdSlider);
    
    ceilingAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, "limiterCeiling", ceilingSlider);
    
    releaseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, "limiterRelease", releaseSlider);
    
    // 초기 값 업데이트
    thresholdValue.setValue((float)thresholdSlider.getValue());
    ceilingValue.setValue((float)ceilingSlider.getValue());
    releaseValue.setValue((float)releaseSlider.getValue());
}

//==============================================================================
void LimiterSection::setGainReduction(float grDb)
{
    grMeter.setValue(grDb);
    grValue.setValue(grDb);
}

} // namespace ELC4L
