//-------------------------------------------------------------------------------------------------------
// ELC4L - Elite 4-Band Compressor + Limiter
// Custom Look and Feel Implementation - iZotope Ozone Style
//-------------------------------------------------------------------------------------------------------

#include "CustomLookAndFeel.h"

namespace ELC4L {

//==============================================================================
CustomLookAndFeel::CustomLookAndFeel()
{
    // 폰트 설정 (Segoe UI / San Francisco 스타일)
    mainFont = juce::Font(juce::FontOptions().withHeight(12.0f));
    titleFont = juce::Font(juce::FontOptions().withHeight(10.0f).withStyle("Bold"));
    smallFont = juce::Font(juce::FontOptions().withHeight(9.5f));
    valueFont = juce::Font(juce::FontOptions().withHeight(11.0f).withStyle("Bold"));
    monoFont = juce::Font(juce::FontOptions().withHeight(11.0f));
    
    // 기본 색상 설정
    setColour(juce::Slider::thumbColourId, Colours::knobPointer);
    setColour(juce::Slider::trackColourId, Colours::faderTrack);
    setColour(juce::Slider::rotarySliderFillColourId, Colours::knobRing);
    setColour(juce::Slider::rotarySliderOutlineColourId, Colours::knobFill);
    setColour(juce::Slider::textBoxTextColourId, Colours::textValue);
    setColour(juce::Slider::textBoxBackgroundColourId, Colours::bgInput);
    setColour(juce::Slider::textBoxOutlineColourId, Colours::border);
    
    setColour(juce::Label::textColourId, Colours::textNormal);
    setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    
    setColour(juce::TextButton::buttonColourId, Colours::btnInactive);
    setColour(juce::TextButton::buttonOnColourId, Colours::btnActive);
    setColour(juce::TextButton::textColourOnId, Colours::textBright);
    setColour(juce::TextButton::textColourOffId, Colours::textNormal);
    
    setColour(juce::ComboBox::backgroundColourId, Colours::bgInput);
    setColour(juce::ComboBox::textColourId, Colours::textNormal);
    setColour(juce::ComboBox::outlineColourId, Colours::border);
    setColour(juce::ComboBox::arrowColourId, Colours::textDim);
    
    setColour(juce::PopupMenu::backgroundColourId, Colours::bgPanel);
    setColour(juce::PopupMenu::textColourId, Colours::textNormal);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, Colours::btnActive);
    setColour(juce::PopupMenu::highlightedTextColourId, Colours::textBright);
}

//==============================================================================
// 선형 슬라이더 (Ozone 스타일)
void CustomLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                                          float sliderPos, float /*minSliderPos*/, float /*maxSliderPos*/,
                                          juce::Slider::SliderStyle style, juce::Slider& /*slider*/)
{
    auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat();
    
    if (style == juce::Slider::LinearVertical) {
        // Ozone 스타일 세로 페이더
        float trackWidth = 5.0f;
        auto trackBounds = bounds.withSizeKeepingCentre(trackWidth, bounds.getHeight() - 8);
        
        // 트랙 배경 (어두운 홈)
        g.setColour(Colours::faderTrack);
        g.fillRoundedRectangle(trackBounds, 2.5f);
        
        // 채워진 부분 (아래에서 위로 - 시안 그래디언트)
        auto fillBounds = trackBounds.withTop(sliderPos).withBottom(trackBounds.getBottom());
        
        juce::ColourGradient gradient(Colours::accentBright, fillBounds.getCentreX(), fillBounds.getY(),
                                       Colours::accentCyan, fillBounds.getCentreX(), fillBounds.getBottom(),
                                       false);
        g.setGradientFill(gradient);
        g.fillRoundedRectangle(fillBounds, 2.5f);
        
        // 글로우 효과
        g.setColour(Colours::accentGlow);
        g.fillRoundedRectangle(fillBounds.expanded(1.0f), 3.0f);
        
        // 실제 채우기
        g.setGradientFill(gradient);
        g.fillRoundedRectangle(fillBounds, 2.5f);
        
        // 썸 (Ozone 스타일 - 가로로 넓은 직사각형)
        float thumbHeight = 10.0f;
        float thumbWidth = 22.0f;
        auto thumbBounds = juce::Rectangle<float>(bounds.getCentreX() - thumbWidth / 2,
                                                   sliderPos - thumbHeight / 2,
                                                   thumbWidth, thumbHeight);
        
        // 썸 그림자
        g.setColour(juce::Colours::black.withAlpha(0.4f));
        g.fillRoundedRectangle(thumbBounds.translated(0, 1), 2.0f);
        
        // 썸 배경 (밝은 회색)
        g.setColour(Colours::faderThumb);
        g.fillRoundedRectangle(thumbBounds, 2.0f);
        
        // 썸 중앙 그루브
        g.setColour(Colours::bgPanel);
        g.drawLine(thumbBounds.getX() + 5, thumbBounds.getCentreY(),
                   thumbBounds.getRight() - 5, thumbBounds.getCentreY(), 1.5f);
    }
    else if (style == juce::Slider::LinearHorizontal) {
        // 수평 슬라이더 (HPF 등)
        float trackHeight = 3.0f;
        auto trackBounds = bounds.withSizeKeepingCentre(bounds.getWidth() - 8, trackHeight);
        
        // 트랙 배경
        g.setColour(Colours::faderTrack);
        g.fillRoundedRectangle(trackBounds, 1.5f);
        
        // 채워진 부분
        auto fillBounds = trackBounds.withRight(sliderPos);
        g.setColour(Colours::accentCyan);
        g.fillRoundedRectangle(fillBounds, 1.5f);
        
        // 썸 (원형)
        float thumbSize = 14.0f;
        auto thumbBounds = juce::Rectangle<float>(sliderPos - thumbSize / 2,
                                                   bounds.getCentreY() - thumbSize / 2,
                                                   thumbSize, thumbSize);
        
        // 글로우
        g.setColour(Colours::accentGlow);
        g.fillEllipse(thumbBounds.expanded(2.0f));
        
        // 썸 배경
        g.setColour(Colours::faderThumb);
        g.fillEllipse(thumbBounds);
        
        // 내부 시안 원
        g.setColour(Colours::accentCyan);
        g.fillEllipse(thumbBounds.reduced(4.0f));
    }
}

