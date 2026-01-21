//-------------------------------------------------------------------------------------------------------
// ELC4L Editor - Premium Pro-Q Style UI Implementation
// ELBIX 4-Band Compressor + Limiter - Dark & Gold Theme
//-------------------------------------------------------------------------------------------------------

#include "HyeokStreamEditor.h"
#include "HyeokStreamMaster.h"
#include <cstdio>
#include <cmath>
#include <algorithm>

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
    // Font-based spacing for no overlap - INCREASED for more breathing room
    constexpr int FontMargin = 20;      // Increased spacing between elements
    constexpr int Padding = 18;
    
    // ===== WINDOW SIZE (reduced width) =====
    constexpr int WindowW = 980;        // Reduced from 1100
    constexpr int WindowH = 700;
    
    // ===== TOP HALF: SPECTRUM ANALYZER =====
    constexpr int SpectrumX = 15;
    constexpr int SpectrumY = 55;
    constexpr int SpectrumW = 860;      // Reduced width (was 1070)
    constexpr int SpectrumH = 260;      // Slightly reduced height (was 300)
    
    // Crossover drag lines (no knobs - lines on spectrum)
    constexpr int XoverLineY1 = SpectrumY + 20;   // Top of draggable area
    constexpr int XoverLineY2 = SpectrumY + SpectrumH - 10;  // Bottom of draggable area
    constexpr int XoverLineWidth = 8;   // Click area width for drag
    
    // ===== BOTTOM HALF: COMPRESSORS + METERS =====
    constexpr int BandSectionY = SpectrumY + SpectrumH + FontMargin + 15;  // More space, moved down
    constexpr int BandHeight = 310;     // Remaining space
    
    // Each band column (4 bands + limiter + IN/OUT)
    constexpr int BandWidth = 140;      // Reduced from 160 for tighter spacing
    constexpr int BandStartX = 20;      // Slightly moved right
    
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
    
    // Limiter section (after 4 bands) - moved right
    constexpr int LimiterX = BandStartX + BandWidth * 4 + 25;  // More gap before limiter
    constexpr int LimiterY = BandSectionY;
    constexpr int LimiterW = 130;
    
    // Limiter bypass position (centered below limiter meters)
    constexpr int LimBypassOffsetY = GRMeterOffsetY + GRMeterH + FontMargin + 40;  // Lower position
    
    // ===== IN/OUT METERS (moved left from far right) =====
    constexpr int IOSectionX = 875;     // Moved left (was 1000)
    constexpr int IOMeterW = 24;
    constexpr int IOMeterH = 160;
    constexpr int IOMeterOffsetY = BandTitleH + FontMargin;
    constexpr int IOMeterGap = 10;
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
    drawSpectrumCurves(hdc, plugin->getDisplayIn(), plugin->getDisplayOut(), kDisplayBins);
    drawCrossoverMarkers(hdc, plugin->getXover1Hz(), plugin->getXover2Hz(), plugin->getXover3Hz());
    
    drawBandSection(hdc);
    drawBypassButtons(hdc);  // Bypass buttons for bands
    drawLimiterSection(hdc);
    drawMeterSection(hdc);
    drawBandButtons(hdc);  // M/S/Δ buttons - now below meters
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
void HyeokStreamEditor::drawSpectrumCurves(HDC hdc, const float* specIn, const float* specOut, int numBins) {
    if (!specIn || !specOut || numBins < 4) return;
    
    using namespace Layout;
    
    const int x0 = SpectrumX + 35;
    const int y0 = SpectrumY + 10;
    const int width = SpectrumW - 45;
    const int height = SpectrumH - 25;
    
    const int interpFactor = 4;
    const int outPoints = (numBins - 1) * interpFactor + 1;
    
    POINT* ptsIn = (POINT*)_alloca(sizeof(POINT) * outPoints);
    POINT* ptsOut = (POINT*)_alloca(sizeof(POINT) * outPoints);
    
    auto clampDb = [](float db) -> float {
        if (db < -60.0f) db = -60.0f;
        if (db > 0.0f) db = 0.0f;
        return (db + 60.0f) / 60.0f;
    };
    
    for (int i = 0; i < numBins - 1; ++i) {
        int i0 = (i > 0) ? i - 1 : 0;
        int i1 = i;
        int i2 = i + 1;
        int i3 = (i < numBins - 2) ? i + 2 : numBins - 1;
        
        float xPos0 = (float)i0 / (float)(numBins - 1);
        float xPos1 = (float)i1 / (float)(numBins - 1);
        float xPos2 = (float)i2 / (float)(numBins - 1);
        float xPos3 = (float)i3 / (float)(numBins - 1);
        
        float yIn0 = clampDb(specIn[i0]);
        float yIn1 = clampDb(specIn[i1]);
        float yIn2 = clampDb(specIn[i2]);
        float yIn3 = clampDb(specIn[i3]);
        
        float yOut0 = clampDb(specOut[i0]);
        float yOut1 = clampDb(specOut[i1]);
        float yOut2 = clampDb(specOut[i2]);
        float yOut3 = clampDb(specOut[i3]);
        
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
    
    float lastDbIn = specIn[numBins - 1];
    float lastDbOut = specOut[numBins - 1];
    lastDbIn = clampDb(lastDbIn) * 60.0f - 60.0f;
    lastDbOut = clampDb(lastDbOut) * 60.0f - 60.0f;
    
    ptsIn[outPoints - 1].x = x0 + width;
    ptsIn[outPoints - 1].y = y0 + (int)((1.0f - clampDb(specIn[numBins - 1])) * height);
    ptsOut[outPoints - 1].x = x0 + width;
    ptsOut[outPoints - 1].y = y0 + (int)((1.0f - clampDb(specOut[numBins - 1])) * height);
    
    fillSpectrumGradient(hdc, ptsOut, outPoints, y0 + height, ELC_SPEC_FILL_OUT, ELC_BG_SPECTRUM);
    fillSpectrumGradient(hdc, ptsIn, outPoints, y0 + height, ELC_SPEC_FILL_IN, ELC_BG_SPECTRUM);
    
    HPEN inPen = CreatePen(PS_SOLID, 2, ELC_SPEC_INPUT);
    HPEN oldPen = (HPEN)SelectObject(hdc, inPen);
    Polyline(hdc, ptsIn, outPoints);
    SelectObject(hdc, oldPen);
    DeleteObject(inPen);
    
    HPEN outPen = CreatePen(PS_SOLID, 2, ELC_SPEC_OUTPUT);
    oldPen = (HPEN)SelectObject(hdc, outPen);
    Polyline(hdc, ptsOut, outPoints);
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
    RECT inValRect = { inX - 5, valY, inX + IOMeterW + 5, valY + 14 };
    RECT outValRect = { outX - 5, valY, outX + IOMeterW + 5, valY + 14 };
    
    DrawTextA(hdc, inText, -1, &inValRect, DT_CENTER | DT_SINGLELINE);
    DrawTextA(hdc, outText, -1, &outValRect, DT_CENTER | DT_SINGLELINE);
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
}

//-------------------------------------------------------------------------------------------------------
// Mouse interaction
//-------------------------------------------------------------------------------------------------------
void HyeokStreamEditor::onMouseDown(int x, int y, bool shiftHeld) {
    using namespace Layout;
    
    activeControl = -1;
    activeCrossover = -1;
    fineMode = shiftHeld;
    
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
    
    // Then check M/S/Δ buttons
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
            activeControl = i;
            
            HyeokStreamMaster* plugin = getPlugin();
            if (plugin) {
                dragStartValue = plugin->getParameterValue(controls[i].paramIndex);
                dragStartY = y;
                
                plugin->beginEdit(controls[i].paramIndex);
                
                if (controls[i].type == CTRL_FADER) {
                    float newValue = controls[i].valueFromY(y);
                    plugin->setParameterAutomated(controls[i].paramIndex, newValue);
                }
            }
            
            InvalidateRect(hwnd, nullptr, FALSE);
            break;
        }
    }
}

void HyeokStreamEditor::onMouseMove(int x, int y, bool shiftHeld) {
    using namespace Layout;
    
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
    activeControl = -1;
    fineMode = false;
}

void HyeokStreamEditor::onMouseWheel(int delta, bool shiftHeld) {
    if (activeControl < 0) return;
    
    HyeokStreamMaster* plugin = getPlugin();
    if (!plugin) return;
    
    const Control& ctrl = controls[activeControl];
    float step = shiftHeld ? 0.001f : 0.01f;
    float currentValue = plugin->getParameterValue(ctrl.paramIndex);
    float newValue = currentValue + (delta > 0 ? step : -step);
    
    if (newValue < 0.0f) newValue = 0.0f;
    if (newValue > 1.0f) newValue = 1.0f;
    
    plugin->setParameterAutomated(ctrl.paramIndex, newValue);
    InvalidateRect(hwnd, nullptr, FALSE);
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
