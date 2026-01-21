//-------------------------------------------------------------------------------------------------------
// ELC4L VST3 - VSTGUI Editor Implementation
// Premium Pro-Q Style UI with Spectrum Analyzer
//-------------------------------------------------------------------------------------------------------

#include "ELC4Leditor.h"
#include "ELC4Lcontroller.h"
#include "vstgui/lib/cfont.h"
#include <cstdarg>
#include <cstdio>
#ifdef _WIN32
#include <Windows.h>
#endif

namespace ELC4L {

using namespace VSTGUI;

// Simple logging helper: sends to OutputDebugString on Windows, stderr otherwise
void ELC4LEditor::logMessage(const char* fmt, ...) {
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
#ifdef _WIN32
    // Output to debug console
    OutputDebugStringA("ELC4L: ");
    OutputDebugStringA(buf);
    OutputDebugStringA("\n");
    // Also append to output file for easier capture
    FILE* f = fopen("..\\..\\output\\plugin_ui_log_vst3.txt", "a");
    if (f) {
        fprintf(f, "ELC4L: %s\n", buf);
        fclose(f);
    }
#else
    fprintf(stderr, "ELC4L: %s\n", buf);
#endif
}

//-------------------------------------------------------------------------------------------------------
// ELC4LEditor Implementation
//-------------------------------------------------------------------------------------------------------
ELC4LEditor::ELC4LEditor(Steinberg::Vst::EditController* controller, UTF8StringPtr templateName, UTF8StringPtr xmlFile)
    : VST3Editor(controller, templateName, xmlFile)
{
}

ELC4LEditor::~ELC4LEditor() {}

bool PLUGIN_API ELC4LEditor::open(void* parent, const PlatformType& platformType) {
    logMessage("ELC4LEditor::open() - entry");

    if (!VST3Editor::open(parent, platformType)) {
        logMessage("VST3Editor::open() failed");
        return false;
    }

    CFrame* f = getFrame();
    if (!f) {
        logMessage("getFrame() returned nullptr");
        return false;
    }

    f->setBackgroundColor(kColorBgDark);

    try {
        createUI(f);
        uiInitialized = true;
        logMessage("UI created successfully");
    } catch (const std::exception& e) {
        logMessage("Exception while creating UI: %s", e.what());
        // Fallback: simple header-only UI to avoid host crash
        CRect containerRect(0, 0, kEditorWidth, kEditorHeight);
        CViewContainer* container = new CViewContainer(containerRect);
        container->setBackgroundColor(kColorBgDark);
        CRect headerRect(20, 20, 260, 60);
        CTextLabel* headerLabel = new CTextLabel(headerRect, "ELC4L (fallback UI)");
        headerLabel->setBackColor(CColor(0, 0, 0, 0));
        headerLabel->setFontColor(kColorGoldPrimary);
        headerLabel->setFont(kNormalFontVeryBig);
        headerLabel->setHoriAlign(kLeftText);
        container->addView(headerLabel);
        f->addView(container);
        uiInitialized = false;
    } catch (...) {
        logMessage("Unknown exception while creating UI");
        CRect containerRect(0, 0, kEditorWidth, kEditorHeight);
        CViewContainer* container = new CViewContainer(containerRect);
        container->setBackgroundColor(kColorBgDark);
        CRect headerRect(20, 20, 260, 60);
        CTextLabel* headerLabel = new CTextLabel(headerRect, "ELC4L (fallback UI)");
        headerLabel->setBackColor(CColor(0, 0, 0, 0));
        headerLabel->setFontColor(kColorGoldPrimary);
        headerLabel->setFont(kNormalFontVeryBig);
        headerLabel->setHoriAlign(kLeftText);
        container->addView(headerLabel);
        f->addView(container);
        uiInitialized = false;
    }

    logMessage("ELC4LEditor::open() - exit (uiInitialized=%d)", uiInitialized);
    return true;
}

void PLUGIN_API ELC4LEditor::close() {
    logMessage("ELC4LEditor::close() - entry");
    spectrumView = nullptr;
    for (int i = 0; i < 4; ++i) grMeters[i] = nullptr;
    limiterGrMeter = nullptr;
    inputMeter = nullptr;
    outputMeter = nullptr;
    for (int i = 0; i < 14; ++i) faders[i] = nullptr;
    uiInitialized = false;
    VST3Editor::close();
    logMessage("ELC4LEditor::close() - exit");
}

//-------------------------------------------------------------------------------------------------------
// IControlListener callbacks
//-------------------------------------------------------------------------------------------------------
void ELC4LEditor::valueChanged(CControl* pControl) {
    if (pControl) {
        int32_t tag = pControl->getTag();
        logMessage("valueChanged() - control tag=%d, ptr=%p", tag, (void*)pControl);
    } else {
        logMessage("valueChanged() - control is nullptr");
    }
    VST3Editor::valueChanged(pControl);
}

void ELC4LEditor::idle() {
    // Update meters from processor
    // In a real implementation, this would get data from the processor via messaging
}

//-------------------------------------------------------------------------------------------------------
// Create UI
//-------------------------------------------------------------------------------------------------------
void ELC4LEditor::createUI(CFrame* frame) {
    logMessage("createUI() - building UI");
    // Minimal safe UI to verify stability
    CRect containerRect(0, 0, kEditorWidth, kEditorHeight);
    CViewContainer* container = new CViewContainer(containerRect);
    container->setBackgroundColor(kColorBgDark);

    CRect headerRect(20, 20, 260, 60);
    CTextLabel* headerLabel = new CTextLabel(headerRect, "ELC4L (UI safe test)");
    headerLabel->setBackColor(CColor(0, 0, 0, 0));
    headerLabel->setFontColor(kColorGoldPrimary);
    headerLabel->setFont(kNormalFontVeryBig);
    headerLabel->setHoriAlign(kLeftText);
    container->addView(headerLabel);

    // Attempt to add richer parts guarded by try/catch to avoid crashes
    try {
        createSpectrumView(container);
        for (int i = 0; i < 4; ++i) createBandSection(container, i);
        createLimiterSection(container);
        createIOMeters(container);
        logMessage("createUI() - added full UI components");
    } catch (const std::exception& e) {
        logMessage("createUI() - exception while adding components: %s", e.what());
        // keep only the minimal header
    } catch (...) {
        logMessage("createUI() - unknown exception while adding components");
    }

    frame->addView(container);
    logMessage("createUI() - done");
}

//-------------------------------------------------------------------------------------------------------
// Spectrum View
//-------------------------------------------------------------------------------------------------------
void ELC4LEditor::createSpectrumView(CViewContainer* container) {
    using namespace Layout;
    
    CRect specRect(SpectrumX, SpectrumY, SpectrumX + SpectrumW, SpectrumY + SpectrumH);
    spectrumView = new CSpectrumView(specRect);
    container->addView(spectrumView);
    
    // Initialize with test data
    float testIn[128], testOut[128];
    for (int i = 0; i < 128; ++i) {
        float freq = 20.0f * powf(1000.0f, i / 127.0f);
        testIn[i] = -30.0f - 20.0f * sinf(freq * 0.01f);
        testOut[i] = testIn[i] + 6.0f;
    }
    spectrumView->setSpectrumData(testIn, testOut, 128);
    spectrumView->setCrossoverFrequencies(120.0f, 800.0f, 4000.0f);
}

//-------------------------------------------------------------------------------------------------------
// Band Section
//-------------------------------------------------------------------------------------------------------
void ELC4LEditor::createBandSection(CViewContainer* container, int bandIndex) {
    using namespace Layout;
    
    CCoord bandX = BandStartX + bandIndex * BandWidth;
    CCoord bandY = BandSectionY;
    
    CColor bandColors[] = { kColorBandLow, kColorBandLowMid, kColorBandHiMid, kColorBandHigh };
    const char* bandNames[] = { "LOW", "LO-MID", "HI-MID", "HIGH" };
    CColor bandColor = bandColors[bandIndex];
    
    // Band container
    CRect bandRect(bandX, bandY, bandX + BandWidth, bandY + 300);
    CViewContainer* bandContainer = new CViewContainer(bandRect);
    bandContainer->setBackgroundColor(CColor(0, 0, 0, 0));
    
    // Title bar
    CRect titleRect(0, 0, BandWidth, BandTitleH);
    CViewContainer* titleBg = new CViewContainer(titleRect);
    titleBg->setBackgroundColor(bandColor);
    bandContainer->addView(titleBg);
    
    CTextLabel* titleLabel = new CTextLabel(titleRect, bandNames[bandIndex]);
    titleLabel->setBackColor(CColor(0, 0, 0, 0));
    titleLabel->setFontColor(CColor(0, 0, 0));
    titleLabel->setFont(kNormalFontSmall);
    titleLabel->setHoriAlign(kCenterText);
    bandContainer->addView(titleLabel);
    
    // GR Meter
    CRect grRect(GRMeterOffsetX, GRMeterOffsetY, GRMeterOffsetX + GRMeterW, GRMeterOffsetY + GRMeterH);
    grMeters[bandIndex] = new CVerticalMeter(grRect, kColorMeterRed);
    grMeters[bandIndex]->setGainReductionMode(true);
    grMeters[bandIndex]->setValue(5.0f);  // Test value
    bandContainer->addView(grMeters[bandIndex]);
    
    // GR label
    CRect grLabelRect(GRMeterOffsetX - 2, GRMeterOffsetY + GRMeterH + 2, GRMeterOffsetX + GRMeterW + 2, GRMeterOffsetY + GRMeterH + 16);
    CTextLabel* grLabel = new CTextLabel(grLabelRect, "GR");
    grLabel->setBackColor(CColor(0, 0, 0, 0));
    grLabel->setFontColor(kColorTextDim);
    grLabel->setFont(kNormalFontVerySmall);
    grLabel->setHoriAlign(kCenterText);
    bandContainer->addView(grLabel);
    
    // Threshold fader
    CRect thrRect(FaderOffsetX, GRMeterOffsetY, FaderOffsetX + FaderWidth, GRMeterOffsetY + FaderHeight);
    faders[bandIndex] = new CFaderView(thrRect, this, kParamBand1Thresh + bandIndex, bandColor);
    faders[bandIndex]->setDefaultValue(0.75f);
    bandContainer->addView(faders[bandIndex]);
    
    // THR label
    CRect thrLabelRect(FaderOffsetX, GRMeterOffsetY + FaderHeight + 2, FaderOffsetX + FaderWidth, GRMeterOffsetY + FaderHeight + 16);
    CTextLabel* thrLabel = new CTextLabel(thrLabelRect, "THR");
    thrLabel->setBackColor(CColor(0, 0, 0, 0));
    thrLabel->setFontColor(kColorTextDim);
    thrLabel->setFont(kNormalFontVerySmall);
    thrLabel->setHoriAlign(kCenterText);
    bandContainer->addView(thrLabel);
    
    // Makeup fader
    CCoord mkX = FaderOffsetX + FaderWidth + FaderGap;
    CRect mkRect(mkX, GRMeterOffsetY, mkX + FaderWidth, GRMeterOffsetY + FaderHeight);
    faders[4 + bandIndex] = new CFaderView(mkRect, this, kParamBand1Makeup + bandIndex, bandColor);
    faders[4 + bandIndex]->setDefaultValue(0.5f);
    bandContainer->addView(faders[4 + bandIndex]);
    
    // MK label
    CRect mkLabelRect(mkX, GRMeterOffsetY + FaderHeight + 2, mkX + FaderWidth, GRMeterOffsetY + FaderHeight + 16);
    CTextLabel* mkLabel = new CTextLabel(mkLabelRect, "MK");
    mkLabel->setBackColor(CColor(0, 0, 0, 0));
    mkLabel->setFontColor(kColorTextDim);
    mkLabel->setFont(kNormalFontVerySmall);
    mkLabel->setHoriAlign(kCenterText);
    bandContainer->addView(mkLabel);
    
    // M button
    CRect mRect(GRMeterOffsetX, BtnOffsetY, GRMeterOffsetX + MSBtnW, BtnOffsetY + MSBtnH);
    CToggleButton* mBtn = new CToggleButton(mRect, this, 100 + bandIndex * 10, "M", kColorBtnMute);
    bandContainer->addView(mBtn);
    
    // S button
    CRect sRect(GRMeterOffsetX + MSBtnW + 3, BtnOffsetY, GRMeterOffsetX + MSBtnW * 2 + 3, BtnOffsetY + MSBtnH);
    CToggleButton* sBtn = new CToggleButton(sRect, this, 101 + bandIndex * 10, "S", kColorBtnSolo);
    bandContainer->addView(sBtn);
    
    // Delta button
    CRect deltaRect(GRMeterOffsetX, DeltaBtnOffsetY, GRMeterOffsetX + DeltaBtnW, DeltaBtnOffsetY + DeltaBtnH);
    CToggleButton* deltaBtn = new CToggleButton(deltaRect, this, 102 + bandIndex * 10, "DELTA", kColorBtnDelta);
    bandContainer->addView(deltaBtn);
    
    // Bypass button
    CRect bypassRect(GRMeterOffsetX, BypassBtnOffsetY, GRMeterOffsetX + DeltaBtnW, BypassBtnOffsetY + DeltaBtnH);
    CToggleButton* bypassBtn = new CToggleButton(bypassRect, this, 103 + bandIndex * 10, "BYPASS", kColorTextDim);
    bandContainer->addView(bypassBtn);
    
    container->addView(bandContainer);
}

//-------------------------------------------------------------------------------------------------------
// Limiter Section
//-------------------------------------------------------------------------------------------------------
void ELC4LEditor::createLimiterSection(CViewContainer* container) {
    using namespace Layout;
    
    CCoord limY = BandSectionY;
    
    // Limiter container
    CRect limRect(LimiterX, limY, LimiterX + LimiterW, limY + 300);
    CViewContainer* limContainer = new CViewContainer(limRect);
    limContainer->setBackgroundColor(CColor(0, 0, 0, 0));
    
    // Title bar
    CRect titleRect(0, 0, LimiterW, BandTitleH);
    CViewContainer* titleBg = new CViewContainer(titleRect);
    titleBg->setBackgroundColor(kColorLimiter);
    limContainer->addView(titleBg);
    
    CTextLabel* titleLabel = new CTextLabel(titleRect, "LIMITER");
    titleLabel->setBackColor(CColor(0, 0, 0, 0));
    titleLabel->setFontColor(CColor(0, 0, 0));
    titleLabel->setFont(kNormalFontSmall);
    titleLabel->setHoriAlign(kCenterText);
    limContainer->addView(titleLabel);
    
    // GR Meter
    CRect grRect(GRMeterOffsetX, GRMeterOffsetY, GRMeterOffsetX + GRMeterW, GRMeterOffsetY + GRMeterH);
    limiterGrMeter = new CVerticalMeter(grRect, kColorMeterRed);
    limiterGrMeter->setGainReductionMode(true);
    limiterGrMeter->setValue(2.0f);
    limContainer->addView(limiterGrMeter);
    
    // Threshold fader
    CCoord fX = FaderOffsetX - 5;
    CRect thrRect(fX, GRMeterOffsetY, fX + FaderWidth, GRMeterOffsetY + FaderHeight);
    faders[11] = new CFaderView(thrRect, this, kParamLimiterThresh, kColorLimiter);
    faders[11]->setDefaultValue(0.75f);
    limContainer->addView(faders[11]);
    
    CRect thrLabelRect(fX, GRMeterOffsetY + FaderHeight + 2, fX + FaderWidth, GRMeterOffsetY + FaderHeight + 16);
    CTextLabel* thrLabel = new CTextLabel(thrLabelRect, "THR");
    thrLabel->setBackColor(CColor(0, 0, 0, 0));
    thrLabel->setFontColor(kColorTextDim);
    thrLabel->setFont(kNormalFontVerySmall);
    thrLabel->setHoriAlign(kCenterText);
    limContainer->addView(thrLabel);
    
    // Ceiling fader
    CCoord cX = fX + FaderWidth + FaderGap;
    CRect ceilRect(cX, GRMeterOffsetY, cX + FaderWidth, GRMeterOffsetY + FaderHeight);
    faders[12] = new CFaderView(ceilRect, this, kParamLimiterCeiling, kColorLimiter);
    faders[12]->setDefaultValue(0.9583f);
    limContainer->addView(faders[12]);
    
    CRect ceilLabelRect(cX, GRMeterOffsetY + FaderHeight + 2, cX + FaderWidth, GRMeterOffsetY + FaderHeight + 16);
    CTextLabel* ceilLabel = new CTextLabel(ceilLabelRect, "CEIL");
    ceilLabel->setBackColor(CColor(0, 0, 0, 0));
    ceilLabel->setFontColor(kColorTextDim);
    ceilLabel->setFont(kNormalFontVerySmall);
    ceilLabel->setHoriAlign(kCenterText);
    limContainer->addView(ceilLabel);
    
    // Release fader
    CCoord rX = cX + FaderWidth + FaderGap;
    CRect relRect(rX, GRMeterOffsetY, rX + FaderWidth, GRMeterOffsetY + FaderHeight);
    faders[13] = new CFaderView(relRect, this, kParamLimiterRelease, kColorLimiter);
    faders[13]->setDefaultValue(0.3f);
    limContainer->addView(faders[13]);
    
    CRect relLabelRect(rX, GRMeterOffsetY + FaderHeight + 2, rX + FaderWidth, GRMeterOffsetY + FaderHeight + 16);
    CTextLabel* relLabel = new CTextLabel(relLabelRect, "REL");
    relLabel->setBackColor(CColor(0, 0, 0, 0));
    relLabel->setFontColor(kColorTextDim);
    relLabel->setFont(kNormalFontVerySmall);
    relLabel->setHoriAlign(kCenterText);
    limContainer->addView(relLabel);
    
    // Bypass button (centered below)
    CRect bypassRect(LimiterW / 2 - DeltaBtnW / 2, LimBypassOffsetY, LimiterW / 2 + DeltaBtnW / 2, LimBypassOffsetY + DeltaBtnH);
    CToggleButton* bypassBtn = new CToggleButton(bypassRect, this, 200, "BYPASS", kColorTextDim);
    limContainer->addView(bypassBtn);
    
    container->addView(limContainer);
}

//-------------------------------------------------------------------------------------------------------
// IN/OUT Meters
//-------------------------------------------------------------------------------------------------------
void ELC4LEditor::createIOMeters(CViewContainer* container) {
    using namespace Layout;
    
    CCoord ioY = BandSectionY;
    
    // IO container
    CRect ioRect(IOSectionX, ioY, IOSectionX + 80, ioY + 200);
    CViewContainer* ioContainer = new CViewContainer(ioRect);
    ioContainer->setBackgroundColor(CColor(0, 0, 0, 0));
    
    // Title
    CRect titleRect(0, 0, 80, BandTitleH);
    CTextLabel* titleLabel = new CTextLabel(titleRect, "LEVEL");
    titleLabel->setBackColor(CColor(0, 0, 0, 0));
    titleLabel->setFontColor(kColorGoldPrimary);
    titleLabel->setFont(kNormalFontSmall);
    titleLabel->setHoriAlign(kCenterText);
    ioContainer->addView(titleLabel);
    
    // Input meter
    CRect inRect(8, BandTitleH + FontMargin, 8 + IOMeterW, BandTitleH + FontMargin + IOMeterH);
    inputMeter = new CVerticalMeter(inRect, kColorSpecInput);
    inputMeter->setValue(-12.0f);
    ioContainer->addView(inputMeter);
    
    CRect inLabelRect(8, BandTitleH + FontMargin + IOMeterH + 2, 8 + IOMeterW, BandTitleH + FontMargin + IOMeterH + 16);
    CTextLabel* inLabel = new CTextLabel(inLabelRect, "IN");
    inLabel->setBackColor(CColor(0, 0, 0, 0));
    inLabel->setFontColor(kColorTextDim);
    inLabel->setFont(kNormalFontVerySmall);
    inLabel->setHoriAlign(kCenterText);
    ioContainer->addView(inLabel);
    
    // Output meter
    CCoord outX = 8 + IOMeterW + 10;
    CRect outRect(outX, BandTitleH + FontMargin, outX + IOMeterW, BandTitleH + FontMargin + IOMeterH);
    outputMeter = new CVerticalMeter(outRect, kColorSpecOutput);
    outputMeter->setValue(-6.0f);
    ioContainer->addView(outputMeter);
    
    CRect outLabelRect(outX, BandTitleH + FontMargin + IOMeterH + 2, outX + IOMeterW, BandTitleH + FontMargin + IOMeterH + 16);
    CTextLabel* outLabel = new CTextLabel(outLabelRect, "OUT");
    outLabel->setBackColor(CColor(0, 0, 0, 0));
    outLabel->setFontColor(kColorTextDim);
    outLabel->setFont(kNormalFontVerySmall);
    outLabel->setHoriAlign(kCenterText);
    ioContainer->addView(outLabel);
    
    container->addView(ioContainer);
}

} // namespace ELC4L
