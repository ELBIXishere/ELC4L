// -*- coding: utf-8 -*-
//-------------------------------------------------------------------------------------------------------
// ELC4L Editor - Premium Pro-Q Style UI Implementation
// ELBIX 4-Band Compressor + Limiter - Dark & Gold Theme
//-------------------------------------------------------------------------------------------------------

#include "HyeokStreamEditor.h"
#include "HyeokStreamMaster.h"
#include <cstdio>
#include <cmath>
#include <algorithm>
#include <vector>

// [異붽?] 諛섑닾紐?泥섎━瑜??꾪븳 ?쇱씠釉뚮윭由?留곹겕
#pragma comment(lib, "Msimg32.lib") 

// Simple logging helper: OutputDebugString + append to file for easier capture
static void EditorLog(const char* msg) {
    OutputDebugStringA(msg);
    FILE* f = fopen("output\\plugin_ui_log.txt", "a");
    if (f) {
        fprintf(f, "%s", msg);
        fclose(f);
    }
}

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Windows min/max can conflict with std::min/max, use macros
#ifndef ELCMIN
#define ELCMIN(a, b) ((a) < (b) ? (a) : (b))
#endif
#ifndef ELCMAX
#define ELCMAX(a, b) ((a) > (b) ? (a) : (b))
#endif

//-------------------------------------------------------------------------------------------------------
// Layout Constants - Professional Plugin UI Design
// NEW DESIGN: Top half = Large Spectrum, Bottom half = Compressors + Meters
// Each band: GR meter + THR fader + MK fader
// M/S buttons (square) below GR, Delta (wide rect) below M/S, Bypass below Delta
// IN/OUT meters at far right
// Crossover: Draggable lines on spectrum analyzer
//-------------------------------------------------------------------------------------------------------
namespace Layout {
    // Font-based spacing for no overlap
    constexpr int FontMargin = 26;
    constexpr int Padding = 22;

    // ===== WINDOW SIZE (Increased for left spacing) =====
    constexpr int WindowW = 1020;        // 980 -> 1020
    constexpr int WindowH = 720;         // 700 -> 720

    // Global left offset to give breathing room
    constexpr int GlobalOffsetX = 80;

    // ===== TOP HALF: SPECTRUM ANALYZER =====
    constexpr int SpectrumX = 15 + GlobalOffsetX; // Offset applied
    constexpr int SpectrumY = 55;
    constexpr int SpectrumW = 860;
    constexpr int SpectrumH = 260;
    
    // Crossover drag lines (no knobs - lines on spectrum)
    constexpr int XoverLineY1 = SpectrumY + 20;   // Top of draggable area
    constexpr int XoverLineY2 = SpectrumY + SpectrumH - 10;  // Bottom of draggable area
    constexpr int XoverLineWidth = 8;   // Click area width for drag
    
    // ===== BOTTOM HALF: COMPRESSORS + METERS =====
    constexpr int BandSectionY = SpectrumY + SpectrumH + FontMargin + 35; // +20px to give limiter text room
    constexpr int BandHeight = 310;

    // Each band column (4 bands + limiter + IN/OUT)
    constexpr int BandWidth = 140;
    // Move band section right to balance layout (reduce empty space on right)
    constexpr int BandStartX = 60 + GlobalOffsetX; // Shifted right
    
    // Band internal layout
    constexpr int BandTitleH = 26;      // Title bar height - increased
    
    // GR Meter (vertical, left side of each band)
    constexpr int GRMeterOffsetX = 8;   // Offset from band left
    constexpr int GRMeterW = 20;        // Width of GR meter
    constexpr int GRMeterH = 120;       // Height of GR meter (reduced)
    constexpr int GRMeterOffsetY = BandTitleH + FontMargin;  // Below title
    
    // Faders (right of GR meter)
    constexpr int FaderOffsetX = GRMeterOffsetX + GRMeterW + 12;  // After GR meter
    constexpr int FaderWidth = 24;      // Fader track width (reduced)
    constexpr int FaderHeight = 120;    // Same as GR meter
    constexpr int FaderGap = 8;         // Gap between THR and MK faders
    
    // Buttons below GR meter (M/S, Delta, Bypass)
    constexpr int BtnOffsetY = GRMeterOffsetY + GRMeterH + FontMargin + 5;  // Below GR meter, more space
    
    // M/S buttons (small square)
    constexpr int MSBtnW = 20;
    constexpr int MSBtnH = 20;
    constexpr int MSBtnGap = 3;
    
    // Delta button (wide rectangle) below M/S
    constexpr int DeltaBtnOffsetY = BtnOffsetY + MSBtnH + 8;
    constexpr int DeltaBtnW = MSBtnW * 2 + MSBtnGap;  // 43
    constexpr int DeltaBtnH = 16;
    
    // Bypass button (wide rectangle) below Delta
    constexpr int BypassBtnOffsetY = DeltaBtnOffsetY + DeltaBtnH + 8;
    constexpr int BypassBtnW = DeltaBtnW;
    constexpr int BypassBtnH = 16;
    
    // Limiter section (after 4 bands)
    // Narrow gap between compressor bands and limiter
    constexpr int LimiterX = BandStartX + BandWidth * 4 + 8;
    constexpr int LimiterY = BandSectionY;
    constexpr int LimiterW = 130;

    // Limiter bypass position (slightly higher)
    constexpr int LimBypassOffsetY = GRMeterOffsetY + GRMeterH + FontMargin + 30;

    // ===== IN/OUT METERS (moved left) =====
    // IO section moved slightly left to provide breathing room on the right edge
    constexpr int IOSectionX = 800 + GlobalOffsetX;
    constexpr int IOMeterW = 24;
    constexpr int IOMeterH = 160;
    constexpr int IOMeterOffsetY = BandTitleH + FontMargin;
    constexpr int IOMeterGap = 10;

    // Sidechain button constants (left sidebar)
    constexpr int SCButtonX = 20;
    constexpr int SCButtonY = SpectrumY;
    constexpr int SCButtonW = 40;
    constexpr int SCButtonH = 24;

    // [NEW] M/S Mode Button Offset (below bypass)
    constexpr int MSModeBtnOffsetY = BypassBtnOffsetY + BypassBtnH + 8;
    constexpr int MSModeBtnW = DeltaBtnW;
    constexpr int MSModeBtnH = 16;
}

//-------------------------------------------------------------------------------------------------------
// Static members
//-------------------------------------------------------------------------------------------------------
const wchar_t* HyeokStreamEditor::kWindowClassName = L"ELC4LEditorClass";
bool HyeokStreamEditor::classRegistered = false;

//-------------------------------------------------------------------------------------------------------
// Helper to initialize controls - NEW LAYOUT
// Each band: GR meter (left) + THR fader + MK fader (right)
// Crossover: Draggable lines on spectrum (no knobs)
//-------------------------------------------------------------------------------------------------------
void initializeControlsHelper(Control* controls) {
    using namespace Layout;
    
    COLORREF bandColors[4] = {
        RGB(220, 90, 90),   // Low - Red
        RGB(90, 190, 90),   // Low-Mid - Green
        RGB(90, 140, 220),  // Hi-Mid - Blue
        RGB(220, 180, 70)   // High - Yellow
    };
    
    // Band 1-4: THR and MK faders
    for (int band = 0; band < 4; ++band) {
        int bandX = BandStartX + band * BandWidth;
        int faderY = BandSectionY + GRMeterOffsetY;
        int faderX = bandX + FaderOffsetX;
        
        // Threshold fader (index 0-3)
        controls[band].x = faderX;
        controls[band].y = faderY;
        controls[band].width = FaderWidth;
        controls[band].height = FaderHeight;
        controls[band].paramIndex = kParamBand1Thresh + band;
        controls[band].type = CTRL_FADER;
        controls[band].color = bandColors[band];
        controls[band].label = "THR";
        controls[band].minVal = -36.0f;
        controls[band].maxVal = 0.0f;
        controls[band].isBipolar = false;
        controls[band].isFrequency = false;
        
        // Makeup fader (index 4-7)
        controls[4 + band].x = faderX + FaderWidth + FaderGap;
        controls[4 + band].y = faderY;
        controls[4 + band].width = FaderWidth;
        controls[4 + band].height = FaderHeight;
        controls[4 + band].paramIndex = kParamBand1Makeup + band;
        controls[4 + band].type = CTRL_FADER;
        controls[4 + band].color = bandColors[band];
        controls[4 + band].label = "MK";
        controls[4 + band].minVal = -12.0f;
        controls[4 + band].maxVal = 12.0f;
        controls[4 + band].isBipolar = true;
        controls[4 + band].isFrequency = false;
    }
    
    // Crossover controls (8-10) - No longer visible knobs, but still track parameters
    // Used for draggable lines on spectrum - position not used for rendering
    for (int i = 0; i < 3; ++i) {
        controls[8 + i].x = 0;
        controls[8 + i].y = 0;
        controls[8 + i].width = 0;
        controls[8 + i].height = 0;
        controls[8 + i].paramIndex = kParamXover1 + i;
        controls[8 + i].type = CTRL_KNOB;  // Keep type but not rendered
        controls[8 + i].color = RGB(255, 200, 60);
        controls[8 + i].label = (i == 0) ? "XO1" : ((i == 1) ? "XO2" : "XO3");
        controls[8 + i].minVal = 20.0f;
        controls[8 + i].maxVal = 20000.0f;
        controls[8 + i].isBipolar = false;
        controls[8 + i].isFrequency = true;
    }
    
    // Limiter controls - 3 faders
    int limFaderY = BandSectionY + GRMeterOffsetY;
    int limFaderX = LimiterX + 18;
    
    // Limiter Threshold
    controls[11].x = limFaderX;
    controls[11].y = limFaderY;
    controls[11].width = FaderWidth;
    controls[11].height = FaderHeight;
    controls[11].paramIndex = kParamLimiterThresh;
    controls[11].type = CTRL_FADER;
    controls[11].color = RGB(255, 120, 60);
    controls[11].label = "THR";
    controls[11].minVal = -24.0f;
    controls[11].maxVal = 0.0f;
    controls[11].isBipolar = false;
    controls[11].isFrequency = false;
    
    // Limiter Ceiling
    controls[12].x = limFaderX + FaderWidth + FaderGap;
    controls[12].y = limFaderY;
    controls[12].width = FaderWidth;
    controls[12].height = FaderHeight;
    controls[12].paramIndex = kParamLimiterCeiling;
    controls[12].type = CTRL_FADER;
    controls[12].color = RGB(255, 120, 60);
    controls[12].label = "CEIL";
    controls[12].minVal = -24.0f;
    controls[12].maxVal = 0.0f;
    controls[12].isBipolar = false;
    controls[12].isFrequency = false;
    
    // Limiter Release
    controls[13].x = limFaderX + (FaderWidth + FaderGap) * 2;
    controls[13].y = limFaderY;
    controls[13].width = FaderWidth;
    controls[13].height = FaderHeight;
    controls[13].paramIndex = kParamLimiterRelease;
    controls[13].type = CTRL_FADER;
    controls[13].color = RGB(255, 120, 60);
    controls[13].label = "REL";
    controls[13].minVal = 10.0f;
    controls[13].maxVal = 500.0f;
    controls[13].isBipolar = false;
    controls[13].isFrequency = false;

    // Sidechain HPF control (index 14) - convert to vertical fader at left edge
    controls[14].paramIndex = kParamSidechainFreq;
    controls[14].type = CTRL_FADER; // vertical fader
    controls[14].color = ELC_GOLD_PRIMARY; // gold accent for SC
    controls[14].label = "SC HPF";
    controls[14].minVal = 20.0f;
    controls[14].maxVal = 20000.0f; // expanded range 20..20k
    controls[14].isBipolar = false;
    controls[14].isFrequency = true;
    // Place the SC HPF fader in left sidebar under the SC button
    controls[14].x = 25; // center under SC button (x=20, button width 40)
    controls[14].y = SpectrumY + 35; // just below the SC button
    controls[14].width = 30;
    controls[14].height = SpectrumH - 50; // tall fader matching spectrum height
}

//-------------------------------------------------------------------------------------------------------
// Constructor
//-------------------------------------------------------------------------------------------------------
HyeokStreamEditor::HyeokStreamEditor(AudioEffect* effect)
    : AEffEditor(effect)
    , hwnd(nullptr)
    , bgBrush(nullptr)
    , headerFont(nullptr)
    , titleFont(nullptr)
    , labelFont(nullptr)
    , valueFont(nullptr)
    , smallFont(nullptr)
    , activeControl(-1)
    , dragStartValue(0.0f)
    , dragStartY(0)
    , mouseDamping(0.5f)
    , fineMode(false)
    , activeCrossover(-1)
    , dragStartX(0)
    , lastUiUpdateMs(0)
{
    editorRect.left = 0;
    editorRect.top = 0;
    editorRect.right = kEditorWidth;
    editorRect.bottom = kEditorHeight;
    
    initializeControlsHelper(controls);
    // Initialize peak-hold values
    for (int i = 0; i < 2; ++i) { peakIO[i] = -1000.0f; peakIOTime[i] = 0; }
    peakLUFS = -1000.0f; peakLUFSTime = 0;
    for (int i = 0; i < 4; ++i) { peakGR[i] = -1000.0f; peakGRTime[i] = 0; }
    peakLimiterGR = -1000.0f; peakLimiterGRTime = 0;
}

HyeokStreamEditor::~HyeokStreamEditor() {
    close();
}

//-------------------------------------------------------------------------------------------------------
// AEffEditor interface
//-------------------------------------------------------------------------------------------------------
bool HyeokStreamEditor::getRect(ERect** rect) {
    *rect = &editorRect;
    return true;
}

