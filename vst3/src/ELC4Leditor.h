//-------------------------------------------------------------------------------------------------------
// ELC4L VST3 - VSTGUI Editor
// Premium Pro-Q Style UI with Spectrum Analyzer
//-------------------------------------------------------------------------------------------------------
#pragma once

#include "public.sdk/source/vst/vstguieditor.h"
#include "vstgui/plugin-bindings/vst3editor.h"
#include "vstgui/lib/cframe.h"
#include "vstgui/lib/cviewcontainer.h"
#include "vstgui/lib/controls/cbuttons.h"
#include "vstgui/lib/controls/ctextlabel.h"
#include "vstgui/lib/ccolor.h"
#include "ELC4Lids.h"

namespace ELC4L {

using namespace VSTGUI;

//-------------------------------------------------------------------------------------------------------
// Editor dimensions
//-------------------------------------------------------------------------------------------------------
constexpr CCoord kEditorWidth = 980;
constexpr CCoord kEditorHeight = 700;

//-------------------------------------------------------------------------------------------------------
// Premium Dark & Gold Color Theme
//-------------------------------------------------------------------------------------------------------
const CColor kColorBgDark(24, 24, 28);
const CColor kColorBgPanel(32, 32, 38);
const CColor kColorBgSpectrum(16, 16, 20);
const CColor kColorBorder(55, 55, 65);
const CColor kColorBorderLight(75, 75, 85);

// Gold accent
const CColor kColorGoldPrimary(255, 200, 60);
const CColor kColorGoldBright(255, 215, 100);
const CColor kColorGoldDim(180, 140, 40);

// Text colors
const CColor kColorTextBright(240, 240, 245);
const CColor kColorTextNormal(190, 190, 200);
const CColor kColorTextDim(110, 110, 125);

// Band colors
const CColor kColorBandLow(200, 100, 100);
const CColor kColorBandLowMid(100, 180, 100);
const CColor kColorBandHiMid(100, 150, 220);
const CColor kColorBandHigh(200, 180, 100);

// Spectrum colors
const CColor kColorSpecInput(80, 180, 220);
const CColor kColorSpecOutput(255, 180, 80);
const CColor kColorSpecFillIn(30, 70, 90, 128);
const CColor kColorSpecFillOut(80, 60, 30, 128);

// Limiter
const CColor kColorLimiter(255, 130, 70);

// Meter colors
const CColor kColorMeterGreen(80, 200, 120);
const CColor kColorMeterYellow(255, 200, 80);
const CColor kColorMeterRed(255, 90, 90);

// Button colors
const CColor kColorBtnInactive(50, 50, 58);
const CColor kColorBtnMute(220, 60, 60);
const CColor kColorBtnSolo(255, 220, 60);
const CColor kColorBtnDelta(60, 200, 220);

//-------------------------------------------------------------------------------------------------------
// Layout constants
//-------------------------------------------------------------------------------------------------------
namespace Layout {
    constexpr CCoord Padding = 18;
    constexpr CCoord FontMargin = 20;
    
    // Spectrum
    constexpr CCoord SpectrumX = 15;
    constexpr CCoord SpectrumY = 55;
    constexpr CCoord SpectrumW = 860;
    constexpr CCoord SpectrumH = 260;
    
    // Band section
    constexpr CCoord BandSectionY = SpectrumY + SpectrumH + FontMargin + 15;
    constexpr CCoord BandWidth = 140;
    constexpr CCoord BandStartX = 20;
    constexpr CCoord BandTitleH = 26;
    
    // GR Meter
    constexpr CCoord GRMeterOffsetX = 8;
    constexpr CCoord GRMeterW = 20;
    constexpr CCoord GRMeterH = 120;
    constexpr CCoord GRMeterOffsetY = BandTitleH + FontMargin;
    
    // Faders
    constexpr CCoord FaderOffsetX = GRMeterOffsetX + GRMeterW + 12;
    constexpr CCoord FaderWidth = 24;
    constexpr CCoord FaderHeight = 120;
    constexpr CCoord FaderGap = 8;
    
    // Buttons
    constexpr CCoord BtnOffsetY = GRMeterOffsetY + GRMeterH + FontMargin + 5;
    constexpr CCoord MSBtnW = 20;
    constexpr CCoord MSBtnH = 20;
    constexpr CCoord DeltaBtnOffsetY = BtnOffsetY + MSBtnH + 8;
    constexpr CCoord DeltaBtnW = 43;
    constexpr CCoord DeltaBtnH = 16;
    constexpr CCoord BypassBtnOffsetY = DeltaBtnOffsetY + DeltaBtnH + 8;
    
