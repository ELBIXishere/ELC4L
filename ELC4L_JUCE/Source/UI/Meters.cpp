//-------------------------------------------------------------------------------------------------------
// ELC4L - Elite 4-Band Compressor + Limiter
// Meter Components Implementation
//-------------------------------------------------------------------------------------------------------

#include "Meters.h"

namespace ELC4L {

//==============================================================================
// VerticalMeter Implementation
//==============================================================================
VerticalMeter::VerticalMeter()
{
    startTimerHz(30);
}

void VerticalMeter::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // 배경
    g.setColour(Colours::bgSpectrum);
    g.fillRoundedRectangle(bounds, 2.0f);
    
    // 테두리
    g.setColour(Colours::border);
    g.drawRoundedRectangle(bounds.reduced(0.5f), 2.0f, 1.0f);
    
    // 미터 영역
    auto meterBounds = bounds.reduced(1.0f);
    
    if (isGainReduction) {
        // GR 미터: 위에서 아래로 채움
        float grNorm = juce::jlimit(0.0f, 1.0f, currentGr / 24.0f);
        float filledHeight = grNorm * meterBounds.getHeight();
        
        juce::Rectangle<float> fillRect(meterBounds.getX(), meterBounds.getY(),
                                         meterBounds.getWidth(), filledHeight);
        
        // 그래디언트 채우기
        juce::ColourGradient gradient(meterColour.brighter(0.2f), fillRect.getX(), fillRect.getY(),
                                       meterColour.darker(0.2f), fillRect.getRight(), fillRect.getY(),
                                       false);
        g.setGradientFill(gradient);
        g.fillRect(fillRect);
        
        // 피크 마커
        if (peakGr > 0.1f) {
            float peakNorm = juce::jlimit(0.0f, 1.0f, peakGr / 24.0f);
            float peakY = meterBounds.getY() + peakNorm * meterBounds.getHeight();
            
            g.setColour(Colours::meterRed);
            g.drawLine(meterBounds.getX() - 2, peakY, meterBounds.getRight() + 2, peakY, 2.0f);
        }
    } else {
        // 레벨 미터: 아래에서 위로 채움
        float dbNorm = (currentDb + 60.0f) / 60.0f;
        dbNorm = juce::jlimit(0.0f, 1.0f, dbNorm);
        float filledHeight = dbNorm * meterBounds.getHeight();
        
        juce::Rectangle<float> fillRect(meterBounds.getX(), 
                                         meterBounds.getBottom() - filledHeight,
                                         meterBounds.getWidth(), filledHeight);
        
        // 색상 결정 (레벨에 따라)
        juce::Colour fillColour = getColourForLevel(currentDb);
        
        // 그래디언트 채우기
        juce::ColourGradient gradient(fillColour.brighter(0.2f), fillRect.getX(), fillRect.getY(),
                                       fillColour.darker(0.2f), fillRect.getRight(), fillRect.getY(),
                                       false);
        g.setGradientFill(gradient);
        g.fillRect(fillRect);
        
        // 피크 마커
        if (peakDb > -100.0f) {
            float peakNorm = (peakDb + 60.0f) / 60.0f;
            peakNorm = juce::jlimit(0.0f, 1.0f, peakNorm);
            float peakY = meterBounds.getBottom() - peakNorm * meterBounds.getHeight();
            
            g.setColour(Colours::meterRed);
            g.drawLine(meterBounds.getX() - 2, peakY, meterBounds.getRight() + 2, peakY, 2.0f);
        }
    }
    
    // 레이블
    if (labelText.isNotEmpty()) {
        g.setColour(Colours::textDim);
        g.setFont(10.0f);
        g.drawText(labelText, bounds.toNearestInt().translated(0, (int)bounds.getHeight() + 2),
                   juce::Justification::centredTop, false);
    }
}

void VerticalMeter::timerCallback()
{
    // 피크 홀드 타임아웃 체크
    if (peakHoldStartTime > 0) {
        juce::int64 elapsed = juce::Time::currentTimeMillis() - peakHoldStartTime;
        if (elapsed > peakHoldTimeMs) {
            if (isGainReduction) {
                peakGr = 0.0f;
            } else {
                peakDb = -120.0f;
            }
            peakHoldStartTime = 0;
        }
    }
    repaint();
}