bool HyeokStreamEditor::open(void* ptr) {
    AEffEditor::open(ptr);

    char dbg[256];
    sprintf(dbg, "HyeokStreamEditor::open ptr=%p\n", ptr);
    EditorLog(dbg);

    if (ptr == nullptr) {
        EditorLog("HyeokStreamEditor::open - parent ptr is NULL\n");
        return false;
    }

    HWND parentHwnd = (HWND)ptr;
    if (!IsWindow(parentHwnd)) {
        EditorLog("HyeokStreamEditor::open - parent hwnd is not a valid window\n");
        return false;
    }

    HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtr(parentHwnd, GWLP_HINSTANCE);
    if (hInstance == 0) {
        DWORD err = GetLastError();
        sprintf(dbg, "HyeokStreamEditor::open - GetWindowLongPtr returned 0, GetLastError=%lu\n", (unsigned long)err);
        EditorLog(dbg);
        hInstance = GetModuleHandle(NULL);
    }

    if (!classRegistered) {
        if (!registerWindowClass(hInstance)) {
            EditorLog("HyeokStreamEditor::open - registerWindowClass failed\n");
            return false;
        }
    }

    bgBrush = CreateSolidBrush(ELC_BG_DARK);
    headerFont = CreateFontW(36, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    titleFont = CreateFontW(18, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                           DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                           CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    labelFont = CreateFontW(14, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                           DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                           CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    valueFont = CreateFontW(13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                           DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                           CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    smallFont = CreateFontW(11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                           DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                           CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    
    hwnd = CreateWindowExW(
        0,
        kWindowClassName,
        L"ELC4L",
        WS_CHILD | WS_VISIBLE,
        0, 0, kEditorWidth, kEditorHeight,
        parentHwnd,
        nullptr,
        hInstance,
        this
    );
    if (!hwnd) {
        DWORD err = GetLastError();
        char dbg2[256];
        sprintf(dbg2, "HyeokStreamEditor::open - CreateWindowExW failed, err=%lu\n", (unsigned long)err);
        EditorLog(dbg2);
        return false;
    }

    return true;
}

void HyeokStreamEditor::close() {
    if (hwnd) {
        DestroyWindow(hwnd);
        hwnd = nullptr;
    }
    
    if (bgBrush) { DeleteObject(bgBrush); bgBrush = nullptr; }
    if (headerFont) { DeleteObject(headerFont); headerFont = nullptr; }
    if (titleFont) { DeleteObject(titleFont); titleFont = nullptr; }
    if (labelFont) { DeleteObject(labelFont); labelFont = nullptr; }
    if (valueFont) { DeleteObject(valueFont); valueFont = nullptr; }
    if (smallFont) { DeleteObject(smallFont); smallFont = nullptr; }
    
    AEffEditor::close();
}

void HyeokStreamEditor::idle() {
    if (!hwnd) return;
    DWORD now = GetTickCount();
    if (now - lastUiUpdateMs > 33) {
        InvalidateRect(hwnd, nullptr, FALSE);
        lastUiUpdateMs = now;
    }
}

void HyeokStreamEditor::updateParameter(int index) {
    if (hwnd) {
        InvalidateRect(hwnd, nullptr, FALSE);
    }
}

HyeokStreamMaster* HyeokStreamEditor::getPlugin() {
    return (HyeokStreamMaster*)effect;
}

//-------------------------------------------------------------------------------------------------------
// Window class registration
//-------------------------------------------------------------------------------------------------------
bool HyeokStreamEditor::registerWindowClass(HINSTANCE hInstance) {
    WNDCLASSEXW wcex = {};
    wcex.cbSize = sizeof(WNDCLASSEXW);
    wcex.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_DBLCLKS;  // Added CS_DBLCLKS for double-click
    wcex.lpfnWndProc = windowProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = sizeof(void*);
    wcex.hInstance = hInstance;
    wcex.hIcon = nullptr;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = nullptr;
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = kWindowClassName;
    wcex.hIconSm = nullptr;
    
    if (RegisterClassExW(&wcex)) {
        classRegistered = true;
        return true;
    }
    
    if (GetLastError() == ERROR_CLASS_ALREADY_EXISTS) {
        classRegistered = true;
        return true;
    }
    
    return false;
}

//-------------------------------------------------------------------------------------------------------
// Window procedure
//-------------------------------------------------------------------------------------------------------
LRESULT CALLBACK HyeokStreamEditor::windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    HyeokStreamEditor* editor = nullptr;
    
    if (msg == WM_CREATE) {
        CREATESTRUCT* cs = (CREATESTRUCT*)lParam;
        editor = (HyeokStreamEditor*)cs->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)editor);
    } else {
        editor = (HyeokStreamEditor*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    }
    
    switch (msg) {
        case WM_PAINT: {
            if (editor) {
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hwnd, &ps);
                
                HDC memDC = CreateCompatibleDC(hdc);
                HBITMAP memBitmap = CreateCompatibleBitmap(hdc, kEditorWidth, kEditorHeight);
                HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, memBitmap);
                
                editor->draw(memDC);
                
                BitBlt(hdc, 0, 0, kEditorWidth, kEditorHeight, memDC, 0, 0, SRCCOPY);
                
                SelectObject(memDC, oldBitmap);
                DeleteObject(memBitmap);
                DeleteDC(memDC);
                
                EndPaint(hwnd, &ps);
            }
            return 0;
        }
        
        case WM_LBUTTONDOWN: {
            if (editor) {
                int x = LOWORD(lParam);
                int y = HIWORD(lParam);
                bool shiftHeld = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
                editor->onMouseDown(x, y, shiftHeld);
                SetCapture(hwnd);
            }
            return 0;
        }
        
        case WM_LBUTTONDBLCLK: {
            if (editor) {
                int x = LOWORD(lParam);
                int y = HIWORD(lParam);
                editor->onDoubleClick(x, y);
            }
            return 0;
        }
        
        case WM_MOUSEMOVE: {
            if (editor && (editor->activeControl >= 0 || editor->activeCrossover >= 0)) {
                int x = LOWORD(lParam);
                int y = HIWORD(lParam);
                bool shiftHeld = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
                editor->onMouseMove(x, y, shiftHeld);
            }
            return 0;
        }
        
        case WM_LBUTTONUP: {
            if (editor) {
                editor->onMouseUp();
                ReleaseCapture();
            }
            return 0;
        }
        
        case WM_MOUSEWHEEL: {
            if (editor) {
                int delta = GET_WHEEL_DELTA_WPARAM(wParam);
                bool shiftHeld = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
                editor->onMouseWheel(delta, shiftHeld);
            }
            return 0;
        }
        
        case WM_SETCURSOR: {
            // Change cursor based on what's under it
            if (editor && LOWORD(lParam) == HTCLIENT) {
                POINT pt;
                GetCursorPos(&pt);
                ScreenToClient(hwnd, &pt);
                
                // Check if over any control
                for (int i = 0; i < editor->kNumControls; ++i) {
                    if (editor->controls[i].hitTest(pt.x, pt.y)) {
                        SetCursor(LoadCursor(nullptr, IDC_HAND));
                        return TRUE;
                    }
                }
                
                // Check if over value area (for double-click input)
                if (editor->hitTestValueArea(pt.x, pt.y) >= 0) {
                    SetCursor(LoadCursor(nullptr, IDC_IBEAM));
                    return TRUE;
                }
                
                // Check if over M/S/Delta buttons
                if (editor->hitTestMSDButton(pt.x, pt.y) >= 0) {
                    SetCursor(LoadCursor(nullptr, IDC_HAND));
                    return TRUE;
                }
                
                // Check if over bypass buttons
                if (editor->hitTestBypassButton(pt.x, pt.y) >= 0) {
                    SetCursor(LoadCursor(nullptr, IDC_HAND));
                    return TRUE;
                }
                
                SetCursor(LoadCursor(nullptr, IDC_ARROW));
                return TRUE;
            }
            break;
        }
        
        case WM_ERASEBKGND:
            return 1;
            
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

//-------------------------------------------------------------------------------------------------------
// Main draw function
//-------------------------------------------------------------------------------------------------------
void HyeokStreamEditor::draw(HDC hdc) {
    drawBackground(hdc);
    drawHeader(hdc);
    
    HyeokStreamMaster* plugin = getPlugin();
    if (!plugin) return;
    
    drawSpectrumPanel(hdc);
    drawSidechainControls(hdc);
    drawSpectrumCurves(hdc, plugin->getDisplayIn(), plugin->getDisplayOut(), kDisplayBins);
    drawCrossoverMarkers(hdc, plugin->getXover1Hz(), plugin->getXover2Hz(), plugin->getXover3Hz());
    
    drawBandSection(hdc);
    drawBypassButtons(hdc);  // Bypass buttons for bands
    drawLimiterSection(hdc);
    drawMeterSection(hdc);
    drawBandButtons(hdc);  // M/S/? buttons - now below meters
    drawLufs(hdc, plugin->getLufsMomentary());
    
    for (int i = 0; i < kNumControls; ++i) {
        float value = plugin->getParameterValue(controls[i].paramIndex);
        drawControl(hdc, controls[i], value);
        drawControlValue(hdc, controls[i], value);
    }
}

//-------------------------------------------------------------------------------------------------------
// Background
//-------------------------------------------------------------------------------------------------------
void HyeokStreamEditor::drawBackground(HDC hdc) {
    RECT rect = { 0, 0, kEditorWidth, kEditorHeight };
    HBRUSH brush = CreateSolidBrush(ELC_BG_DARK);
    FillRect(hdc, &rect, brush);
    DeleteObject(brush);
    
    RECT panelRect = { 15, 50, kEditorWidth - 15, kEditorHeight - 15 };
    brush = CreateSolidBrush(ELC_BG_PANEL);
    FillRect(hdc, &panelRect, brush);
    DeleteObject(brush);
    
    HPEN pen = CreatePen(PS_SOLID, 1, ELC_BORDER);
    HPEN oldPen = (HPEN)SelectObject(hdc, pen);
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
    Rectangle(hdc, panelRect.left, panelRect.top, panelRect.right, panelRect.bottom);
    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBrush);
    DeleteObject(pen);
}

//-------------------------------------------------------------------------------------------------------
// Header with ELBIX branding
//-------------------------------------------------------------------------------------------------------
void HyeokStreamEditor::drawHeader(HDC hdc) {
    SetBkMode(hdc, TRANSPARENT);
    SelectObject(hdc, headerFont);
    SetTextColor(hdc, ELC_GOLD_PRIMARY);
    
    RECT titleRect = { 20, 8, 200, 45 };
    DrawTextA(hdc, "ELC4L", -1, &titleRect, DT_LEFT | DT_SINGLELINE);
    
    SelectObject(hdc, labelFont);
    SetTextColor(hdc, ELC_TEXT_DIM);
    RECT subRect = { 120, 18, 450, 38 };
    DrawTextA(hdc, "ELBIX 4-Band Compressor + Limiter", -1, &subRect, DT_LEFT | DT_SINGLELINE);
}

//-------------------------------------------------------------------------------------------------------
// Spectrum panel
//-------------------------------------------------------------------------------------------------------
void HyeokStreamEditor::drawSpectrumPanel(HDC hdc) {
    using namespace Layout;
    
    RECT panel = { SpectrumX, SpectrumY, SpectrumX + SpectrumW, SpectrumY + SpectrumH };
    HBRUSH panelBrush = CreateSolidBrush(ELC_BG_SPECTRUM);
    FillRect(hdc, &panel, panelBrush);
    DeleteObject(panelBrush);
    
    HPEN borderPen = CreatePen(PS_SOLID, 1, ELC_BORDER);
    HPEN oldPen = (HPEN)SelectObject(hdc, borderPen);
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
    Rectangle(hdc, panel.left, panel.top, panel.right, panel.bottom);
    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBrush);
    DeleteObject(borderPen);
    
    const int x0 = panel.left + 35;
    const int y0 = panel.top + 10;
    const int width = panel.right - panel.left - 45;
    const int height = panel.bottom - panel.top - 25;
    
    HPEN gridPen = CreatePen(PS_SOLID, 1, RGB(35, 35, 42));
    oldPen = (HPEN)SelectObject(hdc, gridPen);
    
    for (int i = 0; i <= 6; ++i) {
        int y = y0 + (i * height) / 6;
        MoveToEx(hdc, x0, y, nullptr);
        LineTo(hdc, x0 + width, y);
    }
    
    float freqs[] = { 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f };
    for (int i = 0; i < 8; ++i) {
        float logPos = (log10f(freqs[i]) - log10f(20.0f)) / (log10f(20000.0f) - log10f(20.0f));
        int x = x0 + (int)(logPos * width);
        MoveToEx(hdc, x, y0, nullptr);
        LineTo(hdc, x, y0 + height);
    }
    
    SelectObject(hdc, oldPen);
    DeleteObject(gridPen);
    
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, ELC_TEXT_DIM);
    SelectObject(hdc, smallFont);
    const char* dbLabels[] = { "0", "-10", "-20", "-30", "-40", "-50", "-60" };
    for (int i = 0; i <= 6; ++i) {
        int y = y0 + (i * height) / 6;
        RECT dbRect = { panel.left + 2, y - 6, x0 - 3, y + 6 };
        DrawTextA(hdc, dbLabels[i], -1, &dbRect, DT_RIGHT | DT_SINGLELINE);
    }
    
    const char* freqLabels[] = { "50", "100", "200", "500", "1k", "2k", "5k", "10k" };
    for (int i = 0; i < 8; ++i) {
        float logPos = (log10f(freqs[i]) - log10f(20.0f)) / (log10f(20000.0f) - log10f(20.0f));
        int x = x0 + (int)(logPos * width);
        RECT freqRect = { x - 18, y0 + height + 2, x + 18, y0 + height + 14 };
        DrawTextA(hdc, freqLabels[i], -1, &freqRect, DT_CENTER | DT_SINGLELINE);
    }
    
    SetTextColor(hdc, ELC_SPEC_INPUT);
    RECT inLabel = { panel.left + 45, panel.top + 2, panel.left + 100, panel.top + 14 };
    DrawTextA(hdc, "INPUT", -1, &inLabel, DT_LEFT | DT_SINGLELINE);
    
    SetTextColor(hdc, ELC_SPEC_OUTPUT);
    RECT outLabel = { panel.left + 105, panel.top + 2, panel.left + 175, panel.top + 14 };
    DrawTextA(hdc, "OUTPUT", -1, &outLabel, DT_LEFT | DT_SINGLELINE);
}

//-------------------------------------------------------------------------------------------------------
// Draw Sidechain controls inside the spectrum area
//-------------------------------------------------------------------------------------------------------