    // Limiter
    constexpr CCoord LimiterX = BandStartX + BandWidth * 4 + 25;
    constexpr CCoord LimiterW = 130;
    constexpr CCoord LimBypassOffsetY = GRMeterOffsetY + GRMeterH + FontMargin + 30;
    
    // IN/OUT
    constexpr CCoord IOSectionX = 875;
    constexpr CCoord IOMeterW = 24;
    constexpr CCoord IOMeterH = 160;
}

//-------------------------------------------------------------------------------------------------------
// Custom Spectrum Analyzer View
//-------------------------------------------------------------------------------------------------------
class CSpectrumView : public CView {
public:
    CSpectrumView(const CRect& size) : CView(size) {
        setWantsFocus(false);
        memset(spectrumIn, 0, sizeof(spectrumIn));
        memset(spectrumOut, 0, sizeof(spectrumOut));
        for (int i = 0; i < 3; ++i) {
            crossoverFreq[i] = 200.0f * (i + 1);
        }
    }
    
    void setSpectrumData(const float* inData, const float* outData, int numBins) {
        if (numBins > 512) numBins = 512;
        memcpy(spectrumIn, inData, numBins * sizeof(float));
        memcpy(spectrumOut, outData, numBins * sizeof(float));
        invalid();
    }
    
    void setCrossoverFrequencies(float f1, float f2, float f3) {
        crossoverFreq[0] = f1;
        crossoverFreq[1] = f2;
        crossoverFreq[2] = f3;
        invalid();
    }
    
    void draw(CDrawContext* context) override {
        CRect r = getViewSize();
        
        // Background
        context->setFillColor(kColorBgSpectrum);
        context->drawRect(r, kDrawFilled);
        
        // Border
        context->setFrameColor(kColorBorder);
        context->setLineWidth(1);
        context->drawRect(r, kDrawStroked);
        
        // Grid
        drawGrid(context, r);
        
        // Spectrum curves
        drawSpectrumCurve(context, r, spectrumIn, kColorSpecInput, kColorSpecFillIn);
        drawSpectrumCurve(context, r, spectrumOut, kColorSpecOutput, kColorSpecFillOut);
        
        // Crossover lines
        drawCrossoverLines(context, r);
    }
    
private:
    float spectrumIn[512];
    float spectrumOut[512];
    float crossoverFreq[3];
    
    float freqToX(float freq, CCoord left, CCoord width) const {
        constexpr float kMinFreq = 20.0f;
        constexpr float kMaxFreq = 20000.0f;
        float logMin = log10f(kMinFreq);
        float logMax = log10f(kMaxFreq);
        float logFreq = log10f(freq);
        float normalized = (logFreq - logMin) / (logMax - logMin);
        return left + normalized * width;
    }
    
    void drawGrid(CDrawContext* context, const CRect& r) {
        context->setFrameColor(CColor(45, 45, 55));
        context->setLineWidth(1);
        
        // Frequency lines
        float freqs[] = { 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000 };
        for (float f : freqs) {
            CCoord x = freqToX(f, r.left + 35, r.getWidth() - 50);
            context->drawLine(CPoint(x, r.top + 10), CPoint(x, r.bottom - 10));
        }
        
        // dB lines
        for (int db = -60; db <= 0; db += 12) {
            float normalized = (db + 60.0f) / 60.0f;
            CCoord y = r.bottom - 10 - normalized * (r.getHeight() - 20);
            context->drawLine(CPoint(r.left + 35, y), CPoint(r.right - 15, y));
        }
    }
    
    void drawSpectrumCurve(CDrawContext* context, const CRect& r, const float* data, const CColor& lineColor, const CColor& fillColor) {
        CCoord x0 = r.left + 35;
        CCoord y0 = r.bottom - 10;
        CCoord w = r.getWidth() - 50;
        CCoord h = r.getHeight() - 20;
        
        auto path = owned(context->createGraphicsPath());
        if (!path) return;
        
        path->beginSubpath(CPoint(x0, y0));
        
        for (int i = 0; i < 128; ++i) {
            float db = data[i];
            if (db < -90.0f) db = -90.0f;
            if (db > 0.0f) db = 0.0f;
            
            float normalized = (db + 90.0f) / 90.0f;
            CCoord x = x0 + (i / 127.0f) * w;
            CCoord y = y0 - normalized * h;
            path->addLine(CPoint(x, y));
        }
        
        path->addLine(CPoint(x0 + w, y0));
        path->closeSubpath();
        
        // Fill
        context->setFillColor(fillColor);
        context->drawGraphicsPath(path, CDrawContext::kPathFilled);
        
        // Stroke
        context->setFrameColor(lineColor);
        context->setLineWidth(2);
        context->drawGraphicsPath(path, CDrawContext::kPathStroked);
    }
    