void VerticalMeter::mouseDown(const juce::MouseEvent& /*event*/)
{
    resetPeak();
}

void VerticalMeter::setValue(float dbValue)
{
    currentDb = dbValue;
    
    if (isGainReduction) {
        currentGr = dbValue;
        if (currentGr > peakGr) {
            peakGr = currentGr;
            peakHoldStartTime = juce::Time::currentTimeMillis();
        }
    } else {
        if (currentDb > peakDb) {
            peakDb = currentDb;
            peakHoldStartTime = juce::Time::currentTimeMillis();
        }
    }
}

void VerticalMeter::setGainReductionMode(bool isGR)
{
    isGainReduction = isGR;
}

void VerticalMeter::setColour(juce::Colour colour)
{
    meterColour = colour;
}

void VerticalMeter::setLabel(const juce::String& label)
{
    labelText = label;
}

void VerticalMeter::setPeakHoldTime(int milliseconds)
{
    peakHoldTimeMs = milliseconds;
}

void VerticalMeter::resetPeak()
{
    peakDb = -120.0f;
    peakGr = 0.0f;
    peakHoldStartTime = 0;
    repaint();
}

float VerticalMeter::dbToY(float db) const
{
    float norm = (db + 60.0f) / 60.0f;
    norm = juce::jlimit(0.0f, 1.0f, norm);
    return getHeight() * (1.0f - norm);
}

juce::Colour VerticalMeter::getColourForLevel(float db) const
{
    if (db > -6.0f) return Colours::meterRed;
    if (db > -18.0f) return Colours::meterYellow;
    return meterColour;
}

//==============================================================================
// LufsMeterUI Implementation
//==============================================================================
LufsMeterUI::LufsMeterUI()
{
    startTimerHz(30);
}

void LufsMeterUI::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // 배경
    g.setColour(Colours::bgPanel);
    g.fillRoundedRectangle(bounds, 4.0f);
    
    // 테두리
    g.setColour(Colours::border);
    g.drawRoundedRectangle(bounds.reduced(0.5f), 4.0f, 1.0f);
    
    // "LUFS" 레이블
    g.setColour(Colours::textDim);
    g.setFont(10.0f);
    g.drawText("LUFS", bounds.removeFromTop(14.0f), juce::Justification::centred, false);
    
    // 현재 값
    juce::String valueText = juce::String(currentLufs, 1);
    g.setColour(Colours::textBright);
    g.setFont(14.0f);
    g.drawText(valueText, bounds.removeFromTop(18.0f), juce::Justification::centred, false);
    
    // 피크 값
    if (peakLufs > -100.0f) {
        juce::String peakText = "PK " + juce::String(peakLufs, 1);
        g.setColour(Colours::meterRed);
        g.setFont(10.0f);
        g.drawText(peakText, bounds, juce::Justification::centred, false);
    }
}

void LufsMeterUI::timerCallback()
{
    // 피크 홀드 타임아웃
    if (peakHoldStartTime > 0) {
        juce::int64 elapsed = juce::Time::currentTimeMillis() - peakHoldStartTime;
        if (elapsed > peakHoldTimeMs) {
            peakLufs = -120.0f;
            peakHoldStartTime = 0;
        }
    }
    repaint();
}

void LufsMeterUI::mouseDown(const juce::MouseEvent& /*event*/)
{
    resetPeak();
}

void LufsMeterUI::setValue(float lufsValue)
{
    currentLufs = lufsValue;
    if (currentLufs > peakLufs) {
        peakLufs = currentLufs;
        peakHoldStartTime = juce::Time::currentTimeMillis();
    }
}

void LufsMeterUI::resetPeak()
{
    peakLufs = -120.0f;
    peakHoldStartTime = 0;
    repaint();
}

//==============================================================================
// IOMetersPanel Implementation
//==============================================================================
IOMetersPanel::IOMetersPanel()
{
    addAndMakeVisible(inputMeter);
    addAndMakeVisible(outputMeter);
    addAndMakeVisible(lufsMeter);
    
    inputMeter.setColour(Colours::specInput);
    inputMeter.setLabel("IN");
    
    outputMeter.setColour(Colours::specOutput);
    outputMeter.setLabel("OUT");
    
    startTimerHz(30);
}