// [NEW] 諛섑닾紐??ㅺ컖??洹몃━湲??ы띁 ?⑥닔 (FabFilter ?ㅽ???Fill)
void FillAlphaPoly(HDC hdc, const std::vector<POINT>& pts, COLORREF color, BYTE alpha) {
    if (pts.empty()) return;

    // 1. ?꾩껜 ?곸뿭 怨꾩궛 (Bounding Box)
    RECT bounds = { pts[0].x, pts[0].y, pts[0].x, pts[0].y };
    for (const auto& p : pts) {
        if (p.x < bounds.left) bounds.left = p.x;
        if (p.x > bounds.right) bounds.right = p.x;
        if (p.y < bounds.top) bounds.top = p.y;
        if (p.y > bounds.bottom) bounds.bottom = p.y;
    }
    int w = bounds.right - bounds.left;
    int h = bounds.bottom - bounds.top;
    if (w <= 0 || h <= 0) return;

    // 2. ?꾩떆 硫붾え由?DC ?앹꽦
    HDC memDC = CreateCompatibleDC(hdc);
    
    // 3. 32鍮꾪듃 鍮꾪듃留??앹꽦 (Alpha 梨꾨꼸 吏?먯슜)
    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = w;
    bmi.bmiHeader.biHeight = h; // Top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* pBits = nullptr;
    HBITMAP hBmp = CreateDIBSection(memDC, &bmi, DIB_RGB_COLORS, &pBits, nullptr, 0);
    HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, hBmp);

    // 4. 諛곌꼍???щ챸?섍쾶 珥덇린??
    RECT tempRect = {0, 0, w, h};
    HBRUSH black = CreateSolidBrush(RGB(0,0,0));
    FillRect(memDC, &tempRect, black);
    DeleteObject(black);

    // 5. ?ㅺ컖??洹몃━湲?(醫뚰몴瑜?0,0 湲곗??쇰줈 ?대룞)
    std::vector<POINT> offsetPts = pts;
    for (auto& p : offsetPts) {
        p.x -= bounds.left;
        p.y -= bounds.top;
    }

    HBRUSH brush = CreateSolidBrush(color);
    HPEN pen = CreatePen(PS_NULL, 0, 0);
    HBRUSH oldBrush = (HBRUSH)SelectObject(memDC, brush);
    HPEN oldPen = (HPEN)SelectObject(memDC, pen);
    Polygon(memDC, offsetPts.data(), (int)offsetPts.size());
    SelectObject(memDC, oldPen);
    SelectObject(memDC, oldBrush);
    DeleteObject(brush);
    DeleteObject(pen);

    // 6. ?붾㈃??AlphaBlend濡??⑹꽦
    BLENDFUNCTION bf;
    bf.BlendOp = AC_SRC_OVER;
    bf.BlendFlags = 0;
    bf.SourceConstantAlpha = alpha;
    bf.AlphaFormat = 0;

    AlphaBlend(hdc, bounds.left, bounds.top, w, h,
               memDC, 0, 0, w, h, bf);

    // ?뺣━
    SelectObject(memDC, oldBmp);
    DeleteObject(hBmp);
    DeleteDC(memDC);
}
void HyeokStreamEditor::drawSidechainControls(HDC hdc) {
    using namespace Layout;
    HyeokStreamMaster* plugin = getPlugin();
    if (!plugin) return;

    // SC button (Spectrum top-left) - place in left sidebar area
    int btnX = SCButtonX;
    int btnY = SCButtonY;
    int btnW = SCButtonW;
    int btnH = SCButtonH;

    bool isActive = plugin->getParameterValue(kParamSidechainActive) > 0.5f;
    drawMSDButton(hdc, btnX, btnY, btnW, btnH, "SC", isActive, ELC_GOLD_PRIMARY);

    // Draw HPF magnitude curve (1-pole HPF) across the spectrum
    float scNorm = plugin->getParameterValue(kParamSidechainFreq);
    float scCutoff = 20.0f * powf(20000.0f / 20.0f, scNorm); // 20..20000 Hz

    const int graphX = SpectrumX + 35;
    const int graphW = SpectrumW - 45;
    const int graphY = SpectrumY + 10;
    const int graphH = SpectrumH - 25;

    // dB mapping matches analyzer: top slightly lowered to avoid hugging top
    const float dbTop = 3.0f;
    const float dbBottom = -90.0f;

    auto freqToX = [&](float freq)->int {
        float lp = (log10f(freq) - log10f(20.0f)) / (log10f(20000.0f) - log10f(20.0f));
        if (lp < 0.0f) lp = 0.0f; if (lp > 1.0f) lp = 1.0f;
        return graphX + (int)(lp * graphW);
    };

    auto dbToY = [&](float db)->int {
        float t = (dbTop - db) / (dbTop - dbBottom); // 0..1
        if (t < 0.0f) t = 0.0f; if (t > 1.0f) t = 1.0f;
        return graphY + (int)(t * graphH);
    };

    auto getHpfMagnitude = [&](float freq, float cutoff)->float {
        // 1-pole HPF magnitude approx: mag = f / sqrt(f^2 + fc^2)
        float f = freq;
        float c = cutoff;
        return (f / sqrtf(f * f + c * c));
    };

    // Build curve points (log spaced) and polygon for fill
    const int samples = 180;
    std::vector<POINT> pts;
    pts.reserve(samples + 2);
    for (int i = 0; i < samples; ++i) {
        float t = (float)i / (float)(samples - 1);
        float freq = 20.0f * powf(20000.0f / 20.0f, t);
        float mag = getHpfMagnitude(freq, scCutoff);
        float db = 20.0f * log10f(mag + 1.0e-12f);
        int x = freqToX(freq);
        int y = dbToY(db);
        pts.push_back({ x, y });
    }

    // Create polygon for fill: curve -> bottom right -> bottom left
    std::vector<POINT> poly;
    poly.reserve(pts.size() + 2);
    for (const auto &p : pts) poly.push_back(p);
    // bottom right
    poly.push_back({ graphX + graphW, graphY + graphH });
    // bottom left
    poly.push_back({ graphX, graphY + graphH });

    // Color logic: Gold when active, gray when off
    COLORREF mainColor = isActive ? ELC_GOLD_PRIMARY : RGB(100, 100, 100);
    COLORREF polyFill = isActive ? ELC_GOLD_PRIMARY : RGB(50, 50, 50);
    BYTE alpha = isActive ? 60 : 30; // fairly transparent

    // Fill under curve using alpha blend so grid remains visible
    FillAlphaPoly(hdc, poly, polyFill, alpha);

    // Draw the curve line (thinner for a refined look)
    HPEN curvePen = CreatePen(PS_SOLID, 1, mainColor);
    HPEN oldCurvePen = (HPEN)SelectObject(hdc, curvePen);
    Polyline(hdc, pts.data(), (int)pts.size());
    SelectObject(hdc, oldCurvePen);
    DeleteObject(curvePen);

    // Draw circular handle ON the curve at cutoff frequency (handle ~ -3 dB point)
    int handleX = freqToX(scCutoff);
    float handleMag = getHpfMagnitude(scCutoff, scCutoff);
    float handleDb = 20.0f * log10f(handleMag + 1.0e-12f);
    int handleY = dbToY(handleDb);
    int handleR = 6;
    HBRUSH handleBrush = CreateSolidBrush(mainColor);
    HBRUSH oldHandleBrush = (HBRUSH)SelectObject(hdc, handleBrush);
    HPEN handlePen = CreatePen(PS_SOLID, 1, RGB(20, 20, 20));
    HPEN oldHandlePen = (HPEN)SelectObject(hdc, handlePen);
    Ellipse(hdc, handleX - handleR, handleY - handleR, handleX + handleR, handleY + handleR);
    SelectObject(hdc, oldHandlePen);
    SelectObject(hdc, oldHandleBrush);
    DeleteObject(handleBrush);
    DeleteObject(handlePen);

    // Draw SC label near handle if active
    if (isActive) {
        char freqText[32];
        sprintf(freqText, "%.0f Hz", scCutoff);
        SetTextColor(hdc, mainColor);
        SelectObject(hdc, smallFont);
        RECT textRect = { handleX + 8, handleY - 10, handleX + 120, handleY + 12 };
        DrawTextA(hdc, freqText, -1, &textRect, DT_LEFT | DT_SINGLELINE);
    }

    // Draw control for sidechain HPF (if control exists)
    if (14 < kNumControls) {
        float scNorm = plugin->getParameterValue(kParamSidechainFreq);
        // override control color depending on active state for visual feedback
        Control temp = controls[14];
        temp.color = isActive ? ELC_GOLD_PRIMARY : RGB(120, 120, 120);
        // Draw using generic control drawer (will draw fader for CTRL_FADER)
        drawControl(hdc, temp, scNorm);

        // Draw numeric frequency value adjacent to the fader (live update)
        float scFreqDisplay = 20.0f * powf(20000.0f / 20.0f, scNorm);
        char freqText[32];
        sprintf(freqText, "%.0f Hz", scFreqDisplay);
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, isActive ? ELC_GOLD_PRIMARY : RGB(140,140,140));
        SelectObject(hdc, smallFont);
        // Place frequency text below the slider to avoid clipping
        RECT fRect = { temp.x, temp.y + temp.height + 5, temp.x + temp.width + 140, temp.y + temp.height + 25 };
        DrawTextA(hdc, freqText, -1, &fRect, DT_LEFT | DT_SINGLELINE);
    }
}

//-------------------------------------------------------------------------------------------------------
// Catmull-Rom spline
//-------------------------------------------------------------------------------------------------------
float HyeokStreamEditor::catmullRom(float p0, float p1, float p2, float p3, float t) {
    float t2 = t * t;
    float t3 = t2 * t;
    return 0.5f * ((2.0f * p1) +
                   (-p0 + p2) * t +
                   (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 +
                   (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3);
}

//-------------------------------------------------------------------------------------------------------
// Spectrum curves with Catmull-Rom smoothing
//-------------------------------------------------------------------------------------------------------
// [REPLACEMENT] Pro-Q Style Transparent Spectrum Rendering
void HyeokStreamEditor::drawSpectrumCurves(HDC hdc, const float* specIn, const float* specOut, int numBins) {
    if (!specIn || !specOut || numBins < 4) return;
    
    using namespace Layout;
    
    const int x0 = SpectrumX + 35;
    const int y0 = SpectrumY + 10;
    const int width = SpectrumW - 45;
    const int height = SpectrumH - 25;
    
    const int interpFactor = 4; // Smoothness factor
    const int outPoints = (numBins - 1) * interpFactor + 1;
    
    // Allocate points
    std::vector<POINT> ptsIn(outPoints);
    std::vector<POINT> ptsOut(outPoints);
    
    // Helper lambda for dB mapping
    auto clampDb = [](float db) -> float {
        if (db < -60.0f) db = -60.0f;
        if (db > 6.0f) db = 6.0f;
        return (db + 60.0f) / 66.0f; // Normalized 0..1
    };
    
    // 1. Calculate Curve Points (Catmull-Rom Smoothing)
    for (int i = 0; i < numBins - 1; ++i) {
        int i0 = (i > 0) ? i - 1 : 0;
        int i1 = i;
        int i2 = i + 1;
        int i3 = (i < numBins - 2) ? i + 2 : numBins - 1;
        
        float xPos0 = (float)i0 / (float)(numBins - 1);
        float xPos1 = (float)i1 / (float)(numBins - 1);
        float xPos2 = (float)i2 / (float)(numBins - 1);
        float xPos3 = (float)i3 / (float)(numBins - 1);
        
        float yIn0 = clampDb(specIn[i0]); float yIn1 = clampDb(specIn[i1]);
        float yIn2 = clampDb(specIn[i2]); float yIn3 = clampDb(specIn[i3]);
        
        float yOut0 = clampDb(specOut[i0]); float yOut1 = clampDb(specOut[i1]);
        float yOut2 = clampDb(specOut[i2]); float yOut3 = clampDb(specOut[i3]);
        
        for (int j = 0; j < interpFactor; ++j) {
            float t = (float)j / (float)interpFactor;
            int idx = i * interpFactor + j;
            
            float xInterp = catmullRom(xPos0, xPos1, xPos2, xPos3, t);
            float yInInterp = catmullRom(yIn0, yIn1, yIn2, yIn3, t);
            float yOutInterp = catmullRom(yOut0, yOut1, yOut2, yOut3, t);
            
            ptsIn[idx].x = x0 + (int)(xInterp * width);
            ptsIn[idx].y = y0 + (int)((1.0f - yInInterp) * height);
            
            ptsOut[idx].x = x0 + (int)(xInterp * width);
            ptsOut[idx].y = y0 + (int)((1.0f - yOutInterp) * height);
        }
    }
    
    // Handle last point
    ptsIn.back().x = x0 + width;
    ptsIn.back().y = y0 + (int)((1.0f - clampDb(specIn[numBins - 1])) * height);
    ptsOut.back().x = x0 + width;
    ptsOut.back().y = y0 + (int)((1.0f - clampDb(specOut[numBins - 1])) * height);

    // 2. Construct Fill Polygons (Add bottom corners)
    auto createFillPoly = [&](const std::vector<POINT>& curvePts) {
        std::vector<POINT> poly = curvePts;
        poly.push_back({ curvePts.back().x, y0 + height }); // Bottom Right
        poly.push_back({ curvePts.front().x, y0 + height }); // Bottom Left
        return poly;
    };

    std::vector<POINT> fillIn = createFillPoly(ptsIn);
    std::vector<POINT> fillOut = createFillPoly(ptsOut);

    // 3. Draw LAYERS (Painter's Algorithm)
    // Order: Input Fill -> Output Fill -> Input Line -> Output Line
    FillAlphaPoly(hdc, fillIn, ELC_SPEC_FILL_IN, 40);
    FillAlphaPoly(hdc, fillOut, ELC_SPEC_FILL_OUT, 60);

    // C. Input Line (Thin, Cyan)
    HPEN inPen = CreatePen(PS_SOLID, 1, ELC_SPEC_INPUT);
    HPEN oldPen = (HPEN)SelectObject(hdc, inPen);
    Polyline(hdc, ptsIn.data(), (int)ptsIn.size());
    SelectObject(hdc, oldPen);
    DeleteObject(inPen);
    
    // D. Output Line (Thicker, Gold)
    HPEN outPen = CreatePen(PS_SOLID, 2, ELC_SPEC_OUTPUT);
    oldPen = (HPEN)SelectObject(hdc, outPen);
    Polyline(hdc, ptsOut.data(), (int)ptsOut.size());
    SelectObject(hdc, oldPen);
    DeleteObject(outPen);
}

//-------------------------------------------------------------------------------------------------------
// Fill spectrum gradient
//-------------------------------------------------------------------------------------------------------
void HyeokStreamEditor::fillSpectrumGradient(HDC hdc, const POINT* pts, int count, int baseY, 
                                              COLORREF topColor, COLORREF bottomColor) {
    if (count < 2) return;
    
    POINT* fillPts = (POINT*)_alloca(sizeof(POINT) * (count + 2));
    for (int i = 0; i < count; ++i) {
        fillPts[i] = pts[i];
    }
    fillPts[count] = { pts[count - 1].x, baseY };
    fillPts[count + 1] = { pts[0].x, baseY };
    
    HBRUSH fillBrush = CreateSolidBrush(topColor);
    HPEN nullPen = CreatePen(PS_NULL, 0, 0);
    HPEN oldPen = (HPEN)SelectObject(hdc, nullPen);
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, fillBrush);
    
    Polygon(hdc, fillPts, count + 2);
    
    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBrush);
    DeleteObject(nullPen);
    DeleteObject(fillBrush);
}

//-------------------------------------------------------------------------------------------------------
// Crossover markers - Now draggable lines
//-------------------------------------------------------------------------------------------------------
void HyeokStreamEditor::drawCrossoverMarkers(HDC hdc, float xo1, float xo2, float xo3) {
    drawCrossoverDragLines(hdc);
}