//==============================================================================
// 로터리 슬라이더 (Ozone 스타일 노브)
void CustomLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                          float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                                          juce::Slider& /*slider*/)
{
    auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat();
    auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f - 3.0f;
    auto centreX = bounds.getCentreX();
    auto centreY = bounds.getCentreY();
    auto rx = centreX - radius;
    auto ry = centreY - radius;
    auto rw = radius * 2.0f;
    auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    
    // 외곽 글로우 (값이 있을 때)
    if (sliderPos > 0.01f) {
        g.setColour(Colours::accentGlow);
        g.fillEllipse(rx - 2, ry - 2, rw + 4, rw + 4);
    }
    
    // 노브 배경 (어두운 원)
    juce::ColourGradient bgGradient(Colours::knobFill.brighter(0.1f), centreX, ry,
                                     Colours::knobFill.darker(0.1f), centreX, ry + rw, false);
    g.setGradientFill(bgGradient);
    g.fillEllipse(rx, ry, rw, rw);
    
    // 트랙 링 (배경)
    float trackRadius = radius - 2.0f;
    g.setColour(Colours::knobTrack);
    g.drawEllipse(centreX - trackRadius, centreY - trackRadius, trackRadius * 2, trackRadius * 2, 3.0f);
    
    // 값 링 (아크) - 시작부터 현재 값까지
    juce::Path arcPath;
    float arcRadius = trackRadius;
    arcPath.addCentredArc(centreX, centreY, arcRadius, arcRadius,
                          0.0f, rotaryStartAngle, angle, true);
    
    g.setColour(Colours::knobRing);
    g.strokePath(arcPath, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded));
    
    // 포인터 (점 + 라인)
    float pointerLength = radius * 0.55f;
    float pointerEndX = centreX + pointerLength * std::sin(angle);
    float pointerEndY = centreY - pointerLength * std::cos(angle);
    
    // 포인터 라인
    g.setColour(Colours::knobPointer);
    g.drawLine(centreX, centreY, pointerEndX, pointerEndY, 2.0f);
    
    // 포인터 끝점
    g.fillEllipse(pointerEndX - 3, pointerEndY - 3, 6, 6);
    
    // 중심점
    g.setColour(Colours::bgModule);
    g.fillEllipse(centreX - 5, centreY - 5, 10, 10);
    
    // 테두리
    g.setColour(Colours::border);
    g.drawEllipse(rx, ry, rw, rw, 1.0f);
}

//==============================================================================
juce::Label* CustomLookAndFeel::createSliderTextBox(juce::Slider& /*slider*/)
{
    auto* label = new juce::Label();
    label->setJustificationType(juce::Justification::centred);
    label->setColour(juce::Label::textColourId, Colours::textValue);
    label->setColour(juce::Label::backgroundColourId, Colours::bgInput);
    label->setColour(juce::Label::outlineColourId, Colours::border);
    label->setFont(valueFont);
    
    return label;
}

