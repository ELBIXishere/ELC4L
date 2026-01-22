//-------------------------------------------------------------------------------------------------------
// ELC4L - Elite 4-Band Compressor + Limiter
// Spectrum Analyzer Component - VST2 Style (Catmull-Rom + Gold/Cyan Theme)
//-------------------------------------------------------------------------------------------------------

#pragma once

#include <JuceHeader.h>
#include "CustomLookAndFeel.h"
#include "../DSP/DSPModules.h"

namespace ELC4L {

//=======================================================================
// 스펙트럼 분석기 컴포넌트 (VST2 스타일)
// Catmull-Rom 스플라인 스무딩 + Gold/Cyan 색상 테마
//=======================================================================
class SpectrumAnalyzer : public juce::Component,
                          public juce::Timer
{
public:
    SpectrumAnalyzer();
    ~SpectrumAnalyzer() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    //==============================================================================
    // 스펙트럼 데이터 설정
    void setSpectrumData(const float* inputData, const float* outputData, int numBins);
    
    // 크로스오버 주파수 설정
    void setCrossoverFrequencies(float xover1, float xover2, float xover3);
    
    // 크로스오버 드래그 콜백
    std::function<void(int crossoverIndex, float newFrequency)> onCrossoverChanged;

    //==============================================================================
    // 마우스 이벤트 (크로스오버 드래그)
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseMove(const juce::MouseEvent& event) override;

private:
    //==============================================================================
    // VST2 스타일 색상 상수 (원본과 동일)
    // 배경
    const juce::Colour kBgSpectrum   { 18, 20, 26 };      // 스펙트럼 배경
    const juce::Colour kBorder       { 35, 38, 45 };      // 테두리
    const juce::Colour kGrid         { 35, 35, 42 };      // 그리드 라인
    const juce::Colour kTextDim      { 100, 105, 115 };   // 흐린 텍스트
    
    // 스펙트럼 색상 (VST2와 동일 - 시안 Input, 골드/오렌지 Output)
    const juce::Colour kSpecInput    { 75, 180, 180 };    // 입력 - 시안/청록
    const juce::Colour kSpecOutput   { 220, 170, 80 };    // 출력 - 골드/오렌지
    const juce::Colour kSpecFillIn   { 60, 140, 140 };    // 입력 필 색상
    const juce::Colour kSpecFillOut  { 180, 140, 60 };    // 출력 필 색상
    
    // 크로스오버 색상 (VST2 스타일 - 골드 계열)
    const juce::Colour kXoverNormal  { 200, 160, 60 };    // 기본 상태
    const juce::Colour kXoverHover   { 230, 190, 80 };    // 호버 상태
    const juce::Colour kXoverActive  { 255, 210, 100 };   // 드래그 상태
    
    //==============================================================================
    // 내부 데이터
    std::array<float, kDisplayBins> spectrumIn;
    std::array<float, kDisplayBins> spectrumOut;
    
    float xoverFreq1 = 120.0f;
    float xoverFreq2 = 800.0f;
    float xoverFreq3 = 4000.0f;
    
    // 드래그 상태
    int activeCrossover = -1;
    int hoveredCrossover = -1;
    
    //==============================================================================
    // 그래픽 영역
    juce::Rectangle<int> graphArea;
    
    // 유틸리티 함수
    float frequencyToX(float freq) const;
    float xToFrequency(float x) const;
    float dbToY(float db) const;
    int hitTestCrossover(int x, int y) const;
    
    // Catmull-Rom 스플라인 보간 (VST2와 동일)
    float catmullRom(float p0, float p1, float p2, float p3, float t) const;
    
    // 그리기 함수
    void drawBackground(juce::Graphics& g);
    void drawGrid(juce::Graphics& g);
    void drawSpectrumVST2Style(juce::Graphics& g, const float* data, 
                                juce::Colour lineColour, juce::Colour fillColour, 
                                float fillAlpha = 0.3f);
    void drawCrossoverLines(juce::Graphics& g);
    void drawBandLabels(juce::Graphics& g);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrumAnalyzer)
};

} // namespace ELC4L