//-------------------------------------------------------------------------------------------------------
// Crossover draggable lines - Draw vertical lines that can be dragged
//-------------------------------------------------------------------------------------------------------
void HyeokStreamEditor::drawCrossoverDragLines(HDC hdc) {
    using namespace Layout;
    
    HyeokStreamMaster* plugin = getPlugin();
    if (!plugin) return;
    
    const int x0 = SpectrumX + 35;
    const int y0 = SpectrumY + 10;
    const int width = SpectrumW - 45;
    const int height = SpectrumH - 25;
    
    float freqs[3] = {
        plugin->getCrossoverFreq(0),
        plugin->getCrossoverFreq(1),
        plugin->getCrossoverFreq(2)
    };
    
    COLORREF colors[3] = { ELC_BAND_LOW, ELC_BAND_LOWMID, ELC_BAND_HIMID };
    const char* labels[3] = { "XO1", "XO2", "XO3" };
    
    for (int i = 0; i < 3; ++i) {
        float freq = freqs[i];
        if (freq < 20.0f || freq > 20000.0f) continue;
        
        float logPos = (log10f(freq) - log10f(20.0f)) / (log10f(20000.0f) - log10f(20.0f));
        int x = x0 + (int)(logPos * width);
        
        // Draw draggable line
        bool isActive = (activeCrossover == i);
        int lineWidth = isActive ? 3 : 2;
        COLORREF lineColor = isActive ? ELC_GOLD_BRIGHT : colors[i];
        
        HPEN linePen = CreatePen(PS_SOLID, lineWidth, lineColor);
        HPEN oldPen = (HPEN)SelectObject(hdc, linePen);
        MoveToEx(hdc, x, y0, nullptr);
        LineTo(hdc, x, y0 + height);
        SelectObject(hdc, oldPen);
        DeleteObject(linePen);
        
        // Draw drag handle (triangle/arrow at bottom)
        POINT handle[3];
        handle[0] = { x, y0 + height + 2 };
        handle[1] = { x - 8, y0 + height + 14 };
        handle[2] = { x + 8, y0 + height + 14 };
        
        HBRUSH handleBrush = CreateSolidBrush(isActive ? ELC_GOLD_BRIGHT : lineColor);
        HPEN handlePen = CreatePen(PS_SOLID, 1, isActive ? ELC_GOLD_BRIGHT : lineColor);
        oldPen = (HPEN)SelectObject(hdc, handlePen);
        HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, handleBrush);
        Polygon(hdc, handle, 3);
        SelectObject(hdc, oldPen);
        SelectObject(hdc, oldBrush);
        DeleteObject(handleBrush);
        DeleteObject(handlePen);
        
        // Draw frequency value
        char freqText[16];
        if (freq >= 1000.0f) {
            sprintf(freqText, "%.1fk", freq / 1000.0f);
        } else {
            sprintf(freqText, "%.0f", freq);
        }
        
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, isActive ? ELC_GOLD_BRIGHT : lineColor);
        SelectObject(hdc, smallFont);
        
        RECT freqRect = { x - 25, y0 + height + 16, x + 25, y0 + height + 32 };
        DrawTextA(hdc, freqText, -1, &freqRect, DT_CENTER | DT_SINGLELINE);
    }
}

//-------------------------------------------------------------------------------------------------------
// Band section - NEW LAYOUT: GR meter + THR fader + MK fader per band
//-------------------------------------------------------------------------------------------------------
void HyeokStreamEditor::drawBandSection(HDC hdc) {
    using namespace Layout;
    
    HyeokStreamMaster* plugin = getPlugin();
    if (!plugin) return;
    
    const char* bandNames[] = { "LOW", "LO-MID", "HI-MID", "HIGH" };
    COLORREF bandColors[] = { ELC_BAND_LOW, ELC_BAND_LOWMID, ELC_BAND_HIMID, ELC_BAND_HIGH };
    
    for (int i = 0; i < 4; ++i) {
        int bx = BandStartX + i * BandWidth;
        
        // Band header background
        RECT labelBg = { bx + 2, BandSectionY, bx + BandWidth - 2, BandSectionY + BandTitleH };
        
        HBRUSH shadowBrush = CreateSolidBrush(RGB(18, 18, 22));
        RECT shadowRect = { labelBg.left + 1, labelBg.top + 1, labelBg.right + 1, labelBg.bottom + 1 };
        FillRect(hdc, &shadowRect, shadowBrush);
        DeleteObject(shadowBrush);
        
        HBRUSH bgBrush = CreateSolidBrush(RGB(32, 32, 38));
        FillRect(hdc, &labelBg, bgBrush);
        DeleteObject(bgBrush);
        
        HPEN highlightPen = CreatePen(PS_SOLID, 1, RGB(50, 50, 58));
        HPEN oldPen = (HPEN)SelectObject(hdc, highlightPen);
        MoveToEx(hdc, labelBg.left, labelBg.top, nullptr);
        LineTo(hdc, labelBg.right, labelBg.top);
        SelectObject(hdc, oldPen);
        DeleteObject(highlightPen);
        
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, bandColors[i]);
        SelectObject(hdc, titleFont);
        
        RECT nameRect = { bx, BandSectionY + 3, bx + BandWidth, BandSectionY + 20 };
        DrawTextA(hdc, bandNames[i], -1, &nameRect, DT_CENTER | DT_SINGLELINE);
        
        // ===== GR Meter (vertical, left side) =====
        int grX = bx + GRMeterOffsetX;
        int grY = BandSectionY + GRMeterOffsetY;
        float grDb = plugin->getBandGrDb(i);
        
        // GR meter background
        RECT grRect = { grX, grY, grX + GRMeterW, grY + GRMeterH };
        HBRUSH grBgBrush = CreateSolidBrush(RGB(12, 12, 15));
        FillRect(hdc, &grRect, grBgBrush);
        DeleteObject(grBgBrush);
        
        // GR meter fill (from top down)
        if (grDb > 0.1f) {
            float grNorm = grDb / 24.0f;
            if (grNorm > 1.0f) grNorm = 1.0f;
            int filled = (int)(grNorm * GRMeterH);
            RECT grFillRect = { grX + 1, grY + 1, grX + GRMeterW - 1, grY + 1 + filled };
            HBRUSH grFillBrush = CreateSolidBrush(bandColors[i]);
            FillRect(hdc, &grFillRect, grFillBrush);
            DeleteObject(grFillBrush);
        }
        
        // GR meter border
        HPEN grBorderPen = CreatePen(PS_SOLID, 1, ELC_BORDER);
        oldPen = (HPEN)SelectObject(hdc, grBorderPen);
        HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
        Rectangle(hdc, grRect.left, grRect.top, grRect.right, grRect.bottom);
        SelectObject(hdc, oldPen);
        SelectObject(hdc, oldBrush);
        DeleteObject(grBorderPen);
        
        // GR label above meter
        SetTextColor(hdc, ELC_TEXT_DIM);
        SelectObject(hdc, smallFont);
        RECT grLabelRect = { grX - 4, grY - FontMargin, grX + GRMeterW + 4, grY - 2 };
        DrawTextA(hdc, "GR", -1, &grLabelRect, DT_CENTER | DT_SINGLELINE);
        
        // GR value below meter
        char grText[16];
        sprintf(grText, "%.1f", grDb > 0.1f ? -grDb : 0.0f);
        SetTextColor(hdc, grDb > 0.1f ? bandColors[i] : ELC_TEXT_DIM);
        RECT grValRect = { grX - 8, grY + GRMeterH + 2, grX + GRMeterW + 8, grY + GRMeterH + FontMargin };
        DrawTextA(hdc, grText, -1, &grValRect, DT_CENTER | DT_SINGLELINE);

        // Peak-hold for GR meter (per-band)
        DWORD nowLocal = GetTickCount();
        if (grDb > peakGR[i]) { peakGR[i] = grDb; peakGRTime[i] = nowLocal; }
        else if (peakGRTime[i] != 0 && nowLocal - peakGRTime[i] > kPeakHoldMs) { peakGR[i] = -1000.0f; peakGRTime[i] = 0; }

        if (peakGRTime[i] != 0 && peakGR[i] > -500.0f) {
            float pk = peakGR[i];
            if (pk < 0.0f) pk = 0.0f; if (pk > 24.0f) pk = 24.0f;
            int peakFilled = (int)((pk / 24.0f) * GRMeterH);
            int peakY = grY + 1 + peakFilled;
            HPEN pPen = CreatePen(PS_SOLID, 2, ELC_METER_RED);
            HPEN oldP = (HPEN)SelectObject(hdc, pPen);
            MoveToEx(hdc, grX - 2, peakY, nullptr);
            LineTo(hdc, grX + GRMeterW + 2, peakY);
            SelectObject(hdc, oldP);
            DeleteObject(pPen);

            // Peak value (red) below the value area
            char pkText[16];
            sprintf(pkText, "PK %.1f", peakGR[i]);
            SetTextColor(hdc, ELC_METER_RED);
            SelectObject(hdc, smallFont);
            RECT pkRect = { grX - 20, grY + GRMeterH + 18, grX + GRMeterW + 20, grY + GRMeterH + FontMargin };
            DrawTextA(hdc, pkText, -1, &pkRect, DT_CENTER | DT_SINGLELINE);
        }
    }
}

//-------------------------------------------------------------------------------------------------------
// Draw clickable value box (button-like appearance)
//-------------------------------------------------------------------------------------------------------
void HyeokStreamEditor::drawValueBox(HDC hdc, int x, int y, int w, int h,
                                      const char* label, const char* value, COLORREF accentColor) {
    // Box background (darker, inset look)
    RECT boxRect = { x, y, x + w, y + h };
    
    // Shadow for 3D depth
    RECT shadowRect = { x + 1, y + 1, x + w + 1, y + h + 1 };
    HBRUSH shadowBrush = CreateSolidBrush(RGB(10, 10, 12));
    FillRect(hdc, &shadowRect, shadowBrush);
    DeleteObject(shadowBrush);
    
    // Main background
    HBRUSH bgBrush = CreateSolidBrush(RGB(28, 28, 32));
    FillRect(hdc, &boxRect, bgBrush);
    DeleteObject(bgBrush);
    
    // Border with accent color hint
    HPEN borderPen = CreatePen(PS_SOLID, 1, RGB(55, 55, 62));
    HPEN oldPen = (HPEN)SelectObject(hdc, borderPen);
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
    Rectangle(hdc, boxRect.left, boxRect.top, boxRect.right, boxRect.bottom);
    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBrush);
    DeleteObject(borderPen);
    
    // Top accent line
    HPEN accentPen = CreatePen(PS_SOLID, 1, accentColor);
    oldPen = (HPEN)SelectObject(hdc, accentPen);
    MoveToEx(hdc, x + 2, y, nullptr);
    LineTo(hdc, x + w - 2, y);
    SelectObject(hdc, oldPen);
    DeleteObject(accentPen);
    
    SetBkMode(hdc, TRANSPARENT);
    
    // Label (small, dimmed)
    SetTextColor(hdc, ELC_TEXT_DIM);
    SelectObject(hdc, smallFont);
    RECT labelRect = { x, y + 2, x + w, y + 10 };
    DrawTextA(hdc, label, -1, &labelRect, DT_CENTER | DT_SINGLELINE);
    
    // Value (larger, bright)
    SetTextColor(hdc, ELC_TEXT_BRIGHT);
    SelectObject(hdc, valueFont);
    RECT valueRect = { x, y + 8, x + w, y + h - 1 };
    DrawTextA(hdc, value, -1, &valueRect, DT_CENTER | DT_SINGLELINE);
}