    void drawCrossoverLines(CDrawContext* context, const CRect& r) {
        CColor xoverColors[] = { kColorBandLow, kColorBandLowMid, kColorBandHiMid };
        
        for (int i = 0; i < 3; ++i) {
            CCoord x = freqToX(crossoverFreq[i], r.left + 35, r.getWidth() - 50);
            
            context->setFrameColor(xoverColors[i]);
            context->setLineWidth(2);
            context->drawLine(CPoint(x, r.top + 20), CPoint(x, r.bottom - 10));
            
            // Frequency label
            char label[32];
            if (crossoverFreq[i] >= 1000.0f) {
                snprintf(label, sizeof(label), "%.1fk", crossoverFreq[i] / 1000.0f);
            } else {
                snprintf(label, sizeof(label), "%.0f", crossoverFreq[i]);
            }
            
            // Draw handle triangle at top
            CGraphicsPath* triangle = context->createGraphicsPath();
            if (triangle) {
                triangle->beginSubpath(CPoint(x - 6, r.top + 5));
                triangle->addLine(CPoint(x + 6, r.top + 5));
                triangle->addLine(CPoint(x, r.top + 18));
                triangle->closeSubpath();
                
                context->setFillColor(xoverColors[i]);
                context->drawGraphicsPath(triangle, CDrawContext::kPathFilled);
                triangle->forget();
            }
        }
    }
    
    CLASS_METHODS(CSpectrumView, CView)
};

//-------------------------------------------------------------------------------------------------------
// Custom Vertical Meter View (for GR and IN/OUT)
//-------------------------------------------------------------------------------------------------------
class CVerticalMeter : public CView {
public:
    CVerticalMeter(const CRect& size, const CColor& color = kColorMeterGreen)
        : CView(size), meterColor(color), value(0.0f), isGainReduction(false) {}
    
    void setValue(float v) {
        value = v;
        invalid();
    }
    
    void setGainReductionMode(bool gr) {
        isGainReduction = gr;
    }
    
    void draw(CDrawContext* context) override {
        CRect r = getViewSize();
        
        // Background
        context->setFillColor(CColor(20, 20, 24));
        context->drawRect(r, kDrawFilled);
        
        // Border
        context->setFrameColor(kColorBorder);
        context->setLineWidth(1);
        context->drawRect(r, kDrawStroked);
        
        // Meter fill
        CCoord h = r.getHeight() - 4;
        CCoord fillH;
        
        if (isGainReduction) {
            // GR meter: value is positive dB of reduction, fills from top
            float normalized = value / 20.0f;  // 0-20 dB range
            if (normalized > 1.0f) normalized = 1.0f;
            fillH = normalized * h;
            
            CRect fillRect(r.left + 2, r.top + 2, r.right - 2, r.top + 2 + fillH);
            context->setFillColor(kColorMeterRed);
            context->drawRect(fillRect, kDrawFilled);
        } else {
            // Level meter: value is dB, fills from bottom
            float normalized = (value + 60.0f) / 60.0f;  // -60 to 0 dB
            if (normalized < 0.0f) normalized = 0.0f;
            if (normalized > 1.0f) normalized = 1.0f;
            fillH = normalized * h;
            
            CRect fillRect(r.left + 2, r.bottom - 2 - fillH, r.right - 2, r.bottom - 2);
            
            // Gradient coloring based on level
            CColor fillColor = kColorMeterGreen;
            if (normalized > 0.85f) fillColor = kColorMeterRed;
            else if (normalized > 0.7f) fillColor = kColorMeterYellow;
            
            context->setFillColor(fillColor);
            context->drawRect(fillRect, kDrawFilled);
        }
    }
    
private:
    CColor meterColor;
    float value;
    bool isGainReduction;
    
    CLASS_METHODS(CVerticalMeter, CView)
};

//-------------------------------------------------------------------------------------------------------
// Custom Fader View (vertical slider with track)
//-------------------------------------------------------------------------------------------------------
class CFaderView : public CControl {
public:
    CFaderView(const CRect& size, IControlListener* listener, int32_t tag, const CColor& color)
        : CControl(size, listener, tag)
        , faderColor(color)
    {
    }
    