//==============================================================================
// 버튼 배경 (Ozone 스타일 플랫 버튼)
void CustomLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                              const juce::Colour& backgroundColour,
                                              bool shouldDrawButtonAsHighlighted,
                                              bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
    
    auto baseColour = backgroundColour;
    if (shouldDrawButtonAsDown)
        baseColour = baseColour.brighter(0.15f);
    else if (shouldDrawButtonAsHighlighted)
        baseColour = baseColour.brighter(0.08f);
    
    // 배경
    g.setColour(baseColour);
    g.fillRoundedRectangle(bounds, 2.0f);
    
    // 활성화 시 글로우 효과
    if (button.getToggleState()) {
        g.setColour(Colours::accentCyan.withAlpha(0.2f));
        g.fillRoundedRectangle(bounds.expanded(1.0f), 3.0f);
        g.setColour(baseColour);
        g.fillRoundedRectangle(bounds, 2.0f);
        
        // 상단 하이라이트
        auto highlight = bounds.removeFromTop(1);
        g.setColour(Colours::accentCyan.withAlpha(0.5f));
        g.fillRect(highlight.reduced(2, 0));
    }
    
    // 테두리
    g.setColour(button.getToggleState() ? Colours::accentCyan.withAlpha(0.6f) : Colours::border);
    g.drawRoundedRectangle(bounds, 2.0f, 1.0f);
}

//==============================================================================
void CustomLookAndFeel::drawButtonText(juce::Graphics& g, juce::TextButton& button,
                                        bool /*shouldDrawButtonAsHighlighted*/,
                                        bool /*shouldDrawButtonAsDown*/)
{
    auto bounds = button.getLocalBounds();
    
    g.setFont(getTextButtonFont(button, button.getHeight()));
    g.setColour(button.getToggleState() ? Colours::textBright : Colours::textNormal);
    g.drawText(button.getButtonText(), bounds, juce::Justification::centred, false);
}

//==============================================================================
void CustomLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                                          bool shouldDrawButtonAsHighlighted,
                                          bool /*shouldDrawButtonAsDown*/)
{
    auto bounds = button.getLocalBounds().toFloat();
    
    // 체크박스 영역
    float boxSize = 12.0f;
    auto boxBounds = bounds.removeFromLeft(boxSize + 5).withSizeKeepingCentre(boxSize, boxSize);
    
    // 배경
    g.setColour(button.getToggleState() ? Colours::accentCyan : Colours::btnInactive);
    g.fillRoundedRectangle(boxBounds, 2.0f);
    
    // 체크 표시
    if (button.getToggleState()) {
        g.setColour(Colours::textBright);
        juce::Path checkPath;
        checkPath.startNewSubPath(boxBounds.getX() + 2.5f, boxBounds.getCentreY());
        checkPath.lineTo(boxBounds.getCentreX() - 0.5f, boxBounds.getBottom() - 3);
        checkPath.lineTo(boxBounds.getRight() - 2.5f, boxBounds.getY() + 3);
        g.strokePath(checkPath, juce::PathStrokeType(1.5f));
    }
    
    // 텍스트
    g.setColour(shouldDrawButtonAsHighlighted ? Colours::textBright : Colours::textNormal);
    g.setFont(smallFont);
    g.drawText(button.getButtonText(), bounds.reduced(3, 0), juce::Justification::centredLeft, false);
}

//==============================================================================
void CustomLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool /*isButtonDown*/,
                                      int /*buttonX*/, int /*buttonY*/, int /*buttonW*/, int /*buttonH*/,
                                      juce::ComboBox& comboBox)
{
    auto bounds = juce::Rectangle<int>(0, 0, width, height).toFloat();
    
    // 배경
    g.setColour(Colours::bgInput);
    g.fillRoundedRectangle(bounds, 2.0f);
    
    // 테두리
    g.setColour(comboBox.hasKeyboardFocus(false) ? Colours::borderAccent : Colours::border);
    g.drawRoundedRectangle(bounds.reduced(0.5f), 2.0f, 1.0f);
    
    // 드롭다운 화살표
    juce::Path arrow;
    float arrowX = width - 12.0f;
    float arrowY = height * 0.5f - 2.0f;
    arrow.addTriangle(arrowX, arrowY, arrowX + 6, arrowY, arrowX + 3, arrowY + 4);
    g.setColour(Colours::textDim);
    g.fillPath(arrow);
}

