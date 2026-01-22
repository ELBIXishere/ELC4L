//-------------------------------------------------------------------------------------------------------
// ELC4L - Elite 4-Band Compressor + Limiter
// Custom Look and Feel - iZotope Ozone Inspired Premium Dark Theme
//-------------------------------------------------------------------------------------------------------

#pragma once

#include <JuceHeader.h>

namespace ELC4L {

//=======================================================================
// 색상 상수 (Ozone 스타일 다크 테마 - 거의 검정 배경)
//=======================================================================
namespace Colours {
    // 배경색 (Ozone 스타일 - 매우 어두운 다크)
    const juce::Colour bgDark       { 17, 18, 21 };       // 메인 배경 #111215
    const juce::Colour bgPanel      { 22, 24, 28 };       // 패널 배경 #161820
    const juce::Colour bgModule     { 28, 30, 36 };       // 모듈 배경 #1C1E24
    const juce::Colour bgSpectrum   { 12, 13, 16 };       // 스펙트럼 배경 (더 어둡게)
    const juce::Colour bgInput      { 35, 38, 45 };       // 입력 필드 배경
    const juce::Colour bgMeter      { 18, 20, 25 };       // 미터 배경
    const juce::Colour bgHeader     { 14, 15, 18 };       // 헤더 배경
    
    // 테두리 (미묘한 구분선)
    const juce::Colour border       { 38, 42, 50 };
    const juce::Colour borderLight  { 52, 58, 68 };
    const juce::Colour borderAccent { 50, 140, 180 };
    const juce::Colour borderGlow   { juce::uint8(0), juce::uint8(160), juce::uint8(200), juce::uint8(60) };  // 글로우 효과
    
    // 주요 악센트 (Ozone 시안/블루)
    const juce::Colour accentCyan   { 0, 180, 220 };      // 메인 시안 #00B4DC
    const juce::Colour accentBlue   { 45, 125, 185 };     // 보조 블루
    const juce::Colour accentBright { 80, 200, 255 };     // 밝은 시안
    const juce::Colour accentDim    { 30, 90, 130 };      // 어두운 시안
    const juce::Colour accentGlow   { juce::uint8(0), juce::uint8(180), juce::uint8(220), juce::uint8(120) }; // 글로우 효과

    // 텍스트 (Ozone 스타일 - 밝은 회색)
    const juce::Colour textBright   { 230, 235, 245 };    // 주요 텍스트
    const juce::Colour textNormal   { 165, 175, 190 };    // 일반 텍스트
    const juce::Colour textDim      { 90, 100, 115 };     // 흐린 텍스트
    const juce::Colour textValue    { 180, 210, 240 };    // 수치 표시용
    const juce::Colour textUnit     { 100, 115, 135 };    // 단위 표시용

    // 밴드 색상 (Ozone 스타일)
    const juce::Colour bandLow      { 230, 85, 85 };      // 빨간색 (저역)
    const juce::Colour bandLowMid   { 130, 195, 75 };     // 연두색 (중저역)
    const juce::Colour bandHiMid    { 55, 155, 215 };     // 파란색 (중고역)
    const juce::Colour bandHigh     { 210, 175, 70 };     // 노란색/주황 (고역)

    // 스펙트럼 색상 (Ozone 웨이브폼 스타일)
    const juce::Colour specInput    { 45, 130, 180 };     // 입력 스펙트럼 (블루)
    const juce::Colour specOutput   { 75, 175, 230 };     // 출력 스펙트럼 (밝은 시안)
    const juce::Colour specFillIn   { juce::uint8(30), juce::uint8(100), juce::uint8(150), juce::uint8(80) }; // 입력 필 (반투명)
    const juce::Colour specFillOut  { juce::uint8(40), juce::uint8(140), juce::uint8(200), juce::uint8(60) }; // 출력 필 (반투명)
    const juce::Colour specGrid     { 35, 40, 50 };       // 그리드 라인
    const juce::Colour specGridText { 65, 75, 90 };       // 그리드 텍스트

    // 리미터/클리핑
    const juce::Colour limiter      { 230, 130, 70 };     // 주황색
    const juce::Colour limiterBright{ 255, 170, 100 };
    const juce::Colour clipRed      { 255, 70, 70 };

    // 미터 색상 (Ozone 스타일 그래디언트)
    const juce::Colour meterGreen   { 75, 200, 120 };
    const juce::Colour meterYellow  { 230, 195, 70 };
    const juce::Colour meterOrange  { 245, 140, 60 };
    const juce::Colour meterRed     { 245, 70, 70 };
    const juce::Colour meterBg      { 22, 25, 32 };
    const juce::Colour meterTick    { 45, 50, 62 };       // 미터 눈금
    
    // GR 미터 (시안 계열)
    const juce::Colour grMeterFill  { 0, 175, 215 };
    const juce::Colour grMeterPeak  { 255, 90, 90 };
    const juce::Colour grMeterBg    { 20, 23, 30 };

    // 버튼 색상 (세련된 플랫 스타일)
    const juce::Colour btnInactive  { 32, 36, 44 };
    const juce::Colour btnHover     { 45, 52, 65 };
    const juce::Colour btnActive    { 55, 135, 180 };
    const juce::Colour btnMute      { 210, 65, 65 };
    const juce::Colour btnSolo      { 220, 190, 50 };
    const juce::Colour btnDelta     { 50, 190, 190 };
    const juce::Colour btnBypass    { 100, 105, 120 };
    