    void draw(CDrawContext* context) override {
        CRect r = getViewSize();
        
        // Track background
        CCoord trackW = 6;
        CCoord trackX = r.left + (r.getWidth() - trackW) / 2;
        CRect trackRect(trackX, r.top + 8, trackX + trackW, r.bottom - 8);
        
        context->setFillColor(CColor(30, 30, 35));
        context->drawRect(trackRect, kDrawFilled);
        context->setFrameColor(kColorBorder);
        context->drawRect(trackRect, kDrawStroked);
        
        // Fill based on value
        float v = getValueNormalized();
        CCoord fillH = v * (trackRect.getHeight() - 2);
        CRect fillRect(trackRect.left + 1, trackRect.bottom - 1 - fillH, trackRect.right - 1, trackRect.bottom - 1);
        context->setFillColor(faderColor);
        context->drawRect(fillRect, kDrawFilled);
        
        // Handle
        CCoord handleH = 16;
        CCoord handleY = trackRect.bottom - fillH - handleH / 2;
        CRect handleRect(r.left + 2, handleY, r.right - 2, handleY + handleH);
        
        context->setFillColor(CColor(60, 60, 68));
        context->drawRect(handleRect, kDrawFilled);
        context->setFrameColor(faderColor);
        context->setLineWidth(2);
        context->drawRect(handleRect, kDrawStroked);
        
        // Center line on handle
        context->setFrameColor(kColorTextDim);
        context->setLineWidth(1);
        CCoord centerY = handleY + handleH / 2;
        context->drawLine(CPoint(r.left + 5, centerY), CPoint(r.right - 5, centerY));
    }
    
    CMouseEventResult onMouseDown(CPoint& where, const CButtonState& buttons) override {
        if (buttons.isLeftButton()) {
            startValue = getValue();
            startY = where.y;
            return kMouseEventHandled;
        }
        return kMouseEventNotHandled;
    }
    
    CMouseEventResult onMouseMoved(CPoint& where, const CButtonState& buttons) override {
        if (buttons.isLeftButton()) {
            CRect r = getViewSize();
            float delta = (startY - where.y) / r.getHeight();
            float newValue = startValue + delta;
            newValue = std::max(0.0f, std::min(1.0f, newValue));
            setValueNormalized(newValue);
            valueChanged();
            invalid();
            return kMouseEventHandled;
        }
        return kMouseEventNotHandled;
    }
    
private:
    CColor faderColor;
    float startValue = 0.0f;
    CCoord startY = 0.0;
    
    CLASS_METHODS(CFaderView, CControl)
};

//-------------------------------------------------------------------------------------------------------
// Custom Toggle Button (for M/S/Delta/Bypass)
//-------------------------------------------------------------------------------------------------------
class CToggleButton : public COnOffButton {
public:
    CToggleButton(const CRect& size, IControlListener* listener, int32_t tag,
                  const UTF8String& label, const CColor& activeColor)
        : COnOffButton(size, listener, tag, nullptr)
        , buttonLabel(label)
        , activeColor(activeColor)
    {}
    
    void draw(CDrawContext* context) override {
        CRect r = getViewSize();
        bool isOn = getValue() > 0.5f;
        
        // Background
        context->setFillColor(isOn ? activeColor : kColorBtnInactive);
        context->drawRect(r, kDrawFilled);
        
        // Border
        context->setFrameColor(isOn ? activeColor : kColorBorder);
        context->setLineWidth(1);
        context->drawRect(r, kDrawStroked);
        
        // Label
        context->setFontColor(isOn ? CColor(0, 0, 0) : kColorTextDim);
        context->setFont(kNormalFontSmall);
        context->drawString(buttonLabel.data(), r, kCenterText);
    }
    
private:
    UTF8String buttonLabel;
    CColor activeColor;
    
    CLASS_METHODS(CToggleButton, COnOffButton)
};

//-------------------------------------------------------------------------------------------------------
// ELC4L Editor Controller with UI
//-------------------------------------------------------------------------------------------------------
class ELC4LEditor : public VST3Editor {
public:
    ELC4LEditor(Steinberg::Vst::EditController* controller, UTF8StringPtr templateName, UTF8StringPtr xmlFile = nullptr);
    ~ELC4LEditor() override;
    
    // Create the UI programmatically
    bool PLUGIN_API open(void* parent, const PlatformType& platformType = PlatformType::kDefaultNative) override;
    void PLUGIN_API close() override;
    
    // Idle for meter updates
    void idle();
    // IControlListener override (forward to VST3Editor default)
    void valueChanged(CControl* pControl) override;
    // Logging helper
    static void logMessage(const char* fmt, ...);
    
protected:
    void createUI(CFrame* frame);
    void createSpectrumView(CViewContainer* container);
    void createBandSection(CViewContainer* container, int bandIndex);
    void createLimiterSection(CViewContainer* container);
    void createIOMeters(CViewContainer* container);
    
    // Spectrum data
    CSpectrumView* spectrumView {nullptr};
    
    // Initialization state for UI
    bool uiInitialized {false};
    // Meters
    CVerticalMeter* grMeters[4] { nullptr, nullptr, nullptr, nullptr };
    CVerticalMeter* limiterGrMeter { nullptr };
    CVerticalMeter* inputMeter { nullptr };
    CVerticalMeter* outputMeter { nullptr };
    
    // Faders
    CFaderView* faders[14] { nullptr };
};

} // namespace ELC4L
