//-------------------------------------------------------------------------------------------------------
// ELC4L - Elite 4-Band Compressor + Limiter
// Limiter Section Component - Ozone Style
//-------------------------------------------------------------------------------------------------------

#pragma once

#include <JuceHeader.h>
#include "CustomLookAndFeel.h"
#include "Meters.h"
#include "BandSection.h"  // ValueDisplay

namespace ELC4L {

//=======================================================================
// 리미터 섹션 컴포넌트 (Ozone 스타일)
// GR 미터 + Threshold/Ceiling/Release 노브 + Bypass 버튼 + 수치 표시
//=======================================================================
class LimiterSection : public juce::Component, public juce::Slider::Listener
{
public:
    LimiterSection();
    ~LimiterSection() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;
    
    void sliderValueChanged(juce::Slider* slider) override;

    //==============================================================================
    void attachToParameters(juce::AudioProcessorValueTreeState& apvts);
    void setGainReduction(float grDb);
    
    bool isBypassed() const { return bypassButton.getToggleState(); }
    void setBypassed(bool state) { bypassButton.setToggleState(state, juce::dontSendNotification); }
    
    std::function<void(bool state)> onBypassChanged;

private:
    // GR 미터
    GRMeter grMeter;
    
    // 슬라이더 (노브 스타일)
    juce::Slider thresholdSlider;
    juce::Slider ceilingSlider;
    juce::Slider releaseSlider;
    
    juce::Label thresholdLabel { {}, "THRESHOLD" };
    juce::Label ceilingLabel { {}, "CEILING" };
    juce::Label releaseLabel { {}, "RELEASE" };
    
    // 수치 표시
    ValueDisplay thresholdValue { "dB" };
    ValueDisplay ceilingValue { "dB" };
    ValueDisplay releaseValue { "ms" };
    ValueDisplay grValue { "dB" };
    
    // 버튼
    juce::TextButton bypassButton { "LIMITER ON" };
    juce::ToggleButton truePeakButton { "True Peak" };
    
    // 파라미터 연결
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> thresholdAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> ceilingAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> releaseAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LimiterSection)
};

} // namespace ELC4L
