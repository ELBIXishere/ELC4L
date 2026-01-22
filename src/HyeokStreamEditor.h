// -*- coding: utf-8 -*-
//-------------------------------------------------------------------------------------------------------
// ELC4L Editor - Premium Pro-Q Style UI with Win32 GDI
// ELBIX 4-Band Compressor + Limiter - Dark & Gold Theme
//-------------------------------------------------------------------------------------------------------

#pragma once

#ifndef __HyeokStreamEditor__
#define __HyeokStreamEditor__

// Windows header must be included FIRST for COLORREF, HWND, etc.
#if defined(_WIN32) || defined(WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#include "audioeffectx.h"

//-------------------------------------------------------------------------------------------------------
// Editor dimensions (optimized pixel-perfect layout - reduced width for tighter design)
//-------------------------------------------------------------------------------------------------------
constexpr int kEditorWidth = 1020;
constexpr int kEditorHeight = 720;

//-------------------------------------------------------------------------------------------------------
// Premium Dark & Gold Color Theme
//-------------------------------------------------------------------------------------------------------
#define ELC_BG_DARK         RGB(24, 24, 28)
#define ELC_BG_PANEL        RGB(32, 32, 38)
#define ELC_BG_SPECTRUM     RGB(16, 16, 20)
#define ELC_BORDER          RGB(55, 55, 65)
#define ELC_BORDER_LIGHT    RGB(75, 75, 85)

// Gold accent colors
#define ELC_GOLD_PRIMARY    RGB(255, 200, 60)
#define ELC_GOLD_BRIGHT     RGB(255, 215, 100)
#define ELC_GOLD_DIM        RGB(180, 140, 40)

// Text colors
#define ELC_TEXT_BRIGHT     RGB(240, 240, 245)
#define ELC_TEXT_NORMAL     RGB(190, 190, 200)
#define ELC_TEXT_DIM        RGB(110, 110, 125)

// Band colors
#define ELC_BAND_LOW        RGB(200, 100, 100)
#define ELC_BAND_LOWMID     RGB(100, 180, 100)
#define ELC_BAND_HIMID      RGB(100, 150, 220)
#define ELC_BAND_HIGH       RGB(200, 180, 100)

// [Upgrade] FabFilter Pro-Q Style Neon Colors
// Input: Deep Cyan / Output: Warm Gold
#define ELC_SPEC_INPUT      RGB(0, 200, 255)    // Cyan Neon (Line)
#define ELC_SPEC_OUTPUT     RGB(255, 180, 50)   // Gold Neon (Line)

// Fill colors (slightly darker/saturated for the body)
#define ELC_SPEC_FILL_IN    RGB(0, 100, 150)    // Deep Cyan Fill
#define ELC_SPEC_FILL_OUT   RGB(180, 100, 20)   // Deep Gold Fill

// Limiter color
#define ELC_LIMITER         RGB(255, 130, 70)

// Meter colors
#define ELC_METER_GREEN     RGB(80, 200, 120)
#define ELC_METER_YELLOW    RGB(255, 200, 80)
#define ELC_METER_RED       RGB(255, 90, 90)

// M/S/D Button colors
#define ELC_BTN_INACTIVE    RGB(50, 50, 58)
#define ELC_BTN_MUTE        RGB(220, 60, 60)
#define ELC_BTN_SOLO        RGB(255, 220, 60)
#define ELC_BTN_DELTA       RGB(60, 200, 220)

// Legacy color mappings for compatibility
#define HSM_COLOR_BACKGROUND    ELC_BG_DARK
#define HSM_COLOR_PANEL         ELC_BG_PANEL
#define HSM_COLOR_BORDER        ELC_BORDER
#define HSM_COLOR_TEXT          ELC_TEXT_NORMAL
#define HSM_COLOR_TEXT_DIM      ELC_TEXT_DIM
#define HSM_COLOR_TEXT_BRIGHT   ELC_TEXT_BRIGHT
#define HSM_COLOR_LOW_BAND      ELC_BAND_LOW
#define HSM_COLOR_MID_BAND      ELC_BAND_LOWMID
#define HSM_COLOR_HIGH_BAND     ELC_BAND_HIMID
#define HSM_COLOR_FREQ_BAND     ELC_BAND_HIGH
#define HSM_COLOR_LIMITER       ELC_LIMITER

//-------------------------------------------------------------------------------------------------------
// Control Types
//-------------------------------------------------------------------------------------------------------
enum ControlType {
    CTRL_KNOB,
    CTRL_FADER,
    CTRL_SLIDER
};

//-------------------------------------------------------------------------------------------------------
// Control structure
//-------------------------------------------------------------------------------------------------------
struct Control {
    int x, y;
    int width, height;
    int paramIndex;
    ControlType type;
    COLORREF color;
    const char* label;
    float minVal, maxVal;
    bool isBipolar;
    bool isFrequency;
    
    bool hitTest(int mx, int my) const {
        int margin = (type == CTRL_KNOB) ? 25 : 12;
        return mx >= x - margin && mx <= x + width + margin && 
               my >= y - margin && my <= y + height + margin;
    }
    
    int getThumbY(float value) const {
        return y + (int)((1.0f - value) * height);
    }
    
