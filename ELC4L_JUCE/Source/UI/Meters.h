//-------------------------------------------------------------------------------------------------------
// ELC4L - Elite 4-Band Compressor + Limiter
// Meter Components with Peak Hold
//-------------------------------------------------------------------------------------------------------

#pragma once

#include <JuceHeader.h>
#include "CustomLookAndFeel.h"

namespace ELC4L {

//=======================================================================
// 수직 미터 컴포넌트 (피크 홀드 포함)
// 레벨 미터 또는 GR 미터로 사용 가능
//=======================================================================
class VerticalMeter : public juce::Component,
                       public juce::Timer
{
public:
    VerticalMeter();
    ~VerticalMeter() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override {}
    void timerCallback() override;
    
    // 마우스 클릭으로 피크 리셋
    void mouseDown(const juce::MouseEvent& event) override;

    //==============================================================================
    // 값 설정
    void setValue(float dbValue);
    void setGainReductionMode(bool isGR);
    void setColour(juce::Colour colour);
    void setLabel(const juce::String& label);
    
    // 피크 홀드 설정
    void setPeakHoldTime(int milliseconds);
    void resetPeak();
    
    // 값 가져오기
    float getValue() const { return currentDb; }
    float getPeakValue() const { return peakDb; }

private:
    float currentDb = -120.0f;
    float peakDb = -120.0f;
    float currentGr = 0.0f;     // GR 모드용 현재 값
    float peakGr = 0.0f;        // GR 모드용 피크 값
    juce::int64 peakHoldStartTime = 0;
    int peakHoldTimeMs = 8000;  // 8초 피크 홀드
    
    bool isGainReduction = false;
    juce::Colour meterColour = Colours::meterGreen;
    juce::String labelText;
    
    // dB를 Y 좌표로 변환
    float dbToY(float db) const;
    
    // 레벨에 따른 색상 결정
    juce::Colour getColourForLevel(float db) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VerticalMeter)
};

//=======================================================================
// LUFS 미터 컴포넌트 (UI용)
//=======================================================================
class LufsMeterUI : public juce::Component,
                   public juce::Timer
{
public:
    LufsMeterUI();
    ~LufsMeterUI() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override {}
    void timerCallback() override;
    void mouseDown(const juce::MouseEvent& event) override;

    void setValue(float lufsValue);
    void resetPeak();
    
    float getValue() const { return currentLufs; }
    float getPeakValue() const { return peakLufs; }

private:
    float currentLufs = -120.0f;
    float peakLufs = -120.0f;
    juce::int64 peakHoldStartTime = 0;
    int peakHoldTimeMs = 8000;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LufsMeterUI)
};

//=======================================================================
// IN/OUT 미터 패널 (두 개의 수직 미터 + LUFS)
//=======================================================================
class IOMetersPanel : public juce::Component,
                       public juce::Timer
{
public:
    IOMetersPanel();
    ~IOMetersPanel() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    void setInputLevel(float db);
    void setOutputLevel(float db);
    void setLufsLevel(float lufs);
    
    void resetAllPeaks();

private:
    VerticalMeter inputMeter;
    VerticalMeter outputMeter;
    LufsMeterUI lufsMeter;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(IOMetersPanel)
};

//=======================================================================
// GR 미터 (밴드별 / 리미터용)
//=======================================================================
class GRMeter : public juce::Component,
                 public juce::Timer
{
public:
    GRMeter();
    ~GRMeter() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override {}
    void timerCallback() override;
    void mouseDown(const juce::MouseEvent& event) override;

    void setValue(float grDb);  // GR은 양수로 전달 (예: 6.0 = -6dB GR)
    void setColour(juce::Colour colour);
    void resetPeak();
    
    float getValue() const { return currentGr; }
    float getPeakValue() const { return peakGr; }

private:
    float currentGr = 0.0f;
    float peakGr = 0.0f;
    juce::int64 peakHoldStartTime = 0;
    int peakHoldTimeMs = 8000;
    juce::Colour meterColour = Colours::bandLow;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GRMeter)
};

} // namespace ELC4L