    // 슬라이더/노브 (Ozone 스타일)
    const juce::Colour knobFill     { 40, 45, 55 };
    const juce::Colour knobRing     { 0, 170, 210 };
    const juce::Colour knobPointer  { 220, 230, 245 };
    const juce::Colour knobTrack    { 30, 34, 42 };
    const juce::Colour faderTrack   { 28, 32, 40 };
    const juce::Colour faderThumb   { 145, 155, 175 };
    const juce::Colour faderActive  { 0, 170, 210 };
    
    // 밴드 색상 배열
    inline const juce::Colour& getBandColour(int bandIndex) {
        static const juce::Colour bandColours[] = { bandLow, bandLowMid, bandHiMid, bandHigh };
        return bandColours[juce::jlimit(0, 3, bandIndex)];
    }
}

//=======================================================================
// 레이아웃 상수 (Ozone 스타일 - 더 넓은 레이아웃)
//=======================================================================
namespace Layout {
    constexpr int WindowW = 940;
    constexpr int WindowH = 620;
    constexpr int HeaderH = 40;
    constexpr int Padding = 10;
    constexpr int Margin = 6;

    // 스펙트럼/웨이브폼 영역 (상단)
    constexpr int SpectrumX = 8;
    constexpr int SpectrumY = HeaderH + 4;
    constexpr int SpectrumW = 700;
    constexpr int SpectrumH = 185;

    // 우측 미터 영역
    constexpr int MeterPanelX = SpectrumX + SpectrumW + 8;
    constexpr int MeterPanelW = 210;
    constexpr int MeterPanelH = SpectrumH;

    // 하단 컨트롤 영역
    constexpr int ControlY = SpectrumY + SpectrumH + 12;
    constexpr int ControlH = WindowH - ControlY - 8;

    // 밴드 섹션
    constexpr int BandWidth = 135;
    constexpr int BandHeight = ControlH;
    constexpr int BandStartX = 8;
    constexpr int BandGap = 4;

    // 리미터 섹션
    constexpr int LimiterX = BandStartX + (BandWidth + BandGap) * 4 + 12;
    constexpr int LimiterW = 220;

    // 글로벌 컨트롤
    constexpr int GlobalX = LimiterX + LimiterW + 12;
    constexpr int GlobalW = WindowW - GlobalX - 8;
}

//=======================================================================
// 커스텀 LookAndFeel 클래스 (Ozone 스타일)
//=======================================================================
class CustomLookAndFeel : public juce::LookAndFeel_V4
{
public:
    CustomLookAndFeel();
    ~CustomLookAndFeel() override = default;

    //==============================================================================
    // 슬라이더 (페이더/노브) - Ozone 스타일
    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          juce::Slider::SliderStyle style, juce::Slider& slider) override;

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                          juce::Slider& slider) override;

    juce::Label* createSliderTextBox(juce::Slider& slider) override;
    
    //==============================================================================
    // 버튼 - 세련된 플랫 스타일
    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                              const juce::Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown) override;

    void drawButtonText(juce::Graphics& g, juce::TextButton& button,
                        bool shouldDrawButtonAsHighlighted,
                        bool shouldDrawButtonAsDown) override;

    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                          bool shouldDrawButtonAsHighlighted,
                          bool shouldDrawButtonAsDown) override;

    //==============================================================================
    // 콤보박스
    void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                      int buttonX, int buttonY, int buttonW, int buttonH,
                      juce::ComboBox& comboBox) override;

    //==============================================================================
    // 레이블
    void drawLabel(juce::Graphics& g, juce::Label& label) override;

    //==============================================================================
    // 폰트
    juce::Font getLabelFont(juce::Label& label) override;
    juce::Font getTextButtonFont(juce::TextButton& button, int buttonHeight) override;
    juce::Font getSliderPopupFont(juce::Slider&) override;

    //==============================================================================
    // 유틸리티 그리기 함수
    static void drawPanelBackground(juce::Graphics& g, juce::Rectangle<int> bounds, 
                                    juce::Colour colour = Colours::bgPanel, 
                                    float cornerSize = 3.0f);
    
    static void drawModuleBackground(juce::Graphics& g, juce::Rectangle<int> bounds,
                                     const juce::String& title = "", 
                                     juce::Colour titleColour = Colours::textDim);
    
    static void drawMeter(juce::Graphics& g, juce::Rectangle<int> bounds,
                         float level, float peak, bool isGR = false);
    
    static void drawValueDisplay(juce::Graphics& g, juce::Rectangle<int> bounds,
                                 const juce::String& value, const juce::String& unit = "",
                                 juce::Colour valueColour = Colours::textValue);
    
    // Ozone 스타일 글로우 효과
    static void drawGlowEffect(juce::Graphics& g, juce::Rectangle<float> bounds,
                               juce::Colour colour, float radius = 8.0f);
    
    // 세련된 섹션 타이틀
    static void drawSectionTitle(juce::Graphics& g, juce::Rectangle<int> bounds,
                                 const juce::String& title, juce::Colour accentColour);

private:
    juce::Font mainFont;
    juce::Font titleFont;
    juce::Font smallFont;
    juce::Font valueFont;
    juce::Font monoFont;  // 수치 표시용 고정폭 폰트

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CustomLookAndFeel)
};

} // namespace ELC4L