//-------------------------------------------------------------------------------------------------------
// Draw 3D M/S/Delta button with glossy effect
//-------------------------------------------------------------------------------------------------------
void HyeokStreamEditor::drawMSDButton(HDC hdc, int x, int y, int w, int h, 
                                       const char* label, bool active, COLORREF activeColor) {
    // Button colors
    COLORREF bgColor = active ? activeColor : RGB(45, 45, 52);
    COLORREF textColor = active ? RGB(15, 15, 18) : RGB(140, 140, 150);
    
    // Drop shadow (3D depth)
    RECT shadowRect = { x + 1, y + 1, x + w + 1, y + h + 1 };
    HBRUSH shadowBrush = CreateSolidBrush(RGB(12, 12, 15));
    FillRect(hdc, &shadowRect, shadowBrush);
    DeleteObject(shadowBrush);
    
    // Main button body
    RECT btnRect = { x, y, x + w, y + h };
    HBRUSH btnBrush = CreateSolidBrush(bgColor);
    FillRect(hdc, &btnRect, btnBrush);
    DeleteObject(btnBrush);
    
    // 3D border effect
    HPEN lightPen = CreatePen(PS_SOLID, 1, active ? RGB(255, 255, 255) : RGB(70, 70, 80));
    HPEN darkPen = CreatePen(PS_SOLID, 1, active ? RGB(80, 80, 90) : RGB(25, 25, 30));
    HPEN oldPen;
    
    // Top and left highlight
    oldPen = (HPEN)SelectObject(hdc, lightPen);
    MoveToEx(hdc, x, y + h - 1, nullptr);
    LineTo(hdc, x, y);
    LineTo(hdc, x + w, y);
    SelectObject(hdc, oldPen);
    
    // Bottom and right shadow
    oldPen = (HPEN)SelectObject(hdc, darkPen);
    MoveToEx(hdc, x + w - 1, y, nullptr);
    LineTo(hdc, x + w - 1, y + h - 1);
    LineTo(hdc, x - 1, y + h - 1);
    SelectObject(hdc, oldPen);
    
    DeleteObject(lightPen);
    DeleteObject(darkPen);
    
    // Gloss effect - lighter gradient on top half
    if (active) {
        COLORREF glossColor = RGB(
            ELCMIN(255, GetRValue(activeColor) + 60),
            ELCMIN(255, GetGValue(activeColor) + 60),
            ELCMIN(255, GetBValue(activeColor) + 60)
        );
        RECT glossRect = { x + 1, y + 1, x + w - 1, y + h / 2 };
        HBRUSH glossBrush = CreateSolidBrush(glossColor);
        FillRect(hdc, &glossRect, glossBrush);
        DeleteObject(glossBrush);
    } else {
        // Subtle gloss for inactive buttons
        RECT glossRect = { x + 1, y + 1, x + w - 1, y + h / 3 };
        HBRUSH glossBrush = CreateSolidBrush(RGB(55, 55, 62));
        FillRect(hdc, &glossRect, glossBrush);
        DeleteObject(glossBrush);
    }
    
    // Label text
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, textColor);
    SelectObject(hdc, smallFont);
    DrawTextA(hdc, label, -1, &btnRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

//-------------------------------------------------------------------------------------------------------
// Draw Bypass buttons - Below Delta buttons in each band
//-------------------------------------------------------------------------------------------------------
void HyeokStreamEditor::drawBypassButtons(HDC hdc) {
    using namespace Layout;
    
    HyeokStreamMaster* plugin = getPlugin();
    if (!plugin) return;
    
    COLORREF bandColors[] = { ELC_BAND_LOW, ELC_BAND_LOWMID, ELC_BAND_HIMID, ELC_BAND_HIGH };
    
    for (int band = 0; band < 4; ++band) {
        int bx = BandStartX + band * BandWidth;
        int grCenterX = bx + GRMeterOffsetX + GRMeterW / 2;
        int btnY = BandSectionY + BypassBtnOffsetY;
        int startX = grCenterX - BypassBtnW / 2;
        
        bool bypassed = plugin->getBandBypass(band);
        
        COLORREF bgColor = bypassed ? RGB(180, 60, 60) : RGB(45, 45, 52);
        COLORREF textColor = bypassed ? RGB(255, 255, 255) : bandColors[band];
        
        RECT shadowRect = { startX + 1, btnY + 1, startX + BypassBtnW + 1, btnY + BypassBtnH + 1 };
        HBRUSH shadowBrush = CreateSolidBrush(RGB(12, 12, 15));
        FillRect(hdc, &shadowRect, shadowBrush);
        DeleteObject(shadowBrush);
        
        RECT btnRect = { startX, btnY, startX + BypassBtnW, btnY + BypassBtnH };
        HBRUSH btnBrush = CreateSolidBrush(bgColor);
        FillRect(hdc, &btnRect, btnBrush);
        DeleteObject(btnBrush);
        
        HPEN borderPen = CreatePen(PS_SOLID, 1, bypassed ? RGB(220, 80, 80) : RGB(60, 60, 70));
        HPEN oldPen = (HPEN)SelectObject(hdc, borderPen);
        HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
        Rectangle(hdc, btnRect.left, btnRect.top, btnRect.right, btnRect.bottom);
        SelectObject(hdc, oldPen);
        SelectObject(hdc, oldBrush);
        DeleteObject(borderPen);
        
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, textColor);
        SelectObject(hdc, smallFont);
        DrawTextA(hdc, bypassed ? "BYP" : "ON", -1, &btnRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }
    
    // M/S Mode buttons (below bypass for each band)
    for (int band = 0; band < 4; ++band) {
        int bx = BandStartX + band * BandWidth;
        int grCenterX = bx + GRMeterOffsetX + GRMeterW / 2;
        int msY = BandSectionY + MSModeBtnOffsetY;
        int msX = grCenterX - MSModeBtnW / 2;

        bool msMode = (getPlugin()->getParameterValue(kParamBand1Mode + band) > 0.5f);
        drawMSDButton(hdc, msX, msY, MSModeBtnW, MSModeBtnH, "M/S", msMode, ELC_GOLD_PRIMARY);
    }
    
    // Limiter bypass button - centered below limiter section
    // Position: below limiter faders, centered under all three faders
    int limBtnY = BandSectionY + LimBypassOffsetY;
    int limCenterX = LimiterX + (LimiterW / 2);  // Center of limiter section
    int limBtnX = limCenterX - BypassBtnW / 2;
    
    bool limBypassed = plugin->getLimiterBypass();
    COLORREF limBgColor = limBypassed ? RGB(180, 60, 60) : RGB(45, 45, 52);
    COLORREF limTextColor = limBypassed ? RGB(255, 255, 255) : ELC_LIMITER;
    
    RECT limShadowRect = { limBtnX + 1, limBtnY + 1, limBtnX + BypassBtnW + 1, limBtnY + BypassBtnH + 1 };
    HBRUSH limShadowBrush = CreateSolidBrush(RGB(12, 12, 15));
    FillRect(hdc, &limShadowRect, limShadowBrush);
    DeleteObject(limShadowBrush);
    
    RECT limBtnRect = { limBtnX, limBtnY, limBtnX + BypassBtnW, limBtnY + BypassBtnH };
    HBRUSH limBtnBrush = CreateSolidBrush(limBgColor);
    FillRect(hdc, &limBtnRect, limBtnBrush);
    DeleteObject(limBtnBrush);
    
    HPEN limBorderPen = CreatePen(PS_SOLID, 1, limBypassed ? RGB(220, 80, 80) : RGB(60, 60, 70));
    HPEN oldPen = (HPEN)SelectObject(hdc, limBorderPen);
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
    Rectangle(hdc, limBtnRect.left, limBtnRect.top, limBtnRect.right, limBtnRect.bottom);
    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBrush);
    DeleteObject(limBorderPen);
    
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, limTextColor);
    SelectObject(hdc, smallFont);
    DrawTextA(hdc, limBypassed ? "BYP" : "ON", -1, &limBtnRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

//-------------------------------------------------------------------------------------------------------
// Draw M/S/Delta buttons - NEW DESIGN:
// M/S buttons (square) below GR meter
// Delta button (wide rect) below M/S
//-------------------------------------------------------------------------------------------------------
void HyeokStreamEditor::drawBandButtons(HDC hdc) {
    using namespace Layout;
    
    HyeokStreamMaster* plugin = getPlugin();
    if (!plugin) return;
    
    COLORREF bandColors[] = { ELC_BAND_LOW, ELC_BAND_LOWMID, ELC_BAND_HIMID, ELC_BAND_HIGH };
    
    for (int band = 0; band < 4; ++band) {
        int bx = BandStartX + band * BandWidth;
        int grCenterX = bx + GRMeterOffsetX + GRMeterW / 2;
        
        // ===== M/S Buttons (square) below GR meter =====
        int msY = BandSectionY + BtnOffsetY;
        int msStartX = grCenterX - (MSBtnW * 2 + MSBtnGap) / 2;
        
        // [M] Mute button
        bool muted = plugin->getBandMute(band);
        drawMSDButton(hdc, msStartX, msY, MSBtnW, MSBtnH, "M", muted, ELC_BTN_MUTE);
        
        // [S] Solo button
        bool soloed = plugin->getBandSolo(band);
        drawMSDButton(hdc, msStartX + MSBtnW + MSBtnGap, msY, MSBtnW, MSBtnH, "S", soloed, ELC_BTN_SOLO);
        
        // ===== Delta Button (wide rect) below M/S =====
        int deltaY = BandSectionY + DeltaBtnOffsetY;
        int deltaX = grCenterX - DeltaBtnW / 2;
        bool delta = plugin->getBandDelta(band);
        drawMSDButton(hdc, deltaX, deltaY, DeltaBtnW, DeltaBtnH, "DELTA", delta, ELC_BTN_DELTA);
    }
}

//-------------------------------------------------------------------------------------------------------
// Hit test for M/S/Delta buttons - NEW positions (M/S below GR, Delta below M/S)
//-------------------------------------------------------------------------------------------------------
int HyeokStreamEditor::hitTestMSDButton(int mx, int my) {
    using namespace Layout;
    
    for (int band = 0; band < 4; ++band) {
        int bx = BandStartX + band * BandWidth;
        int grCenterX = bx + GRMeterOffsetX + GRMeterW / 2;
        
        // M/S buttons below GR meter
        int msY = BandSectionY + BtnOffsetY;
        int msStartX = grCenterX - (MSBtnW * 2 + MSBtnGap) / 2;
        
        // [M] Mute button
        if (mx >= msStartX && mx <= msStartX + MSBtnW && 
            my >= msY && my <= msY + MSBtnH) {
            return band;  // 0-3 = Mute
        }
        // [S] Solo button
        int soloX = msStartX + MSBtnW + MSBtnGap;
        if (mx >= soloX && mx <= soloX + MSBtnW && 
            my >= msY && my <= msY + MSBtnH) {
            return 4 + band;  // 4-7 = Solo
        }
        
        // [D] Delta button below M/S buttons
        int deltaY = BandSectionY + DeltaBtnOffsetY;
        int deltaX = grCenterX - DeltaBtnW / 2;
        if (mx >= deltaX && mx <= deltaX + DeltaBtnW && 
            my >= deltaY && my <= deltaY + DeltaBtnH) {
            return 8 + band;  // 8-11 = Delta
        }
    }
    
    return -1;
}

//-------------------------------------------------------------------------------------------------------
// Hit test for M/S Mode button (below bypass)
// Returns band index (0-3) or -1
//-------------------------------------------------------------------------------------------------------
int HyeokStreamEditor::hitTestMSModeButton(int mx, int my) {
    using namespace Layout;
    for (int band = 0; band < 4; ++band) {
        int bx = BandStartX + band * BandWidth;
        int grCenterX = bx + GRMeterOffsetX + GRMeterW / 2;
        int msY = BandSectionY + MSModeBtnOffsetY;
        int msX = grCenterX - MSModeBtnW / 2;
        if (mx >= msX && mx <= msX + MSModeBtnW && my >= msY && my <= msY + MSModeBtnH) {
            return band;
        }
    }
    return -1;
}

//-------------------------------------------------------------------------------------------------------
// Hit test for Sidechain controls (button + handle)
// Returns 1 for button, 2 for handle, 0 for none
//-------------------------------------------------------------------------------------------------------
int HyeokStreamEditor::hitTestSidechainControl(int mx, int my) {
    using namespace Layout;
    // SC button area (matches drawSidechainControls)
    int btnX = SCButtonX;
    int btnY = SCButtonY;
    int btnW = SCButtonW;
    int btnH = SCButtonH;
    if (mx >= btnX && mx <= btnX + btnW && my >= btnY && my <= btnY + btnH) return 1;

    // Handle area
    HyeokStreamMaster* plugin = getPlugin();
    if (!plugin) return 0;
    float scNorm = plugin->getParameterValue(kParamSidechainFreq);
    float scFreq = 20.0f * powf(20000.0f / 20.0f, scNorm);
    const int graphX = SpectrumX + 35;
    const int graphW = SpectrumW - 45;
    const int graphY = SpectrumY + 10;
    const int graphH = SpectrumH - 25;

    auto freqToX = [&](float freq)->int {
        float lp = (log10f(freq) - log10f(20.0f)) / (log10f(20000.0f) - log10f(20.0f));
        if (lp < 0.0f) lp = 0.0f; if (lp > 1.0f) lp = 1.0f;
        return graphX + (int)(lp * graphW);
    };

    auto getHpfMagnitude = [&](float freq, float cutoff)->float {
        float f = freq;
        float c = cutoff;
        return (f / sqrtf(f * f + c * c));
    };

    int handleX = freqToX(scFreq);
    float handleMag = getHpfMagnitude(scFreq, scFreq);
    float handleDb = 20.0f * log10f(handleMag + 1.0e-12f);
    const float dbTop = 6.0f;
    const float dbBottom = -90.0f;
    float t = (dbTop - handleDb) / (dbTop - dbBottom);
    if (t < 0.0f) t = 0.0f; if (t > 1.0f) t = 1.0f;
    int handleY = graphY + (int)(t * graphH);

    const int handleR = 8;
    if (mx >= handleX - handleR && mx <= handleX + handleR && my >= handleY - handleR && my <= handleY + handleR) return 2;

    // Knob hit test (control 14)
    if (14 < kNumControls) {
        if (controls[14].hitTest(mx, my)) return 3;
    }
    return 0;
}

//-------------------------------------------------------------------------------------------------------
// Hit test for Bypass buttons - Below Delta buttons in each band
// Limiter bypass is centered below limiter section
//-------------------------------------------------------------------------------------------------------
int HyeokStreamEditor::hitTestBypassButton(int mx, int my) {
    using namespace Layout;
    
    int btnY = BandSectionY + BypassBtnOffsetY;
    
    // Check band bypass buttons
    for (int band = 0; band < 4; ++band) {
        int bx = BandStartX + band * BandWidth;
        int grCenterX = bx + GRMeterOffsetX + GRMeterW / 2;
        int startX = grCenterX - BypassBtnW / 2;
        
        if (mx >= startX && mx <= startX + BypassBtnW && my >= btnY && my <= btnY + BypassBtnH) {
            return band;  // 0-3 = band bypass
        }
    }
    
    // Check limiter bypass button (centered below limiter section)
    int limBtnY = BandSectionY + LimBypassOffsetY;
    int limCenterX = LimiterX + (LimiterW / 2);
    int limBtnX = limCenterX - BypassBtnW / 2;
    if (mx >= limBtnX && mx <= limBtnX + BypassBtnW && my >= limBtnY && my <= limBtnY + BypassBtnH) {
        return 4;  // 4 = limiter bypass
    }
    
    return -1;
}

//-------------------------------------------------------------------------------------------------------
// Hit test for value boxes (double-click input) - For fader value areas
//-------------------------------------------------------------------------------------------------------
int HyeokStreamEditor::hitTestValueArea(int mx, int my) {
    using namespace Layout;
    
    // Check band fader value areas (below faders)
    int valueY = BandSectionY + GRMeterOffsetY + FaderHeight + FontMargin;
    
    for (int band = 0; band < 4; ++band) {
        int bx = BandStartX + band * BandWidth;
        
        // Threshold fader value area
        int thrFaderX = bx + FaderOffsetX;
        if (mx >= thrFaderX - 10 && mx <= thrFaderX + FaderWidth + 10 &&
            my >= valueY && my <= valueY + 16) {
            return band;  // 0-3 = Threshold controls
        }
        
        // Makeup fader value area
        int mkFaderX = thrFaderX + FaderWidth + FaderGap;
        if (mx >= mkFaderX - 10 && mx <= mkFaderX + FaderWidth + 10 &&
            my >= valueY && my <= valueY + 16) {
            return 4 + band;  // 4-7 = Makeup controls
        }
    }
    
    // Also check fader controls directly
    for (int i = 0; i < kNumControls; ++i) {
        const Control& ctrl = controls[i];
        if (ctrl.hitTest(mx, my)) {
            return i;
        }
    }
    
    return -1;
}

//-------------------------------------------------------------------------------------------------------
// Show value input dialog
//-------------------------------------------------------------------------------------------------------
void HyeokStreamEditor::showValueInputDialog(int controlIndex) {
    if (controlIndex < 0 || controlIndex >= kNumControls) return;
    
    HyeokStreamMaster* plugin = getPlugin();
    if (!plugin) return;
    
    const Control& ctrl = controls[controlIndex];
    float currentValue = plugin->getParameterValue(ctrl.paramIndex);
    
    // Convert normalized to display value
    char currentText[32];
    if (ctrl.isFrequency) {
        float freq = 20.0f * powf(1000.0f, currentValue);
        sprintf(currentText, "%.1f", freq);
    } else if (ctrl.paramIndex == kParamLimiterRelease) {
        float ms = 10.0f + currentValue * 490.0f;
        sprintf(currentText, "%.0f", ms);
    } else {
        float db = ctrl.minVal + (ctrl.maxVal - ctrl.minVal) * currentValue;
        sprintf(currentText, "%.1f", db);
    }
    
    // Create input dialog using Windows API
    char inputBuffer[64] = {0};
    strncpy(inputBuffer, currentText, sizeof(inputBuffer) - 1);
    
    // Simple input box using a message box with input
    // For a real implementation, you'd create a proper dialog window
    // Here we use a workaround with a simple prompt
    
    char prompt[128];
    sprintf(prompt, "Enter value for %s:", ctrl.label);
    
    // Use a simple edit control in a temporary window
    HWND editHwnd = CreateWindowExA(
        WS_EX_CLIENTEDGE, "EDIT", currentText,
        WS_POPUP | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
        controls[controlIndex].x + 10, controls[controlIndex].y - 25,
        60, 20,
        hwnd, nullptr,
        (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
        nullptr
    );
    
    if (editHwnd) {
        SetFocus(editHwnd);
        SendMessage(editHwnd, EM_SETSEL, 0, -1);  // Select all text
        
        // Store for later use
        SetWindowLongPtr(editHwnd, GWLP_USERDATA, (LONG_PTR)controlIndex);
    }
}

//-------------------------------------------------------------------------------------------------------
// Limiter section with 3D header
//-------------------------------------------------------------------------------------------------------
void HyeokStreamEditor::drawLimiterSection(HDC hdc) {
    using namespace Layout;
    
    // Header background with 3D inset
    RECT labelBg = { LimiterX + 2, BandSectionY, LimiterX + LimiterW - 2, BandSectionY + 28 };
    
    HBRUSH shadowBrush = CreateSolidBrush(RGB(18, 18, 22));
    RECT shadowRect = { labelBg.left + 1, labelBg.top + 1, labelBg.right + 1, labelBg.bottom + 1 };
    FillRect(hdc, &shadowRect, shadowBrush);
    DeleteObject(shadowBrush);
    
    HBRUSH bgBrush = CreateSolidBrush(RGB(32, 32, 38));
    FillRect(hdc, &labelBg, bgBrush);
    DeleteObject(bgBrush);
    
    // Top highlight
    HPEN highlightPen = CreatePen(PS_SOLID, 1, RGB(50, 50, 58));
    HPEN oldPen = (HPEN)SelectObject(hdc, highlightPen);
    MoveToEx(hdc, labelBg.left, labelBg.top, nullptr);
    LineTo(hdc, labelBg.right, labelBg.top);
    SelectObject(hdc, oldPen);
    DeleteObject(highlightPen);
    
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, ELC_LIMITER);
    SelectObject(hdc, titleFont);
    
    RECT titleRect = { LimiterX, BandSectionY + 5, LimiterX + LimiterW, BandSectionY + 26 };
    DrawTextA(hdc, "LIMITER", -1, &titleRect, DT_CENTER | DT_SINGLELINE);
    
    HPEN accentPen = CreatePen(PS_SOLID, 2, ELC_LIMITER);
    oldPen = (HPEN)SelectObject(hdc, accentPen);
    MoveToEx(hdc, LimiterX + 15, BandSectionY + 30, nullptr);
    LineTo(hdc, LimiterX + LimiterW - 15, BandSectionY + 30);
    SelectObject(hdc, oldPen);
    DeleteObject(accentPen);

    // Draw a small GR meter for limiter with peak marker support
    HyeokStreamMaster* plugin = getPlugin();
    if (plugin) {
        int limMeterX = LimiterX + 18;
        int limMeterY = BandSectionY + GRMeterOffsetY;
        float lgr = plugin->getLimiterGrDb();
        drawVerticalMeter(hdc, limMeterX, limMeterY, GRMeterW, GRMeterH, lgr, ELC_LIMITER, "GR", true);

        // Update limiter GR peak
        DWORD nowLocal = GetTickCount();
        if (lgr > peakLimiterGR) { peakLimiterGR = lgr; peakLimiterGRTime = nowLocal; }
        else if (peakLimiterGRTime != 0 && nowLocal - peakLimiterGRTime > kPeakHoldMs) { peakLimiterGR = -1000.0f; peakLimiterGRTime = 0; }

        if (peakLimiterGRTime != 0 && peakLimiterGR > -500.0f) {
            int pkFilled = (int)((peakLimiterGR / 24.0f) * GRMeterH);
            int pkY = limMeterY + 1 + pkFilled;
            HPEN pPen = CreatePen(PS_SOLID, 2, ELC_METER_RED);
            HPEN oldP = (HPEN)SelectObject(hdc, pPen);
            MoveToEx(hdc, limMeterX - 2, pkY, nullptr);
            LineTo(hdc, limMeterX + GRMeterW + 2, pkY);
            SelectObject(hdc, oldP);
            DeleteObject(pPen);

            char pkText[32]; sprintf(pkText, "PK %.1f", peakLimiterGR);
            SetTextColor(hdc, ELC_METER_RED);
            SelectObject(hdc, smallFont);
            RECT pkRect = { limMeterX - 6, limMeterY + GRMeterH + 12, limMeterX + GRMeterW + 6, limMeterY + GRMeterH + FontMargin };
            DrawTextA(hdc, pkText, -1, &pkRect, DT_CENTER | DT_SINGLELINE);
        }
    }
}

//-------------------------------------------------------------------------------------------------------
// Meter section - NEW DESIGN:
// Level meters (IN/OUT) at top-right
// GR meters below with M/S buttons above and Delta buttons below
//-------------------------------------------------------------------------------------------------------
void HyeokStreamEditor::drawMeterSection(HDC hdc) {
    using namespace Layout;
    
    HyeokStreamMaster* plugin = getPlugin();
    if (!plugin) return;
    
    // ===== IN/OUT Meters Panel (far right) =====
    int panelX = IOSectionX - 5;
    int panelY = BandSectionY;
    int panelW = IOMeterW * 2 + IOMeterGap + 20;
    int panelH = IOMeterH + 60;
    
    RECT levelPanel = { panelX, panelY, panelX + panelW, panelY + panelH };
    HBRUSH panelBrush = CreateSolidBrush(RGB(22, 22, 26));
    FillRect(hdc, &levelPanel, panelBrush);
    DeleteObject(panelBrush);
    
    // 3D inset border
    HPEN darkPen = CreatePen(PS_SOLID, 1, RGB(12, 12, 15));
    HPEN lightPen = CreatePen(PS_SOLID, 1, RGB(50, 50, 58));
    HPEN oldPen;
    
    oldPen = (HPEN)SelectObject(hdc, darkPen);
    MoveToEx(hdc, levelPanel.left, levelPanel.bottom - 1, nullptr);
    LineTo(hdc, levelPanel.left, levelPanel.top);
    LineTo(hdc, levelPanel.right, levelPanel.top);
    SelectObject(hdc, oldPen);
    
    oldPen = (HPEN)SelectObject(hdc, lightPen);
    MoveToEx(hdc, levelPanel.right - 1, levelPanel.top, nullptr);
    LineTo(hdc, levelPanel.right - 1, levelPanel.bottom - 1);
    LineTo(hdc, levelPanel.left, levelPanel.bottom - 1);
    SelectObject(hdc, oldPen);
    
    DeleteObject(darkPen);
    DeleteObject(lightPen);
    
    // Title
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, ELC_TEXT_NORMAL);
    SelectObject(hdc, labelFont);
    RECT titleRect = { panelX, panelY + 8, panelX + panelW, panelY + 24 };
    DrawTextA(hdc, "LEVEL", -1, &titleRect, DT_CENTER | DT_SINGLELINE);
    
    // Draw IN meter
    int inX = IOSectionX;
    int meterY = panelY + 32;
    drawVerticalMeter(hdc, inX, meterY, IOMeterW, IOMeterH, plugin->getInputDb(), ELC_SPEC_INPUT, "IN", false);
    
    // Draw OUT meter
    int outX = inX + IOMeterW + IOMeterGap;
    drawVerticalMeter(hdc, outX, meterY, IOMeterW, IOMeterH, plugin->getOutputDb(), ELC_SPEC_OUTPUT, "OUT", false);
    
    // Value displays below meters
    SetTextColor(hdc, ELC_TEXT_DIM);
    SelectObject(hdc, smallFont);
    
    char inText[16], outText[16];
    sprintf(inText, "%.1f", plugin->getInputDb());
    sprintf(outText, "%.1f", plugin->getOutputDb());
    
    int valY = meterY + IOMeterH + FontMargin;
    // Lower the value text a bit to avoid overlapping the meter graphics
    valY += 6;
    RECT inValRect = { inX - 5, valY, inX + IOMeterW + 5, valY + 14 };
    RECT outValRect = { outX - 5, valY, outX + IOMeterW + 5, valY + 14 };
    
    DrawTextA(hdc, inText, -1, &inValRect, DT_CENTER | DT_SINGLELINE);
    DrawTextA(hdc, outText, -1, &outValRect, DT_CENTER | DT_SINGLELINE);

    // Update and draw peak-hold markers for IN/OUT
    DWORD now = GetTickCount();
    float inDb = plugin->getInputDb();
    float outDb = plugin->getOutputDb();
    // IN
    if (inDb > peakIO[0]) { peakIO[0] = inDb; peakIOTime[0] = now; }
    else if (peakIOTime[0] != 0 && now - peakIOTime[0] > kPeakHoldMs) { peakIO[0] = -1000.0f; peakIOTime[0] = 0; }
    // OUT
    if (outDb > peakIO[1]) { peakIO[1] = outDb; peakIOTime[1] = now; }
    else if (peakIOTime[1] != 0 && now - peakIOTime[1] > kPeakHoldMs) { peakIO[1] = -1000.0f; peakIOTime[1] = 0; }

    // Draw peak markers (small red horizontal line on meter) and value text
    auto drawPeakOnMeter = [&](int meterX, float peakVal) {
        if (peakVal < -500.0f) return;
        // Map db to y coordinate (same mapping as drawVerticalMeter)
        float db = peakVal;
        if (db < -60.0f) db = -60.0f; if (db > 0.0f) db = 0.0f;
        float norm = (db + 60.0f) / 60.0f;
        int py = meterY + IOMeterH - (int)(norm * IOMeterH);
        HPEN pPen = CreatePen(PS_SOLID, 2, ELC_METER_RED);
        HPEN old = (HPEN)SelectObject(hdc, pPen);
        MoveToEx(hdc, meterX - 2, py, nullptr);
        LineTo(hdc, meterX + IOMeterW + 2, py);
        SelectObject(hdc, old);
        DeleteObject(pPen);

        // Peak value text to the right of panel
        char ptxt[16];
        sprintf(ptxt, "%.1f", peakVal);
        SetTextColor(hdc, ELC_METER_RED);
        SelectObject(hdc, smallFont);
        RECT pValRect = { meterX + IOMeterW + 12, meterY + 2, meterX + IOMeterW + 80, meterY + 16 };
        DrawTextA(hdc, ptxt, -1, &pValRect, DT_LEFT | DT_SINGLELINE);
    };

    drawPeakOnMeter(inX, peakIO[0]);
    drawPeakOnMeter(outX, peakIO[1]);
}

