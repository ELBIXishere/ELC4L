//-------------------------------------------------------------------------------------------------------
// ELC4L - Elite 4-Band Compressor + Limiter
// Band Section Component - Ozone Style
//-------------------------------------------------------------------------------------------------------

#pragma once

#include <JuceHeader.h>
#include "CustomLookAndFeel.h"
#include "Meters.h"

namespace ELC4L {

//=======================================================================
// 수치 표시 레이블 컴포넌트
//=======================================================================
class ValueDisplay : public juce::Component, public juce::Timer
{
public:
    ValueDisplay(const juce::String& suffix = "dB") : unitSuffix(suffix) {
        startTimerHz(15);
    }
    
    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat();
        
        // 배경
        g.setColour(Colours::bgInput);
        g.fillRoundedRectangle(bounds, 2.0f);
        
        // 값
        g.setColour(Colours::textValue);
        g.setFont(juce::Font(juce::FontOptions().withHeight(11.0f).withStyle("Bold")));
        g.drawText(juce::String(currentValue, 1) + " " + unitSuffix, bounds.reduced(2),
                   juce::Justification::centred, false);
    }
    
    void timerCallback() override { repaint(); }
    void setValue(float val) { currentValue = val; }
    
private:
    float currentValue = 0.0f;
    juce::String unitSuffix;
};

//=======================================================================
// 단일 밴드 컨트롤 섹션 (Ozone 스타일)
// GR 미터 + Threshold/Makeup 노브 + M/S/Delta/Bypass 버튼 + 수치 표시
//=======================================================================
class BandSection : public juce::Component, public juce::Slider::Listener
{
public:
    BandSection(int bandIndex);
    ~BandSection() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;
    
    // 슬라이더 값 변경 콜백
    void sliderValueChanged(juce::Slider* slider) override;

    //==============================================================================
    // 슬라이더 연결
    void attachToParameters(juce::AudioProcessorValueTreeState& apvts);
    
    // GR 값 설정
    void setGainReduction(float grDb);
    
    // 버튼 상태
    bool isMuted() const { return muteButton.getToggleState(); }
    bool isSoloed() const { return soloButton.getToggleState(); }
    bool isDelta() const { return deltaButton.getToggleState(); }
    bool isBypassed() const { return bypassButton.getToggleState(); }
    
    // 버튼 상태 설정
    void setMuted(bool state) { muteButton.setToggleState(state, juce::dontSendNotification); }
    void setSoloed(bool state) { soloButton.setToggleState(state, juce::dontSendNotification); }
    void setDelta(bool state) { deltaButton.setToggleState(state, juce::dontSendNotification); }
    void setBypassed(bool state) { bypassButton.setToggleState(state, juce::dontSendNotification); }
    
    // 버튼 콜백
    std::function<void(int band, bool state)> onMuteChanged;
    std::function<void(int band, bool state)> onSoloChanged;
    std::function<void(int band, bool state)> onDeltaChanged;
    std::function<void(int band, bool state)> onBypassChanged;

private:
    int bandIndex;
    juce::String bandName;
    juce::Colour bandColour;
    
    // GR 미터
    GRMeter grMeter;
    
    // 슬라이더 (노브 스타일)
    juce::Slider thresholdSlider;
    juce::Slider makeupSlider;
    
    juce::Label thresholdLabel { {}, "THRESHOLD" };
    juce::Label makeupLabel { {}, "MAKEUP" };
    
    // 수치 표시
    ValueDisplay thresholdValue { "dB" };
    ValueDisplay makeupValue { "dB" };
    ValueDisplay grValue { "dB" };
    
    // 버튼
    juce::TextButton muteButton { "M" };
    juce::TextButton soloButton { "S" };
    juce::TextButton deltaButton { "Δ" };
    juce::TextButton bypassButton { "ON" };
    
    // 파라미터 연결
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> thresholdAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> makeupAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BandSection)
};

} // namespace ELC4L
