//-------------------------------------------------------------------------------------------------------
// ELC4L - Elite 4-Band Compressor + Limiter
// Band Section Implementation - Ozone Style
//-------------------------------------------------------------------------------------------------------

#include "BandSection.h"

namespace ELC4L {

//==============================================================================
BandSection::BandSection(int index)
    : bandIndex(index)
{
    // 밴드 이름 및 색상 설정
    const char* names[] = { "LOW", "LO-MID", "HI-MID", "HIGH" };
    bandName = names[juce::jlimit(0, 3, bandIndex)];
    bandColour = Colours::getBandColour(bandIndex);
    
    // GR 미터 설정
    grMeter.setColour(Colours::grMeterFill);
    addAndMakeVisible(grMeter);
    
    // 슬라이더 설정 - Threshold (노브 스타일)
    thresholdSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    thresholdSlider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    thresholdSlider.setColour(juce::Slider::rotarySliderFillColourId, bandColour);
    thresholdSlider.setColour(juce::Slider::rotarySliderOutlineColourId, Colours::knobFill);
    thresholdSlider.setColour(juce::Slider::thumbColourId, Colours::knobPointer);
    thresholdSlider.addListener(this);
    addAndMakeVisible(thresholdSlider);
    
    // 슬라이더 설정 - Makeup (노브 스타일)
    makeupSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    makeupSlider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    makeupSlider.setColour(juce::Slider::rotarySliderFillColourId, bandColour);
    makeupSlider.setColour(juce::Slider::rotarySliderOutlineColourId, Colours::knobFill);
    makeupSlider.setColour(juce::Slider::thumbColourId, Colours::knobPointer);
    makeupSlider.addListener(this);
    addAndMakeVisible(makeupSlider);
    
    // 레이블 설정
    for (auto* label : { &thresholdLabel, &makeupLabel }) {
        label->setFont(juce::Font(juce::FontOptions().withHeight(9.0f)));
        label->setColour(juce::Label::textColourId, Colours::textDim);
        label->setJustificationType(juce::Justification::centred);
        addAndMakeVisible(label);
    }
    
    // 수치 표시 추가
    addAndMakeVisible(thresholdValue);
    addAndMakeVisible(makeupValue);
    addAndMakeVisible(grValue);
    
    // 버튼 설정 - Mute
    muteButton.setClickingTogglesState(true);
    muteButton.setColour(juce::TextButton::buttonColourId, Colours::btnInactive);
    muteButton.setColour(juce::TextButton::buttonOnColourId, Colours::btnMute);
    muteButton.setColour(juce::TextButton::textColourOffId, Colours::textDim);
    muteButton.setColour(juce::TextButton::textColourOnId, Colours::textBright);
    muteButton.onClick = [this]() {
        if (onMuteChanged) onMuteChanged(bandIndex, muteButton.getToggleState());
    };
    addAndMakeVisible(muteButton);
    
    // 버튼 설정 - Solo
    soloButton.setClickingTogglesState(true);
    soloButton.setColour(juce::TextButton::buttonColourId, Colours::btnInactive);
    soloButton.setColour(juce::TextButton::buttonOnColourId, Colours::btnSolo);
    soloButton.setColour(juce::TextButton::textColourOffId, Colours::textDim);
    soloButton.setColour(juce::TextButton::textColourOnId, Colours::bgDark);
    soloButton.onClick = [this]() {
        if (onSoloChanged) onSoloChanged(bandIndex, soloButton.getToggleState());
    };
    addAndMakeVisible(soloButton);
    
    // 버튼 설정 - Delta
    deltaButton.setClickingTogglesState(true);
    deltaButton.setColour(juce::TextButton::buttonColourId, Colours::btnInactive);
    deltaButton.setColour(juce::TextButton::buttonOnColourId, Colours::btnDelta);
    deltaButton.setColour(juce::TextButton::textColourOffId, Colours::textDim);
    deltaButton.setColour(juce::TextButton::textColourOnId, Colours::bgDark);
    deltaButton.onClick = [this]() {
        if (onDeltaChanged) onDeltaChanged(bandIndex, deltaButton.getToggleState());
    };
    addAndMakeVisible(deltaButton);
    
    // 버튼 설정 - Bypass
    bypassButton.setClickingTogglesState(true);
    bypassButton.setColour(juce::TextButton::buttonColourId, bandColour.withAlpha(0.6f));
    bypassButton.setColour(juce::TextButton::buttonOnColourId, Colours::btnBypass);
    bypassButton.setColour(juce::TextButton::textColourOffId, Colours::textBright);
    bypassButton.setColour(juce::TextButton::textColourOnId, Colours::textDim);
    bypassButton.onClick = [this]() {
        bypassButton.setButtonText(bypassButton.getToggleState() ? "OFF" : "ON");
        if (onBypassChanged) onBypassChanged(bandIndex, bypassButton.getToggleState());
    };
    addAndMakeVisible(bypassButton);
}

//==============================================================================
void BandSection::sliderValueChanged(juce::Slider* slider)
{
    if (slider == &thresholdSlider) {
        thresholdValue.setValue((float)thresholdSlider.getValue());
    }
    else if (slider == &makeupSlider) {
        makeupValue.setValue((float)makeupSlider.getValue());
    }
}

//==============================================================================
void BandSection::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    
    // 메인 배경 (모듈 스타일)
    g.setColour(Colours::bgModule);
    g.fillRoundedRectangle(bounds.toFloat(), 4.0f);
    