//-------------------------------------------------------------------------------------------------------
// Vertical meter
//-------------------------------------------------------------------------------------------------------
void HyeokStreamEditor::drawVerticalMeter(HDC hdc, int x, int y, int w, int h, float db, 
                                           COLORREF color, const char* label, bool isGR) {
    RECT track = { x, y, x + w, y + h };
    HBRUSH trackBrush = CreateSolidBrush(RGB(12, 12, 15));
    FillRect(hdc, &track, trackBrush);
    DeleteObject(trackBrush);
    
    float norm;
    if (isGR) {
        if (db < 0.0f) db = 0.0f;
        if (db > 24.0f) db = 24.0f;
        norm = db / 24.0f;
        int filled = (int)(norm * h);
        RECT r = { x + 1, y + 1, x + w - 1, y + 1 + filled };
        HBRUSH b = CreateSolidBrush(color);
        FillRect(hdc, &r, b);
        DeleteObject(b);
    } else {
        if (db < -60.0f) db = -60.0f;
        if (db > 0.0f) db = 0.0f;
        norm = (db + 60.0f) / 60.0f;
        int filled = (int)(norm * h);
        RECT r = { x + 1, y + h - filled, x + w - 1, y + h - 1 };
        
        COLORREF barColor = color;
        if (db > -6.0f) barColor = ELC_METER_RED;
        else if (db > -18.0f) barColor = ELC_METER_YELLOW;
        
        HBRUSH b = CreateSolidBrush(barColor);
        FillRect(hdc, &r, b);
        DeleteObject(b);
    }
    
    HPEN pen = CreatePen(PS_SOLID, 1, ELC_BORDER);
    HPEN oldPen = (HPEN)SelectObject(hdc, pen);
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
    Rectangle(hdc, track.left, track.top, track.right, track.bottom);
    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBrush);
    DeleteObject(pen);
    
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, ELC_TEXT_DIM);
    SelectObject(hdc, smallFont);
    RECT labelRect = { x - 5, y + h + 3, x + w + 5, y + h + 15 };
    DrawTextA(hdc, label, -1, &labelRect, DT_CENTER | DT_SINGLELINE);
}

//-------------------------------------------------------------------------------------------------------
// Draw control
//-------------------------------------------------------------------------------------------------------
void HyeokStreamEditor::drawControl(HDC hdc, const Control& ctrl, float value) {
    if (ctrl.type == CTRL_KNOB) {
        drawKnob(hdc, ctrl, value);
    } else if (ctrl.type == CTRL_FADER) {
        drawFader(hdc, ctrl, value);
    } else if (ctrl.type == CTRL_SLIDER) {
        drawSlider(hdc, ctrl, value);
    } else {
        drawFader(hdc, ctrl, value);
    }
}

//-------------------------------------------------------------------------------------------------------
// Rotary knob
//-------------------------------------------------------------------------------------------------------
void HyeokStreamEditor::drawKnob(HDC hdc, const Control& ctrl, float value) {
    int cx = ctrl.x + ctrl.width / 2;
    int cy = ctrl.y + ctrl.height / 2;
    int radius = ctrl.width / 2 - 2;
    
    HPEN ringPen = CreatePen(PS_SOLID, 3, ctrl.color);
    HPEN oldPen = (HPEN)SelectObject(hdc, ringPen);
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
    
    Arc(hdc, cx - radius, cy - radius, cx + radius, cy + radius,
        cx - radius, cy + radius/2, cx + radius, cy + radius/2);
    
    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBrush);
    DeleteObject(ringPen);
    
    int innerRadius = radius - 6;
    HBRUSH bodyBrush = CreateSolidBrush(RGB(50, 50, 58));
    oldBrush = (HBRUSH)SelectObject(hdc, bodyBrush);
    Ellipse(hdc, cx - innerRadius, cy - innerRadius, cx + innerRadius, cy + innerRadius);
    SelectObject(hdc, oldBrush);
    DeleteObject(bodyBrush);
    
    float angle = ctrl.getKnobAngle(value) * (float)M_PI / 180.0f;
    int ix = cx + (int)(sinf(angle) * (innerRadius - 4));
    int iy = cy - (int)(cosf(angle) * (innerRadius - 4));
    
    HPEN indPen = CreatePen(PS_SOLID, 3, ctrl.color);
    oldPen = (HPEN)SelectObject(hdc, indPen);
    MoveToEx(hdc, cx, cy, nullptr);
    LineTo(hdc, ix, iy);
    SelectObject(hdc, oldPen);
    DeleteObject(indPen);
    
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, ELC_TEXT_NORMAL);
    SelectObject(hdc, labelFont);
    RECT labelRect = { ctrl.x - 10, ctrl.y + ctrl.height + 5, ctrl.x + ctrl.width + 10, ctrl.y + ctrl.height + 20 };
    DrawTextA(hdc, ctrl.label, -1, &labelRect, DT_CENTER | DT_SINGLELINE);
}