//==============================================================================
void CustomLookAndFeel::drawLabel(juce::Graphics& g, juce::Label& label)
{
    g.fillAll(label.findColour(juce::Label::backgroundColourId));
    
    if (!label.isBeingEdited()) {
        auto textColour = label.findColour(juce::Label::textColourId);
        g.setColour(textColour);
        g.setFont(getLabelFont(label));
        
        auto textArea = label.getBorderSize().subtractedFrom(label.getLocalBounds());
        g.drawText(label.getText(), textArea, label.getJustificationType(), false);
    }
}

//==============================================================================
juce::Font CustomLookAndFeel::getLabelFont(juce::Label& /*label*/)
{
    return mainFont;
}

juce::Font CustomLookAndFeel::getTextButtonFont(juce::TextButton& /*button*/, int /*buttonHeight*/)
{
    return smallFont;
}

juce::Font CustomLookAndFeel::getSliderPopupFont(juce::Slider&)
{
    return valueFont;
}

//==============================================================================
// 패널 배경 그리기 (Ozone 스타일)
void CustomLookAndFeel::drawPanelBackground(juce::Graphics& g, juce::Rectangle<int> bounds, 
                                             juce::Colour colour, float cornerSize)
{
    // 배경
    g.setColour(colour);
    g.fillRoundedRectangle(bounds.toFloat(), cornerSize);
    
    // 미묘한 내부 그림자 (상단)
    juce::ColourGradient shadowGradient(juce::Colours::black.withAlpha(0.12f), 
                                         bounds.getX(), bounds.getY(),
                                         juce::Colours::transparentBlack,
                                         bounds.getX(), bounds.getY() + 8, false);
    g.setGradientFill(shadowGradient);
    g.fillRoundedRectangle(bounds.toFloat(), cornerSize);
    
    // 테두리
    g.setColour(Colours::border);
    g.drawRoundedRectangle(bounds.toFloat().reduced(0.5f), cornerSize, 1.0f);
}

//==============================================================================
// 모듈 배경 그리기 (타이틀 포함)
void CustomLookAndFeel::drawModuleBackground(juce::Graphics& g, juce::Rectangle<int> bounds,
                                              const juce::String& title, juce::Colour titleColour)
{
    auto boundsF = bounds.toFloat();
    
    // 배경
    g.setColour(Colours::bgModule);
    g.fillRoundedRectangle(boundsF, 3.0f);
    
    // 상단 악센트 라인
    auto accentRect = boundsF.removeFromTop(2);
    g.setColour(Colours::accentCyan.withAlpha(0.25f));
    g.fillRoundedRectangle(accentRect.reduced(1, 0), 1.0f);
    
    // 테두리
    g.setColour(Colours::border);
    g.drawRoundedRectangle(bounds.toFloat().reduced(0.5f), 3.0f, 1.0f);
    
    // 타이틀
    if (title.isNotEmpty()) {
        g.setColour(titleColour);
        g.setFont(juce::Font(juce::FontOptions().withHeight(9.5f).withStyle("Bold")));
        g.drawText(title, bounds.getX() + 6, bounds.getY() + 4, bounds.getWidth() - 12, 12,
                   juce::Justification::centredLeft, false);
    }
}