    float valueFromY(int mouseY) const {
        float v = 1.0f - (float)(mouseY - y) / (float)height;
        if (v < 0.0f) v = 0.0f;
        if (v > 1.0f) v = 1.0f;
        return v;
    }
    
    float getKnobAngle(float value) const {
        return -135.0f + value * 270.0f;
    }
};

//-------------------------------------------------------------------------------------------------------
// Forward declaration
//-------------------------------------------------------------------------------------------------------
class HyeokStreamMaster;

//-------------------------------------------------------------------------------------------------------
// HyeokStreamEditor class
//-------------------------------------------------------------------------------------------------------
class HyeokStreamEditor : public AEffEditor {
public:
    HyeokStreamEditor(AudioEffect* effect);
    virtual ~HyeokStreamEditor();
    
    virtual bool getRect(ERect** rect) override;
    virtual bool open(void* ptr) override;
    virtual void close() override;
    virtual void idle() override;
    
    void updateParameter(int index);
    HyeokStreamMaster* getPlugin();

private:
    static const wchar_t* kWindowClassName;
    static bool classRegistered;
    
    static bool registerWindowClass(HINSTANCE hInstance);
    static LRESULT CALLBACK windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    
    // Drawing functions
    void draw(HDC hdc);
    void drawBackground(HDC hdc);
    void drawHeader(HDC hdc);
    void drawSpectrumPanel(HDC hdc);
    void drawSidechainControls(HDC hdc);
    void drawSpectrumCurves(HDC hdc, const float* specIn, const float* specOut, int numBins);
    void drawCrossoverMarkers(HDC hdc, float xo1, float xo2, float xo3);
    void drawCrossoverDragLines(HDC hdc);  // Draggable crossover lines on spectrum
    void drawBandSection(HDC hdc);
    void drawBandButtons(HDC hdc);  // M/S/? buttons
    void drawBypassButtons(HDC hdc);  // Bypass buttons for bands and limiter
    void drawLimiterSection(HDC hdc);
    void drawMeterSection(HDC hdc);
    void drawControl(HDC hdc, const Control& ctrl, float value);
    void drawKnob(HDC hdc, const Control& ctrl, float value);
    void drawFader(HDC hdc, const Control& ctrl, float value);
    void drawSlider(HDC hdc, const Control& ctrl, float value);
    void drawControlValue(HDC hdc, const Control& ctrl, float value);
    void drawLufs(HDC hdc, float lufs);
    void drawVerticalMeter(HDC hdc, int x, int y, int w, int h, float db, 
                           COLORREF color, const char* label, bool isGR);
    void drawMSDButton(HDC hdc, int x, int y, int w, int h, const char* label, bool active, COLORREF activeColor);
    void drawValueBox(HDC hdc, int x, int y, int w, int h, const char* label, const char* value, COLORREF accentColor);
    
    // Smooth curve rendering
    void drawSmoothCurve(HDC hdc, const POINT* pts, int count, COLORREF color, int thickness);
    float catmullRom(float p0, float p1, float p2, float p3, float t);
    void fillSpectrumGradient(HDC hdc, const POINT* pts, int count, int baseY, 
                              COLORREF topColor, COLORREF bottomColor);
    
    // Interaction
    void onMouseDown(int x, int y, bool shiftHeld);
    void onMouseMove(int x, int y, bool shiftHeld);
    void onMouseUp();
    void onMouseWheel(int delta, bool shiftHeld);
    void onDoubleClick(int x, int y);  // Double-click for value input
    int hitTestMSDButton(int x, int y);  // Returns: -1=none, 0-3=M, 4-7=S, 8-11=?
    int hitTestBypassButton(int x, int y);  // Returns: -1=none, 0-3=band bypass, 4=limiter bypass
    int hitTestSidechainControl(int x, int y); // SC button / handle hit test
    int hitTestMSModeButton(int x, int y); // M/S mode button below bypass
    int hitTestValueArea(int x, int y);  // Returns control index for value area click
    void showValueInputDialog(int controlIndex);  // Show text input dialog
    
    ERect editorRect;
    HWND hwnd;
    
    // GDI resources
    HBRUSH bgBrush;
    HFONT headerFont;
    HFONT titleFont;
    HFONT labelFont;
    HFONT valueFont;
    HFONT smallFont;
    
    // Controls
    static constexpr int kNumControls = 15;
    Control controls[kNumControls];
    
    // Interaction state
    int activeControl;
    float dragStartValue;
    int dragStartY;
    float mouseDamping;
    bool fineMode;
    
    // Crossover drag state
    int activeCrossover;       // -1=none, 0-2=xover being dragged
    int dragStartX;            // Start X for crossover drag
    // Sidechain drag state
    bool activeSidechainDrag = false;
    int scDragStartX = 0;
    float scDragStartValue = 0.0f;
    
    DWORD lastUiUpdateMs;
    
    // Peak-hold state (IN/OUT, LUFS, per-band GR, limiter GR)
    float peakIO[2];           // 0 = IN, 1 = OUT
    DWORD peakIOTime[2];
    float peakLUFS;
    DWORD peakLUFSTime;
    float peakGR[4];
    DWORD peakGRTime[4];
    float peakLimiterGR;
    DWORD peakLimiterGRTime;
    // Peak hold duration (ms)
    static constexpr DWORD kPeakHoldMs = 8000; // 8 seconds hold
};

#endif // __HyeokStreamEditor__