//-------------------------------------------------------------------------------------------------------
// Glossy 3D Fader with inset track and premium thumb
//-------------------------------------------------------------------------------------------------------
void HyeokStreamEditor::drawFader(HDC hdc, const Control& ctrl, float value) {
    int trackWidth = 6;
    int trackX = ctrl.x + (ctrl.width - trackWidth) / 2;
    
    // Track inset shadow (3D depth)
    RECT shadowRect = { trackX - 1, ctrl.y - 1, trackX + trackWidth + 1, ctrl.y + ctrl.height + 1 };
    HBRUSH shadowBrush = CreateSolidBrush(RGB(8, 8, 10));
    FillRect(hdc, &shadowRect, shadowBrush);
    DeleteObject(shadowBrush);
    
    // Track body (inset look)
    RECT trackRect = { trackX, ctrl.y, trackX + trackWidth, ctrl.y + ctrl.height };
    HBRUSH trackBrush = CreateSolidBrush(RGB(15, 15, 18));
    FillRect(hdc, &trackRect, trackBrush);
    DeleteObject(trackBrush);
    
    // Track 3D inset border
    HPEN darkPen = CreatePen(PS_SOLID, 1, RGB(5, 5, 7));
    HPEN lightPen = CreatePen(PS_SOLID, 1, RGB(35, 35, 42));
    HPEN oldPen;
    
    oldPen = (HPEN)SelectObject(hdc, darkPen);
    MoveToEx(hdc, trackX + trackWidth, ctrl.y, nullptr);
    LineTo(hdc, trackX, ctrl.y);
    LineTo(hdc, trackX, ctrl.y + ctrl.height);
    SelectObject(hdc, oldPen);
    
    oldPen = (HPEN)SelectObject(hdc, lightPen);
    MoveToEx(hdc, trackX, ctrl.y + ctrl.height - 1, nullptr);
    LineTo(hdc, trackX + trackWidth, ctrl.y + ctrl.height - 1);
    LineTo(hdc, trackX + trackWidth, ctrl.y - 1);
    SelectObject(hdc, oldPen);
    
    DeleteObject(darkPen);
    DeleteObject(lightPen);
    
    // Bipolar center line
    if (ctrl.isBipolar) {
        HPEN centerPen = CreatePen(PS_SOLID, 1, RGB(80, 80, 90));
        oldPen = (HPEN)SelectObject(hdc, centerPen);
        int centerY = ctrl.y + ctrl.height / 2;
        MoveToEx(hdc, ctrl.x + 2, centerY, nullptr);
        LineTo(hdc, ctrl.x + ctrl.width - 2, centerY);
        SelectObject(hdc, oldPen);
        DeleteObject(centerPen);
    }
    
    // Value fill in track
    int thumbY = ctrl.getThumbY(value);
    RECT fillRect;
    
    if (ctrl.isBipolar) {
        int centerY = ctrl.y + ctrl.height / 2;
        if (thumbY < centerY) {
            fillRect = { trackX + 1, thumbY, trackX + trackWidth - 1, centerY };
        } else {
            fillRect = { trackX + 1, centerY, trackX + trackWidth - 1, thumbY };
        }
    } else {
        fillRect = { trackX + 1, thumbY, trackX + trackWidth - 1, ctrl.y + ctrl.height - 1 };
    }
    
    HBRUSH fillBrush = CreateSolidBrush(ctrl.color);
    FillRect(hdc, &fillRect, fillBrush);
    DeleteObject(fillBrush);
    
    // Thumb dimensions
    int thumbHeight = 12;
    int thumbWidth = ctrl.width - 6;
    int thumbX = ctrl.x + 3;
    
    // Thumb drop shadow
    RECT thumbShadow = { thumbX + 2, thumbY - thumbHeight/2 + 2, thumbX + thumbWidth + 2, thumbY + thumbHeight/2 + 2 };
    HBRUSH thumbShadowBrush = CreateSolidBrush(RGB(8, 8, 10));
    FillRect(hdc, &thumbShadow, thumbShadowBrush);
    DeleteObject(thumbShadowBrush);
    
    // Thumb body with gradient-like effect (darker at bottom)
    RECT thumbRect = { thumbX, thumbY - thumbHeight/2, thumbX + thumbWidth, thumbY + thumbHeight/2 };
    
    // Darker bottom half
    RECT thumbBottom = { thumbX, thumbY, thumbX + thumbWidth, thumbY + thumbHeight/2 };
    COLORREF darkColor = RGB(
        ELCMAX(0, GetRValue(ctrl.color) - 40),
        ELCMAX(0, GetGValue(ctrl.color) - 40),
        ELCMAX(0, GetBValue(ctrl.color) - 40)
    );
    HBRUSH thumbBottomBrush = CreateSolidBrush(darkColor);
    FillRect(hdc, &thumbBottom, thumbBottomBrush);
    DeleteObject(thumbBottomBrush);
    
    // Lighter top half
    RECT thumbTop = { thumbX, thumbY - thumbHeight/2, thumbX + thumbWidth, thumbY };
    HBRUSH thumbTopBrush = CreateSolidBrush(ctrl.color);
    FillRect(hdc, &thumbTop, thumbTopBrush);
    DeleteObject(thumbTopBrush);
    
    // White highlight line on top edge
    HPEN highlightPen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
    oldPen = (HPEN)SelectObject(hdc, highlightPen);
    MoveToEx(hdc, thumbX + 1, thumbY - thumbHeight/2, nullptr);
    LineTo(hdc, thumbX + thumbWidth - 1, thumbY - thumbHeight/2);
    SelectObject(hdc, oldPen);
    DeleteObject(highlightPen);
    
    // Thumb border
    HPEN thumbBorderPen = CreatePen(PS_SOLID, 1, RGB(20, 20, 25));
    oldPen = (HPEN)SelectObject(hdc, thumbBorderPen);
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
    Rectangle(hdc, thumbRect.left, thumbRect.top, thumbRect.right, thumbRect.bottom);
    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBrush);
    DeleteObject(thumbBorderPen);
    
    // Label below fader
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, ELC_TEXT_NORMAL);
    SelectObject(hdc, labelFont);
    RECT labelRect = { ctrl.x - 6, ctrl.y + ctrl.height + 6, ctrl.x + ctrl.width + 6, ctrl.y + ctrl.height + 20 };
    DrawTextA(hdc, ctrl.label, -1, &labelRect, DT_CENTER | DT_SINGLELINE);
}

//-------------------------------------------------------------------------------------------------------
// Horizontal slider (Waves C6 style compact)
//-------------------------------------------------------------------------------------------------------
void HyeokStreamEditor::drawSlider(HDC hdc, const Control& ctrl, float value) {
    // Groove
    int gx = ctrl.x;
    int gy = ctrl.y + ctrl.height / 2 - 3;
    int gw = ctrl.width;
    int gh = 6;

    RECT groove = { gx, gy, gx + gw, gy + gh };
    HBRUSH grooveBrush = CreateSolidBrush(RGB(18, 18, 22));
    FillRect(hdc, &groove, grooveBrush);
    DeleteObject(grooveBrush);

    // Active fill up to value
    int fillW = gx + (int)(value * (float)gw);
    if (fillW > gx) {
        RECT fillRect = { gx, gy, fillW, gy + gh };
        HBRUSH fillBrush = CreateSolidBrush(ctrl.color);
        FillRect(hdc, &fillRect, fillBrush);
        DeleteObject(fillBrush);
    }

    // Thumb (rectangular)
    int thumbW = 12;
    int thumbH = ctrl.height + 2;
    int thumbCX = gx + (int)(value * (float)gw);
    int thumbL = thumbCX - thumbW/2;
    int thumbT = ctrl.y - (thumbH - ctrl.height)/2;

    // Shadow
    RECT sh = { thumbL + 2, thumbT + 2, thumbL + thumbW + 2, thumbT + thumbH + 2 };
    HBRUSH shBrush = CreateSolidBrush(RGB(8,8,10));
    FillRect(hdc, &sh, shBrush);
    DeleteObject(shBrush);

    RECT thumbRect = { thumbL, thumbT, thumbL + thumbW, thumbT + thumbH };
    HBRUSH thumbBrushTop = CreateSolidBrush(ctrl.color);
    HBRUSH thumbBrushBottom = CreateSolidBrush(RGB(
        ELCMAX(0, GetRValue(ctrl.color)-40),
        ELCMAX(0, GetGValue(ctrl.color)-40),
        ELCMAX(0, GetBValue(ctrl.color)-40)));

    // top half
    RECT topHalf = { thumbRect.left, thumbRect.top, thumbRect.right, thumbRect.top + thumbH/2 };
    FillRect(hdc, &topHalf, thumbBrushTop);
    RECT bottomHalf = { thumbRect.left, thumbRect.top + thumbH/2, thumbRect.right, thumbRect.bottom };
    FillRect(hdc, &bottomHalf, thumbBrushBottom);

    // Border
    HPEN border = CreatePen(PS_SOLID, 1, RGB(20,20,25));
    HPEN oldPen = (HPEN)SelectObject(hdc, border);
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
    Rectangle(hdc, thumbRect.left, thumbRect.top, thumbRect.right, thumbRect.bottom);
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(border);

    DeleteObject(thumbBrushTop);
    DeleteObject(thumbBrushBottom);

    // Label
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, ELC_TEXT_NORMAL);
    SelectObject(hdc, labelFont);
    RECT labelRect = { ctrl.x - 6, ctrl.y + ctrl.height + 6, ctrl.x + ctrl.width + 6, ctrl.y + ctrl.height + 20 };
    DrawTextA(hdc, ctrl.label, -1, &labelRect, DT_CENTER | DT_SINGLELINE);
}

//-------------------------------------------------------------------------------------------------------
// Control value
//-------------------------------------------------------------------------------------------------------
void HyeokStreamEditor::drawControlValue(HDC hdc, const Control& ctrl, float value) {
    char text[32];
    
    if (ctrl.isFrequency) {
        float freq = 20.0f * powf(1000.0f, value);
        if (freq >= 1000.0f) {
            sprintf(text, "%.1fk", freq / 1000.0f);
        } else {
            sprintf(text, "%.0f", freq);
        }
    } else if (ctrl.paramIndex == kParamLimiterRelease) {
        float ms = 10.0f + value * 490.0f;
        sprintf(text, "%.0fms", ms);
    } else {
        float db = ctrl.minVal + (ctrl.maxVal - ctrl.minVal) * value;
        if (db >= 0.0f) {
            sprintf(text, "+%.1f", db);
        } else {
            sprintf(text, "%.1f", db);
        }
    }
    
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, ctrl.color);
    SelectObject(hdc, valueFont);
    
    int valueY = (ctrl.type == CTRL_KNOB) ? ctrl.y - 18 : ctrl.y - 20;
    // Push limiter values slightly lower to avoid overlapping other labels
    if (ctrl.paramIndex == kParamLimiterThresh || ctrl.paramIndex == kParamLimiterCeiling || ctrl.paramIndex == kParamLimiterRelease) {
        valueY += 10;
    }
    RECT valueRect = { ctrl.x - 15, valueY, ctrl.x + ctrl.width + 15, valueY + 16 };
    DrawTextA(hdc, text, -1, &valueRect, DT_CENTER | DT_SINGLELINE);
}

//-------------------------------------------------------------------------------------------------------
// LUFS
//-------------------------------------------------------------------------------------------------------
void HyeokStreamEditor::drawLufs(HDC hdc, float lufs) {
    char text[32];
    sprintf(text, "%.1f LUFS", lufs);
    
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, ELC_GOLD_PRIMARY);
    SelectObject(hdc, titleFont);
    
    RECT lufsRect = { kEditorWidth - 200, 12, kEditorWidth - 20, 38 };
    DrawTextA(hdc, text, -1, &lufsRect, DT_RIGHT | DT_SINGLELINE);

    // Peak-hold for LUFS (red value to the left of the LUFS text)
    DWORD now = GetTickCount();
    if (lufs > peakLUFS) {
        peakLUFS = lufs;
        peakLUFSTime = now;
    } else if (peakLUFSTime != 0 && now - peakLUFSTime > kPeakHoldMs) {
        peakLUFS = -1000.0f;
        peakLUFSTime = 0;
    }

    if (peakLUFSTime != 0 && peakLUFS > -500.0f) {
        char ptext[32];
        sprintf(ptext, "PK %.1f", peakLUFS);
        SetTextColor(hdc, ELC_METER_RED);
        SelectObject(hdc, smallFont);
        RECT pRect = { kEditorWidth - 320, 12, kEditorWidth - 210, 38 };
        DrawTextA(hdc, ptext, -1, &pRect, DT_RIGHT | DT_SINGLELINE);
    }
}