    // 상단 밴드 색상 악센트
    auto accentLine = bounds.removeFromTop(3);
    g.setColour(bandColour);
    g.fillRoundedRectangle(accentLine.reduced(1, 0).toFloat(), 1.5f);
    
    // 타이틀 영역
    auto titleArea = bounds.removeFromTop(24);
    g.setColour(Colours::textBright);
    g.setFont(juce::Font(juce::FontOptions().withHeight(12.0f).withStyle("Bold")));
    g.drawText(bandName, titleArea, juce::Justification::centred, false);
    
    // 테두리
    g.setColour(Colours::border);
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 4.0f, 1.0f);
}

//==============================================================================
void BandSection::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop(27);  // 타이틀 영역
    bounds = bounds.reduced(6, 4);
    
    // 상단 영역: GR 미터 + 수치
    auto topArea = bounds.removeFromTop(70);
    
    // GR 미터 (중앙)
    auto grArea = topArea.withSizeKeepingCentre(24, 50);
    grMeter.setBounds(grArea);
    
    // GR 수치 (미터 아래)
    grValue.setBounds(topArea.removeFromBottom(16).withSizeKeepingCentre(50, 16));
    
    bounds.removeFromTop(5);
    
    // 노브 영역
    auto knobArea = bounds.removeFromTop(90);
    int knobSize = 50;
    int knobSpacing = (knobArea.getWidth() - knobSize * 2) / 3;
    
    // Threshold 노브
    auto thrArea = knobArea.removeFromLeft(knobSpacing + knobSize);
    thrArea.removeFromLeft(knobSpacing / 2);
    thresholdSlider.setBounds(thrArea.removeFromTop(knobSize).withSizeKeepingCentre(knobSize, knobSize));
    thrArea.removeFromTop(2);
    thresholdLabel.setBounds(thrArea.removeFromTop(12));
    thresholdValue.setBounds(thrArea.removeFromTop(16).withSizeKeepingCentre(50, 16));
    
    // Makeup 노브
    auto mkArea = knobArea.removeFromLeft(knobSpacing + knobSize);
    makeupSlider.setBounds(mkArea.removeFromTop(knobSize).withSizeKeepingCentre(knobSize, knobSize));
    mkArea.removeFromTop(2);
    makeupLabel.setBounds(mkArea.removeFromTop(12));
    makeupValue.setBounds(mkArea.removeFromTop(16).withSizeKeepingCentre(50, 16));
    
    bounds.removeFromTop(8);
    
    // 버튼 영역
    auto buttonArea = bounds.removeFromTop(60);
    int btnWidth = 26;
    int btnHeight = 20;
    int btnGap = 4;
    
    // M/S 버튼 (중앙 정렬)
    auto msRow = buttonArea.removeFromTop(btnHeight);
    int totalMsWidth = btnWidth * 2 + btnGap;
    msRow = msRow.withSizeKeepingCentre(totalMsWidth, btnHeight);
    muteButton.setBounds(msRow.removeFromLeft(btnWidth));
    msRow.removeFromLeft(btnGap);
    soloButton.setBounds(msRow.removeFromLeft(btnWidth));
    
    buttonArea.removeFromTop(btnGap);
    
    // Delta 버튼
    auto deltaRow = buttonArea.removeFromTop(btnHeight);
    deltaButton.setBounds(deltaRow.withSizeKeepingCentre(totalMsWidth, btnHeight));
    
    buttonArea.removeFromTop(btnGap);
    
    // Bypass/ON 버튼 (더 눈에 띄게)
    auto bypassRow = buttonArea.removeFromTop(btnHeight + 4);
    bypassButton.setBounds(bypassRow.withSizeKeepingCentre(totalMsWidth + 10, btnHeight + 4));
}

//==============================================================================
void BandSection::attachToParameters(juce::AudioProcessorValueTreeState& apvts)
{
    juce::String bandStr = juce::String(bandIndex + 1);
    
    thresholdAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, "band" + bandStr + "Thresh", thresholdSlider);
    
    makeupAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, "band" + bandStr + "Makeup", makeupSlider);
    
    // 초기 값 업데이트
    thresholdValue.setValue((float)thresholdSlider.getValue());
    makeupValue.setValue((float)makeupSlider.getValue());
}

//==============================================================================
void BandSection::setGainReduction(float grDb)
{
    grMeter.setValue(grDb);
    grValue.setValue(grDb);
}

} // namespace ELC4L