//==============================================================================
// 미터 그리기 (Ozone 스타일)
void CustomLookAndFeel::drawMeter(juce::Graphics& g, juce::Rectangle<int> bounds,
                                   float level, float peak, bool isGR)
{
    auto meterBounds = bounds.toFloat();
    
    // 배경
    g.setColour(isGR ? Colours::grMeterBg : Colours::meterBg);
    g.fillRoundedRectangle(meterBounds, 1.5f);
    
    if (isGR) {
        // GR 미터 (위에서 아래로 - 시안)
        float grNorm = juce::jlimit(0.0f, 1.0f, level / 24.0f);
        float fillHeight = grNorm * meterBounds.getHeight();
        
        auto fillBounds = meterBounds.withHeight(fillHeight);
        
        // GR 그래디언트 (밝은 시안 -> 어두운 시안)
        juce::ColourGradient grGradient(Colours::accentBright, fillBounds.getCentreX(), fillBounds.getY(),
                                         Colours::accentCyan, fillBounds.getCentreX(), fillBounds.getBottom(), false);
        g.setGradientFill(grGradient);
        g.fillRoundedRectangle(fillBounds, 1.5f);
        
        // 피크 마커
        if (peak > 0.1f) {
            float peakNorm = juce::jlimit(0.0f, 1.0f, peak / 24.0f);
            float peakY = peakNorm * meterBounds.getHeight();
            g.setColour(Colours::grMeterPeak);
            g.fillRect(meterBounds.getX(), meterBounds.getY() + peakY - 1.0f,
                       meterBounds.getWidth(), 2.0f);
        }
    } else {
        // 레벨 미터 (아래에서 위로)
        float dbNorm = (level + 60.0f) / 60.0f;
        dbNorm = juce::jlimit(0.0f, 1.0f, dbNorm);
        float fillHeight = dbNorm * meterBounds.getHeight();
        
        auto fillBounds = juce::Rectangle<float>(meterBounds.getX(),
                                                  meterBounds.getBottom() - fillHeight,
                                                  meterBounds.getWidth(), fillHeight);
        
        // Ozone 스타일 그래디언트 (녹색 -> 시안 -> 노란색 -> 빨간색)
        juce::ColourGradient gradient(Colours::meterGreen, fillBounds.getX(), fillBounds.getBottom(),
                                       Colours::meterRed, fillBounds.getX(), meterBounds.getY(), false);
        gradient.addColour(0.4, Colours::accentCyan);
        gradient.addColour(0.65, Colours::meterYellow);
        gradient.addColour(0.85, Colours::meterOrange);
        g.setGradientFill(gradient);
        g.fillRoundedRectangle(fillBounds, 1.5f);
        
        // 피크 마커
        if (peak > -100.0f) {
            float peakNorm = (peak + 60.0f) / 60.0f;
            peakNorm = juce::jlimit(0.0f, 1.0f, peakNorm);
            float peakY = meterBounds.getBottom() - peakNorm * meterBounds.getHeight();
            g.setColour(peak > -1.0f ? Colours::clipRed : Colours::textBright);
            g.fillRect(meterBounds.getX(), peakY - 1.0f, meterBounds.getWidth(), 2.0f);
        }
    }
    
    // 테두리
    g.setColour(Colours::border);
    g.drawRoundedRectangle(meterBounds, 1.5f, 0.5f);
}

//==============================================================================
// 수치 표시 그리기 (Ozone 스타일)
void CustomLookAndFeel::drawValueDisplay(juce::Graphics& g, juce::Rectangle<int> bounds,
                                          const juce::String& value, const juce::String& unit,
                                          juce::Colour valueColour)
{
    auto textBounds = bounds.toFloat();
    
    // 배경
    g.setColour(Colours::bgInput);
    g.fillRoundedRectangle(textBounds, 2.0f);
    
    // 값
    g.setColour(valueColour);
    g.setFont(juce::Font(juce::FontOptions().withHeight(11.0f).withStyle("Bold")));
    
    if (unit.isNotEmpty()) {
        auto valueWidth = g.getCurrentFont().getStringWidth(value);
        auto totalWidth = textBounds.getWidth();
        
        // 값을 중앙에서 약간 왼쪽에
        g.drawText(value, textBounds.reduced(2), juce::Justification::centred, false);
        
        // 단위를 오른쪽 아래에 작게
        g.setColour(Colours::textUnit);
        g.setFont(juce::Font(juce::FontOptions().withHeight(8.0f)));
        g.drawText(unit, textBounds.reduced(3).removeFromBottom(10).removeFromRight(textBounds.getWidth() * 0.4f),
                   juce::Justification::centredRight, false);
    } else {
        g.drawText(value, textBounds.reduced(2), juce::Justification::centred, false);
    }
    
    // 테두리
    g.setColour(Colours::border);
    g.drawRoundedRectangle(textBounds, 2.0f, 0.5f);
}

//==============================================================================
// 글로우 효과
void CustomLookAndFeel::drawGlowEffect(juce::Graphics& g, juce::Rectangle<float> bounds,
                                        juce::Colour colour, float radius)
{
    for (float i = radius; i > 0; i -= 1.0f) {
        float alpha = (1.0f - i / radius) * 0.15f;
        g.setColour(colour.withAlpha(alpha));
        g.fillRoundedRectangle(bounds.expanded(i), i * 0.5f);
    }
}

//==============================================================================
// 섹션 타이틀
void CustomLookAndFeel::drawSectionTitle(juce::Graphics& g, juce::Rectangle<int> bounds,
                                          const juce::String& title, juce::Colour accentColour)
{
    // 악센트 바
    auto accentBar = bounds.removeFromLeft(3);
    g.setColour(accentColour);
    g.fillRoundedRectangle(accentBar.toFloat(), 1.5f);
    
    // 타이틀 텍스트
    g.setColour(Colours::textNormal);
    g.setFont(juce::Font(juce::FontOptions().withHeight(11.0f).withStyle("Bold")));
    g.drawText(title, bounds.reduced(6, 0), juce::Justification::centredLeft, false);
}

} // namespace ELC4L