//-------------------------------------------------------------------------------------------------------
// Mouse interaction
//-------------------------------------------------------------------------------------------------------
void HyeokStreamEditor::onMouseDown(int x, int y, bool shiftHeld) {
    using namespace Layout;
    
    activeControl = -1;
    activeCrossover = -1;
    fineMode = shiftHeld;

    // Quick click-to-reset for peaks: IN/OUT meters, LUFS, per-band GR, limiter GR
    HyeokStreamMaster* plugin = getPlugin();
    if (plugin) {
        using namespace Layout;
        // LUFS rect (top-right)
        RECT lufsRect = { kEditorWidth - 200, 12, kEditorWidth - 20, 38 };
        if (x >= lufsRect.left && x <= lufsRect.right && y >= lufsRect.top && y <= lufsRect.bottom) {
            peakLUFS = -1000.0f; peakLUFSTime = 0;
            InvalidateRect(hwnd, nullptr, FALSE);
            return;
        }

        // IN/OUT meters
        int panelX = IOSectionX - 5;
        int panelY = BandSectionY;
        int inX = IOSectionX;
        int meterY = panelY + 32;
        int outX = inX + IOMeterW + IOMeterGap;
        RECT inRect = { inX, meterY, inX + IOMeterW, meterY + IOMeterH };
        RECT outRect = { outX, meterY, outX + IOMeterW, meterY + IOMeterH };
        if (x >= inRect.left && x <= inRect.right && y >= inRect.top && y <= inRect.bottom) {
            peakIO[0] = -1000.0f; peakIOTime[0] = 0; InvalidateRect(hwnd, nullptr, FALSE); return;
        }
        if (x >= outRect.left && x <= outRect.right && y >= outRect.top && y <= outRect.bottom) {
            peakIO[1] = -1000.0f; peakIOTime[1] = 0; InvalidateRect(hwnd, nullptr, FALSE); return;
        }

        // Per-band GR meters
        for (int i = 0; i < 4; ++i) {
            int bx = BandStartX + i * BandWidth;
            int grX = bx + GRMeterOffsetX;
            int grY = BandSectionY + GRMeterOffsetY;
            RECT grRect = { grX, grY, grX + GRMeterW, grY + GRMeterH };
            if (x >= grRect.left && x <= grRect.right && y >= grRect.top && y <= grRect.bottom) {
                peakGR[i] = -1000.0f; peakGRTime[i] = 0; InvalidateRect(hwnd, nullptr, FALSE); return;
            }
        }

        // Limiter GR meter reset
        int limMeterX = LimiterX + 18;
        int limMeterY = BandSectionY + GRMeterOffsetY;
        RECT limRect = { limMeterX, limMeterY, limMeterX + GRMeterW, limMeterY + GRMeterH };
        if (x >= limRect.left && x <= limRect.right && y >= limRect.top && y <= limRect.bottom) {
            peakLimiterGR = -1000.0f; peakLimiterGRTime = 0; InvalidateRect(hwnd, nullptr, FALSE); return;
        }
    }
    
    // First check bypass buttons
    int bypassHit = hitTestBypassButton(x, y);
    if (bypassHit >= 0) {
        HyeokStreamMaster* plugin = getPlugin();
        if (plugin) {
            if (bypassHit < 4) {
                // Band bypass button (0-3)
                plugin->toggleBandBypass(bypassHit);
            } else {
                // Limiter bypass button (4)
                plugin->toggleLimiterBypass();
            }
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return;
    }
    
    // Then check M/S/? buttons
    // Check M/S Mode buttons (below bypass)
    int msModeHit = hitTestMSModeButton(x, y);
    if (msModeHit >= 0) {
        HyeokStreamMaster* plugin = getPlugin();
        if (plugin) {
            int paramIndex = kParamBand1Mode + msModeHit;
            float val = plugin->getParameterValue(paramIndex);
            plugin->setParameterAutomated(paramIndex, val > 0.5f ? 0.0f : 1.0f);
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return;
    }

    // Sidechain button / handle hit
    int scHit = hitTestSidechainControl(x, y);
    if (scHit == 1) {
        HyeokStreamMaster* plugin = getPlugin();
        if (plugin) {
            float val = plugin->getParameterValue(kParamSidechainActive);
            plugin->setParameterAutomated(kParamSidechainActive, val > 0.5f ? 0.0f : 1.0f);
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return;
    } else if (scHit == 2) {
        // Start dragging sidechain handle
        activeSidechainDrag = true;
        scDragStartX = x;
        HyeokStreamMaster* plugin = getPlugin();
        if (plugin) {
            scDragStartValue = plugin->getParameterValue(kParamSidechainFreq);
            plugin->beginEdit(kParamSidechainFreq);
        }
        return;
    } else if (scHit == 3) {
        // Clicked the slider area - allow interaction even when SC is off
        HyeokStreamMaster* plugin = getPlugin();
        if (plugin) {
            activeControl = 14;
            dragStartValue = plugin->getParameterValue(kParamSidechainFreq);
            dragStartX = x;
            plugin->beginEdit(kParamSidechainFreq);
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return;
    }

    int btnHit = hitTestMSDButton(x, y);
    if (btnHit >= 0) {
        HyeokStreamMaster* plugin = getPlugin();
        if (plugin) {
            if (btnHit < 4) {
                // Mute button (0-3)
                plugin->toggleBandMute(btnHit);
            } else if (btnHit < 8) {
                // Solo button (4-7)
                plugin->toggleBandSolo(btnHit - 4);
            } else {
                // Delta button (8-11)
                plugin->toggleBandDelta(btnHit - 8);
            }
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return;
    }
    
    // Check crossover drag lines on spectrum
    if (y >= SpectrumY && y <= SpectrumY + SpectrumH + 35) {
        HyeokStreamMaster* plugin = getPlugin();
        if (plugin) {
            const int x0 = SpectrumX + 35;
            const int width = SpectrumW - 45;
            
            float freqs[3] = {
                plugin->getCrossoverFreq(0),
                plugin->getCrossoverFreq(1),
                plugin->getCrossoverFreq(2)
            };
            
            for (int i = 0; i < 3; ++i) {
                float freq = freqs[i];
                if (freq < 20.0f || freq > 20000.0f) continue;
                
                float logPos = (log10f(freq) - log10f(20.0f)) / (log10f(20000.0f) - log10f(20.0f));
                int lineX = x0 + (int)(logPos * width);
                
                // Check if click is near the crossover line (within 12 pixels)
                if (abs(x - lineX) <= 12) {
                    activeCrossover = i;
                    dragStartX = x;
                    dragStartValue = plugin->getParameterValue(kParamXover1 + i);
                    plugin->beginEdit(kParamXover1 + i);
                    InvalidateRect(hwnd, nullptr, FALSE);
                    return;
                }
            }
        }
    }
    
    // Then check controls (faders, knobs)
    for (int i = 0; i < kNumControls; ++i) {
        // Skip crossover controls (8-10) - they're handled by drag lines
        if (i >= 8 && i <= 10) continue;
        
        if (controls[i].hitTest(x, y)) {
            // allow interaction for sidechain control even if SC is bypassed

            activeControl = i;

            HyeokStreamMaster* plugin = getPlugin();
            if (plugin) {
                dragStartValue = plugin->getParameterValue(controls[i].paramIndex);
                dragStartY = y;

                plugin->beginEdit(controls[i].paramIndex);

                if (controls[i].type == CTRL_FADER) {
                    float newValue = controls[i].valueFromY(y);
                    plugin->setParameterAutomated(controls[i].paramIndex, newValue);
                } else if (controls[i].type == CTRL_SLIDER) {
                    float nx = (float)(x - controls[i].x) / (float)controls[i].width;
                    if (nx < 0.0f) nx = 0.0f; if (nx > 1.0f) nx = 1.0f;
                    plugin->setParameterAutomated(controls[i].paramIndex, nx);
                    // store drag start for fine adjustments
                    dragStartValue = nx;
                    dragStartX = x;
                }
            }

            InvalidateRect(hwnd, nullptr, FALSE);
            break;
        }
    }
}

void HyeokStreamEditor::onMouseMove(int x, int y, bool shiftHeld) {
    using namespace Layout;
    
    // Handle sidechain handle dragging
    if (activeSidechainDrag) {
        HyeokStreamMaster* plugin = getPlugin();
        if (!plugin) return;

        const int graphX = SpectrumX + 35;
        const int graphW = SpectrumW - 45;

        float logPos = (float)(x - graphX) / (float)graphW;
        if (logPos < 0.0f) logPos = 0.0f;
        if (logPos > 1.0f) logPos = 1.0f;

        // Frequency at X (20..500 Hz for sidechain)
        float freq = 20.0f * powf(20000.0f / 20.0f, logPos);
        if (freq < 20.0f) freq = 20.0f;
        if (freq > 20000.0f) freq = 20000.0f; // sidechain HPF limited to 20kHz

        // Map to sidechain normalized range (0..1) using same mapping as drawSidechainControls
        float newNorm = logf(freq / 20.0f) / logf(20000.0f / 20.0f);

        // Apply fine mode
        if (fineMode) {
            float delta = newNorm - scDragStartValue;
            newNorm = scDragStartValue + delta * 0.1f;
        }

        if (newNorm < 0.0f) newNorm = 0.0f;
        if (newNorm > 1.0f) newNorm = 1.0f;

        plugin->setParameterAutomated(kParamSidechainFreq, newNorm);
        InvalidateRect(hwnd, nullptr, FALSE);
        return;
    }

    // Handle crossover drag
    if (activeCrossover >= 0) {
        HyeokStreamMaster* plugin = getPlugin();
        if (!plugin) return;
        
        const int x0 = SpectrumX + 35;
        const int width = SpectrumW - 45;
        
        // Convert X position to frequency (logarithmic)
        float logPos = (float)(x - x0) / (float)width;
        if (logPos < 0.0f) logPos = 0.0f;
        if (logPos > 1.0f) logPos = 1.0f;
        
        // Convert to frequency: 20Hz to 20kHz (log scale)
        float freq = 20.0f * powf(1000.0f, logPos);
        
        // Clamp to reasonable range
        if (freq < 20.0f) freq = 20.0f;
        if (freq > 20000.0f) freq = 20000.0f;
        
        // Convert to normalized value
        float newValue = (log10f(freq) - log10f(20.0f)) / (log10f(20000.0f) - log10f(20.0f));
        
        // Apply fine mode
        if (fineMode) {
            float delta = newValue - dragStartValue;
            newValue = dragStartValue + delta * 0.1f;
        }
        
        if (newValue < 0.0f) newValue = 0.0f;
        if (newValue > 1.0f) newValue = 1.0f;
        
        plugin->setParameterAutomated(kParamXover1 + activeCrossover, newValue);
        InvalidateRect(hwnd, nullptr, FALSE);
        return;
    }
    
    if (activeControl < 0) return;
    
    HyeokStreamMaster* plugin = getPlugin();
    if (!plugin) return;
    
    const Control& ctrl = controls[activeControl];
    float sensitivity = fineMode ? 0.1f : 1.0f;
    
    if (ctrl.type == CTRL_KNOB) {
        int deltaY = dragStartY - y;
        float deltaValue = (float)deltaY / 200.0f * sensitivity;
        float newValue = dragStartValue + deltaValue;
        
        if (newValue < 0.0f) newValue = 0.0f;
        if (newValue > 1.0f) newValue = 1.0f;
        
        plugin->setParameterAutomated(ctrl.paramIndex, newValue);
    } else if (ctrl.type == CTRL_SLIDER) {
        float nx = (float)(x - ctrl.x) / (float)ctrl.width;
        if (nx < 0.0f) nx = 0.0f; if (nx > 1.0f) nx = 1.0f;
        if (fineMode) {
            float delta = nx - dragStartValue;
            nx = dragStartValue + delta * 0.1f;
        }
        plugin->setParameterAutomated(ctrl.paramIndex, nx);
    } else {
        float targetValue = ctrl.valueFromY(y);
        float currentValue = plugin->getParameterValue(ctrl.paramIndex);
        
        float newValue = currentValue + (targetValue - currentValue) * (fineMode ? 0.2f : 0.8f);
        
        plugin->setParameterAutomated(ctrl.paramIndex, newValue);
    }
    
    InvalidateRect(hwnd, nullptr, FALSE);
}

void HyeokStreamEditor::onMouseUp() {
    if (activeCrossover >= 0) {
        HyeokStreamMaster* plugin = getPlugin();
        if (plugin) {
            plugin->endEdit(kParamXover1 + activeCrossover);
        }
        activeCrossover = -1;
    }
    
    if (activeControl >= 0) {
        HyeokStreamMaster* plugin = getPlugin();
        if (plugin) {
            plugin->endEdit(controls[activeControl].paramIndex);
        }
    }
    if (activeSidechainDrag) {
        HyeokStreamMaster* plugin = getPlugin();
        if (plugin) {
            plugin->endEdit(kParamSidechainFreq);
        }
        activeSidechainDrag = false;
    }
    activeControl = -1;
    fineMode = false;
}

void HyeokStreamEditor::onMouseWheel(int delta, bool shiftHeld) {
    HyeokStreamMaster* plugin = getPlugin();
    if (!plugin) return;

    float step = shiftHeld ? 0.001f : 0.01f;

    // If a control is actively being dragged, adjust that control
    if (activeControl >= 0) {
        const Control& ctrl = controls[activeControl];
        float currentValue = plugin->getParameterValue(ctrl.paramIndex);
        float newValue = currentValue + (delta > 0 ? step : -step);
        if (newValue < 0.0f) newValue = 0.0f;
        if (newValue > 1.0f) newValue = 1.0f;
        plugin->setParameterAutomated(ctrl.paramIndex, newValue);
        InvalidateRect(hwnd, nullptr, FALSE);
        return;
    }

    // If no active control, support wheel-on-hover for the sidechain slider
    POINT pt;
    GetCursorPos(&pt);
    ScreenToClient(hwnd, &pt);

    // If no active control, allow wheel-on-hover for any control under cursor
    for (int i = 0; i < kNumControls; ++i) {
        if (!controls[i].hitTest(pt.x, pt.y)) continue;

        // Adjust this control (allow adjusting SC even when SC is off)
        plugin->beginEdit(controls[i].paramIndex);
        float cur = plugin->getParameterValue(controls[i].paramIndex);
        float nv = cur + (delta > 0 ? step : -step);
        if (nv < 0.0f) nv = 0.0f;
        if (nv > 1.0f) nv = 1.0f;
        plugin->setParameterAutomated(controls[i].paramIndex, nv);
        plugin->endEdit(controls[i].paramIndex);
        InvalidateRect(hwnd, nullptr, FALSE);
        return;
    }

    // Otherwise ignore wheel
}

//-------------------------------------------------------------------------------------------------------
// Double-click for value input
//-------------------------------------------------------------------------------------------------------
void HyeokStreamEditor::onDoubleClick(int x, int y) {
    // Check if clicked on a value display area
    int ctrlIndex = hitTestValueArea(x, y);
    if (ctrlIndex >= 0) {
        HyeokStreamMaster* plugin = getPlugin();
        if (!plugin) return;
        
        const Control& ctrl = controls[ctrlIndex];
        float currentValue = plugin->getParameterValue(ctrl.paramIndex);
        
        // Get current display value
        char currentText[32];
        if (ctrl.isFrequency) {
            float freq = 20.0f * powf(1000.0f, currentValue);
            sprintf(currentText, "%.1f", freq);
        } else if (ctrl.paramIndex == kParamLimiterRelease) {
            float ms = 10.0f + currentValue * 490.0f;
            sprintf(currentText, "%.0f", ms);
        } else {
            float db = ctrl.minVal + (ctrl.maxVal - ctrl.minVal) * currentValue;
            sprintf(currentText, "%.1f", db);
        }
        
        // Prompt for new value
        char promptTitle[64];
        sprintf(promptTitle, "Enter %s value", ctrl.label);
        
        char inputBuffer[64];
        strcpy(inputBuffer, currentText);
        
        // Create a simple input dialog
        // Using Windows API directly for a modal input box
        char* result = inputBuffer;
        
        // Simple approach: Use MessageBox with input simulation
        // In a production plugin, you'd create a proper edit control
        int valueY = (ctrl.type == CTRL_KNOB) ? ctrl.y - 18 : ctrl.y - 20;
        
        // Create floating edit control
        HWND editWnd = CreateWindowExA(
            WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
            "EDIT",
            currentText,
            WS_POPUP | WS_VISIBLE | WS_BORDER | ES_CENTER | ES_AUTOHSCROLL,
            0, 0, 70, 22,
            hwnd,
            (HMENU)9999,
            (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
            nullptr
        );
        
        if (editWnd) {
            // Position the edit control over the value area
            POINT pt = { ctrl.x + ctrl.width / 2 - 35, valueY - 2 };
            ClientToScreen(hwnd, &pt);
            SetWindowPos(editWnd, HWND_TOPMOST, pt.x, pt.y, 70, 22, SWP_SHOWWINDOW);
            
            // Set font
            SendMessage(editWnd, WM_SETFONT, (WPARAM)valueFont, TRUE);
            
            // Select all text
            SendMessage(editWnd, EM_SETSEL, 0, -1);
            SetFocus(editWnd);
            
            // Store control index in window property
            SetPropA(editWnd, "ControlIndex", (HANDLE)(intptr_t)ctrlIndex);
            SetPropA(editWnd, "ParentEditor", (HANDLE)this);
            
            // Subclass the edit control to handle Enter/Escape
            SetWindowLongPtr(editWnd, GWLP_USERDATA, (LONG_PTR)ctrlIndex);
            
            // Set a timer to auto-close if focus is lost
            SetTimer(editWnd, 1, 100, [](HWND hWnd, UINT, UINT_PTR, DWORD) {
                if (GetFocus() != hWnd) {
                    // Get the text and apply value
                    char text[64];
                    GetWindowTextA(hWnd, text, sizeof(text));
                    
                    int ctrlIdx = (int)GetWindowLongPtr(hWnd, GWLP_USERDATA);
                    HyeokStreamEditor* editor = (HyeokStreamEditor*)GetPropA(hWnd, "ParentEditor");
                    
                    if (editor && ctrlIdx >= 0 && ctrlIdx < kNumControls) {
                        HyeokStreamMaster* plugin = editor->getPlugin();
                        if (plugin) {
                            const Control& c = editor->controls[ctrlIdx];
                            float inputVal = (float)atof(text);
                            float newNorm = 0.5f;
                            
                            if (c.isFrequency) {
                                // Frequency: convert Hz to normalized
                                if (inputVal < 20.0f) inputVal = 20.0f;
                                if (inputVal > 20000.0f) inputVal = 20000.0f;
                                newNorm = log10f(inputVal / 20.0f) / 3.0f;  // log10(1000) = 3
                            } else if (c.paramIndex == kParamLimiterRelease) {
                                // Release: convert ms to normalized
                                if (inputVal < 10.0f) inputVal = 10.0f;
                                if (inputVal > 500.0f) inputVal = 500.0f;
                                newNorm = (inputVal - 10.0f) / 490.0f;
                            } else {
                                // dB value
                                if (inputVal < c.minVal) inputVal = c.minVal;
                                if (inputVal > c.maxVal) inputVal = c.maxVal;
                                newNorm = (inputVal - c.minVal) / (c.maxVal - c.minVal);
                            }
                            
                            if (newNorm < 0.0f) newNorm = 0.0f;
                            if (newNorm > 1.0f) newNorm = 1.0f;
                            
                            plugin->setParameterAutomated(c.paramIndex, newNorm);
                        }
                    }
                    
                    KillTimer(hWnd, 1);
                    DestroyWindow(hWnd);
                }
            });
        }
    }
}
