//-------------------------------------------------------------------------------------------------------
// ELC4L - Elite 4-Band Compressor + Limiter
// Spectrum Analyzer Implementation - VST2 Style (Catmull-Rom Smoothing + Gold/Cyan Colors)
//-------------------------------------------------------------------------------------------------------

#include "SpectrumAnalyzer.h"

namespace ELC4L {

//==============================================================================
SpectrumAnalyzer::SpectrumAnalyzer()
{
    // 스펙트럼 초기화
    spectrumIn.fill(-90.0f);
    spectrumOut.fill(-90.0f);
    
    // 30fps로 업데이트
    startTimerHz(30);
}

//==============================================================================
void SpectrumAnalyzer::paint(juce::Graphics& g)
{
    drawBackground(g);
    drawGrid(g);
    
    // VST2와 동일: 입력(시안) 뒤에, 출력(골드/오렌지) 앞에
    drawSpectrumVST2Style(g, spectrumIn.data(), kSpecInput, kSpecFillIn, 0.25f);
    drawSpectrumVST2Style(g, spectrumOut.data(), kSpecOutput, kSpecFillOut, 0.35f);
    
    drawCrossoverLines(g);
    drawBandLabels(g);
}

//==============================================================================
void SpectrumAnalyzer::resized()
{
    // 그래프 영역 설정 (VST2와 유사한 여백)
    graphArea = getLocalBounds().reduced(38, 18);
    graphArea.removeFromTop(8);
    graphArea.removeFromBottom(22);
}

//==============================================================================
void SpectrumAnalyzer::timerCallback()
{
    repaint();
}

//==============================================================================
void SpectrumAnalyzer::setSpectrumData(const float* inputData, const float* outputData, int numBins)
{
    int copyCount = juce::jmin(numBins, (int)kDisplayBins);
    
    for (int i = 0; i < copyCount; ++i) {
        spectrumIn[i] = inputData[i];
        spectrumOut[i] = outputData[i];
    }
}

//==============================================================================
void SpectrumAnalyzer::setCrossoverFrequencies(float xover1, float xover2, float xover3)
{
    xoverFreq1 = xover1;
    xoverFreq2 = xover2;
    xoverFreq3 = xover3;
}

//==============================================================================
float SpectrumAnalyzer::frequencyToX(float freq) const
{
    float logMin = std::log10(20.0f);
    float logMax = std::log10(20000.0f);
    float logFreq = std::log10(juce::jlimit(20.0f, 20000.0f, freq));
    float normalized = (logFreq - logMin) / (logMax - logMin);
    return graphArea.getX() + normalized * graphArea.getWidth();
}

//==============================================================================
float SpectrumAnalyzer::xToFrequency(float x) const
{
    float normalized = (x - graphArea.getX()) / graphArea.getWidth();
    normalized = juce::jlimit(0.0f, 1.0f, normalized);
    float logMin = std::log10(20.0f);
    float logMax = std::log10(20000.0f);
    float logFreq = logMin + normalized * (logMax - logMin);
    return std::pow(10.0f, logFreq);
}

//==============================================================================
float SpectrumAnalyzer::dbToY(float db) const
{
    // VST2와 동일: -60dB to +6dB
    const float dbTop = 6.0f;
    const float dbBottom = -60.0f;
    float normalized = (dbTop - db) / (dbTop - dbBottom);
    normalized = juce::jlimit(0.0f, 1.0f, normalized);
    return graphArea.getY() + normalized * graphArea.getHeight();
}

//==============================================================================
// Catmull-Rom 스플라인 보간 (VST2와 동일)
float SpectrumAnalyzer::catmullRom(float p0, float p1, float p2, float p3, float t) const
{
    float t2 = t * t;
    float t3 = t2 * t;
    return 0.5f * ((2.0f * p1) +
                   (-p0 + p2) * t +
                   (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 +
                   (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3);
}

//==============================================================================
int SpectrumAnalyzer::hitTestCrossover(int x, int /*y*/) const
{
    const float hitRadius = 10.0f;
    
    float x1 = frequencyToX(xoverFreq1);
    float x2 = frequencyToX(xoverFreq2);
    float x3 = frequencyToX(xoverFreq3);
    
    if (std::abs(x - x1) < hitRadius) return 0;
    if (std::abs(x - x2) < hitRadius) return 1;
    if (std::abs(x - x3) < hitRadius) return 2;
    
    return -1;
}

//==============================================================================
void SpectrumAnalyzer::mouseDown(const juce::MouseEvent& event)
{
    activeCrossover = hitTestCrossover(event.x, event.y);
}

//==============================================================================
void SpectrumAnalyzer::mouseDrag(const juce::MouseEvent& event)
{
    if (activeCrossover >= 0 && activeCrossover <= 2) {
        float newFreq = xToFrequency((float)event.x);
        newFreq = juce::jlimit(kMinFreq, kMaxFreq, newFreq);
        
        switch (activeCrossover) {
            case 0:
                newFreq = juce::jmin(newFreq, xoverFreq2 * 0.9f);
                xoverFreq1 = newFreq;
                break;
            case 1:
                newFreq = juce::jmax(newFreq, xoverFreq1 * 1.1f);
                newFreq = juce::jmin(newFreq, xoverFreq3 * 0.9f);
                xoverFreq2 = newFreq;
                break;
            case 2:
                newFreq = juce::jmax(newFreq, xoverFreq2 * 1.1f);
                xoverFreq3 = newFreq;
                break;
        }
        
        if (onCrossoverChanged)
            onCrossoverChanged(activeCrossover, newFreq);
        
        repaint();
    }
}

//==============================================================================
void SpectrumAnalyzer::mouseUp(const juce::MouseEvent& /*event*/)
{
    activeCrossover = -1;
}

//==============================================================================
void SpectrumAnalyzer::mouseMove(const juce::MouseEvent& event)
{
    int newHovered = hitTestCrossover(event.x, event.y);
    if (newHovered != hoveredCrossover) {
        hoveredCrossover = newHovered;
        setMouseCursor(hoveredCrossover >= 0 ? 
            juce::MouseCursor::LeftRightResizeCursor : 
            juce::MouseCursor::NormalCursor);
        repaint();
    }
}

//==============================================================================
void SpectrumAnalyzer::drawBackground(juce::Graphics& g)
{
    // VST2 스타일 배경 (어두운 단색)
    g.setColour(kBgSpectrum);
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 3.0f);
    
    // 테두리
    g.setColour(kBorder);
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 3.0f, 1.0f);
}

//==============================================================================
void SpectrumAnalyzer::drawGrid(juce::Graphics& g)
{
    // 그리드 라인 색상 (VST2와 동일)
    g.setColour(kGrid);
    
    // 주파수 그리드 라인
    const float freqLines[] = { 50, 100, 200, 500, 1000, 2000, 5000, 10000 };
    
    for (float freq : freqLines) {
        float x = frequencyToX(freq);
        g.drawLine(x, (float)graphArea.getY(), x, (float)graphArea.getBottom(), 0.5f);
        
        // 주파수 레이블
        juce::String label;
        if (freq >= 1000)
            label = juce::String(freq / 1000.0f, 0) + "k";
        else
            label = juce::String((int)freq);
        
        g.setColour(kTextDim);
        g.setFont(9.0f);
        g.drawText(label, (int)x - 15, graphArea.getBottom() + 4, 30, 14, 
                   juce::Justification::centred, false);
    }
    
    // dB 그리드 라인 (VST2와 동일: 0, -10, -20, -30, -40, -50, -60)
    const float dbLines[] = { 0, -10, -20, -30, -40, -50, -60 };
    
    for (float db : dbLines) {
        float y = dbToY(db);
        g.setColour(kGrid);
        g.drawLine((float)graphArea.getX(), y, (float)graphArea.getRight(), y, 0.5f);
        
        // dB 레이블
        g.setColour(kTextDim);
        g.setFont(9.0f);
        g.drawText(juce::String((int)db), graphArea.getX() - 32, (int)y - 6, 28, 12,
                   juce::Justification::right, false);
    }
    
    // INPUT/OUTPUT 레이블 (VST2 스타일)
    g.setFont(10.0f);
    g.setColour(kSpecInput);
    g.drawText("INPUT", graphArea.getX() + 5, graphArea.getY() - 16, 50, 14,
               juce::Justification::left, false);
    
    g.setColour(kSpecOutput);
    g.drawText("OUTPUT", graphArea.getX() + 60, graphArea.getY() - 16, 60, 14,
               juce::Justification::left, false);
}

//==============================================================================
// VST2 스타일 스펙트럼 그리기 (Catmull-Rom 스무딩)
void SpectrumAnalyzer::drawSpectrumVST2Style(juce::Graphics& g, const float* data, 
                                              juce::Colour lineColour, juce::Colour fillColour, 
                                              float fillAlpha)
{
    if (graphArea.isEmpty()) return;
    
    const int numBins = kDisplayBins;
    const int interpFactor = 4;  // VST2와 동일한 스무딩 팩터
    const int outPoints = (numBins - 1) * interpFactor + 1;
    
    // dB 클램핑 (VST2와 동일)
    auto clampDb = [](float db) -> float {
        if (db < -60.0f) db = -60.0f;
        if (db > 6.0f) db = 6.0f;
        return (db + 60.0f) / 66.0f;  // 0..1 정규화
    };
    
    // Catmull-Rom 보간된 포인트 생성
    std::vector<juce::Point<float>> points;
    points.reserve(outPoints);
    
    for (int i = 0; i < numBins - 1; ++i) {
        int i0 = (i > 0) ? i - 1 : 0;
        int i1 = i;
        int i2 = i + 1;
        int i3 = (i < numBins - 2) ? i + 2 : numBins - 1;
        
        float xPos0 = (float)i0 / (float)(numBins - 1);
        float xPos1 = (float)i1 / (float)(numBins - 1);
        float xPos2 = (float)i2 / (float)(numBins - 1);
        float xPos3 = (float)i3 / (float)(numBins - 1);
        
        float y0 = clampDb(data[i0]);
        float y1 = clampDb(data[i1]);
        float y2 = clampDb(data[i2]);
        float y3 = clampDb(data[i3]);
        
        for (int j = 0; j < interpFactor; ++j) {
            float t = (float)j / (float)interpFactor;
            
            float xInterp = catmullRom(xPos0, xPos1, xPos2, xPos3, t);
            float yInterp = catmullRom(y0, y1, y2, y3, t);
            
            float px = graphArea.getX() + xInterp * graphArea.getWidth();
            float py = graphArea.getY() + (1.0f - yInterp) * graphArea.getHeight();
            
            points.push_back({ px, py });
        }
    }
    
    // 마지막 포인트
    float lastDb = clampDb(data[numBins - 1]);
    points.push_back({ 
        (float)graphArea.getRight(), 
        graphArea.getY() + (1.0f - lastDb) * graphArea.getHeight() 
    });
    
    // 경로 생성
    juce::Path spectrumPath;
    juce::Path fillPath;
    
    float baseY = (float)graphArea.getBottom();
    
    spectrumPath.startNewSubPath(points[0]);
    fillPath.startNewSubPath(points[0].x, baseY);
    fillPath.lineTo(points[0]);
    
    for (size_t i = 1; i < points.size(); ++i) {
        spectrumPath.lineTo(points[i]);
        fillPath.lineTo(points[i]);
    }
    
    fillPath.lineTo(points.back().x, baseY);
    fillPath.closeSubPath();
    
    // 필 그리기 (VST2 스타일 - 반투명)
    g.setColour(fillColour.withAlpha(fillAlpha));
    g.fillPath(fillPath);
    
    // 라인 그리기 (VST2 스타일)
    // Output은 더 두껍게 (2px), Input은 얇게 (1px)
    float lineWidth = (lineColour == kSpecOutput) ? 2.0f : 1.5f;
    g.setColour(lineColour);
    g.strokePath(spectrumPath, juce::PathStrokeType(lineWidth, juce::PathStrokeType::curved,
                                                     juce::PathStrokeType::rounded));
}

//==============================================================================
void SpectrumAnalyzer::drawCrossoverLines(juce::Graphics& g)
{
    const float frequencies[] = { xoverFreq1, xoverFreq2, xoverFreq3 };
    
    // VST2 스타일 크로스오버 색상 (골드 계열)
    for (int i = 0; i < 3; ++i) {
        float x = frequencyToX(frequencies[i]);
        
        bool isActive = (activeCrossover == i);
        bool isHovered = (hoveredCrossover == i);
        
        // 색상 결정 (VST2 스타일 - 골드)
        juce::Colour lineColour = kXoverNormal;
        if (isActive) {
            lineColour = kXoverActive;
        } else if (isHovered) {
            lineColour = kXoverHover;
        }
        
        // 수직 라인 (VST2 스타일)
        g.setColour(lineColour.withAlpha(0.7f));
        g.drawLine(x, (float)graphArea.getY(), x, (float)graphArea.getBottom(), 
                   isActive || isHovered ? 2.0f : 1.5f);
        
        // 삼각형 핸들 (VST2 스타일) - 하단
        float handleY = (float)graphArea.getBottom();
        float handleSize = isActive || isHovered ? 10.0f : 8.0f;
        
        juce::Path triangle;
        triangle.addTriangle(x - handleSize / 2, handleY,
                            x + handleSize / 2, handleY,
                            x, handleY - handleSize);
        
        g.setColour(lineColour);
        g.fillPath(triangle);
        
        // 주파수 레이블
        juce::String freqText;
        if (frequencies[i] >= 1000)
            freqText = juce::String(frequencies[i] / 1000.0f, 1) + "k";
        else
            freqText = juce::String((int)frequencies[i]);
        
        g.setColour(lineColour);
        g.setFont(10.0f);
        g.drawText(freqText, (int)x - 20, graphArea.getBottom() + 8, 40, 14,
                   juce::Justification::centred, false);
    }
}

//==============================================================================
void SpectrumAnalyzer::drawBandLabels(juce::Graphics& g)
{
    // VST2 스타일 밴드 레이블은 스펙트럼 상단에 표시하지 않음
    // (각 밴드 섹션에서 표시됨)
}

} // namespace ELC4L