void IOMetersPanel::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    
    // 패널 배경
    CustomLookAndFeel::drawPanelBackground(g, bounds);
    
    // 타이틀
    g.setColour(Colours::textNormal);
    g.setFont(12.0f);
    g.drawText("LEVEL", bounds.removeFromTop(20), juce::Justification::centred, false);
}

void IOMetersPanel::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop(25);  // 타이틀 공간
    bounds.reduce(5, 5);
    
    int meterWidth = (bounds.getWidth() - 10) / 2;
    int meterHeight = bounds.getHeight() - 50;
    
    inputMeter.setBounds(bounds.getX(), bounds.getY(), meterWidth, meterHeight);
    outputMeter.setBounds(bounds.getX() + meterWidth + 10, bounds.getY(), meterWidth, meterHeight);
    
    lufsMeter.setBounds(bounds.getX(), bounds.getBottom() - 45, bounds.getWidth(), 40);
}

void IOMetersPanel::timerCallback()
{
    repaint();
}

void IOMetersPanel::setInputLevel(float db)
{
    inputMeter.setValue(db);
}

void IOMetersPanel::setOutputLevel(float db)
{
    outputMeter.setValue(db);
}

void IOMetersPanel::setLufsLevel(float lufs)
{
    lufsMeter.setValue(lufs);
}

void IOMetersPanel::resetAllPeaks()
{
    inputMeter.resetPeak();
    outputMeter.resetPeak();
    lufsMeter.resetPeak();
}

//==============================================================================
// GRMeter Implementation
//==============================================================================
GRMeter::GRMeter()
{
    startTimerHz(30);
}

void GRMeter::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // 배경
    g.setColour(Colours::bgSpectrum);
    g.fillRoundedRectangle(bounds, 2.0f);
    
    // 테두리
    g.setColour(Colours::border);
    g.drawRoundedRectangle(bounds.reduced(0.5f), 2.0f, 1.0f);
    
    // 미터 영역
    auto meterBounds = bounds.reduced(1.0f);
    
    // GR 미터: 위에서 아래로 채움 (양수 = 게인 감소)
    if (currentGr > 0.1f) {
        float grNorm = juce::jlimit(0.0f, 1.0f, currentGr / 24.0f);
        float filledHeight = grNorm * meterBounds.getHeight();
        
        juce::Rectangle<float> fillRect(meterBounds.getX(), meterBounds.getY(),
                                         meterBounds.getWidth(), filledHeight);
        
        // 그래디언트 채우기
        juce::ColourGradient gradient(meterColour.brighter(0.3f), fillRect.getX(), fillRect.getY(),
                                       meterColour.darker(0.1f), fillRect.getRight(), fillRect.getY(),
                                       false);
        g.setGradientFill(gradient);
        g.fillRect(fillRect);
    }
    
    // 피크 마커
    if (peakGr > 0.1f) {
        float peakNorm = juce::jlimit(0.0f, 1.0f, peakGr / 24.0f);
        float peakY = meterBounds.getY() + peakNorm * meterBounds.getHeight();
        
        g.setColour(Colours::meterRed);
        g.drawLine(meterBounds.getX() - 2, peakY, meterBounds.getRight() + 2, peakY, 2.0f);
    }
}

void GRMeter::timerCallback()
{
    // 피크 홀드 타임아웃
    if (peakHoldStartTime > 0) {
        juce::int64 elapsed = juce::Time::currentTimeMillis() - peakHoldStartTime;
        if (elapsed > peakHoldTimeMs) {
            peakGr = 0.0f;
            peakHoldStartTime = 0;
        }
    }
    repaint();
}

void GRMeter::mouseDown(const juce::MouseEvent& /*event*/)
{
    resetPeak();
}

void GRMeter::setValue(float grDb)
{
    currentGr = grDb;
    if (currentGr > peakGr) {
        peakGr = currentGr;
        peakHoldStartTime = juce::Time::currentTimeMillis();
    }
}

void GRMeter::setColour(juce::Colour colour)
{
    meterColour = colour;
}

void GRMeter::resetPeak()
{
    peakGr = 0.0f;
    peakHoldStartTime = 0;
    repaint();
}

} // namespace ELC4L
