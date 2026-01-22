# ELC4L - Elite 4-Band Compressor + Limiter (JUCE Version)

**프리미엄 4밴드 멀티밴드 컴프레서 + 브릭월 리미터 VST/AU 플러그인**

## 지원 포맷

| 플랫폼 | 포맷 | 파일 |
|--------|------|------|
| Windows x64 | VST3 | `ELC4L.vst3` |
| Windows x64 | VST2 | `ELC4L.dll` (SDK 필요) |
| Windows x64 | Standalone | `ELC4L.exe` |
| macOS Universal | VST3 | `ELC4L.vst3` |
| macOS Universal | AU | `ELC4L.component` |
| macOS Universal | VST2 | `ELC4L.vst` (SDK 필요) |
| macOS Universal | Standalone | `ELC4L.app` |

## 요구 사항

### Windows
- Visual Studio 2022 (또는 2019)
- CMake 3.22+
- JUCE 7.x 프레임워크

### macOS
- Xcode 13+
- CMake 3.22+
- JUCE 7.x 프레임워크

## 빌드 방법

### 1. JUCE 설치

JUCE 프레임워크를 다운로드하고 설치합니다:
- https://juce.com/get-juce/download

환경 변수 설정 (선택적):
```bash
# Windows (PowerShell)
$env:JUCE_PATH = "C:\JUCE"

# macOS/Linux
export JUCE_PATH=~/JUCE
```

### 2. VST2 SDK (선택적)

VST2 빌드가 필요한 경우 Steinberg VST2 SDK를 준비하고:
```bash
# Windows
set VST2_SDK_PATH=C:\SDKs\vstsdk2.4

# macOS/Linux
export VST2_SDK_PATH=~/SDKs/vstsdk2.4
```

### 3. 빌드 실행

**Windows:**
```batch
cd ELC4L_JUCE
build_windows.bat
```

**macOS:**
```bash
cd ELC4L_JUCE
chmod +x build_macos.sh
./build_macos.sh
```

### 4. 수동 CMake 빌드

```bash
mkdir build
cd build
cmake -DJUCE_PATH=/path/to/JUCE ..
cmake --build . --config Release
```

## 출력 파일 위치

빌드 완료 후 플러그인 파일은 다음 경로에 생성됩니다:

```
build_win/ELC4L_artefacts/Release/
├── VST3/
│   └── ELC4L.vst3
├── VST/                    (VST2, SDK 필요)
│   └── ELC4L.dll
└── Standalone/
    └── ELC4L.exe

build_mac/ELC4L_artefacts/Release/
├── VST3/
│   └── ELC4L.vst3
├── AU/
│   └── ELC4L.component
├── VST/                    (VST2, SDK 필요)
│   └── ELC4L.vst
└── Standalone/
    └── ELC4L.app
```

## 플러그인 설치

### Windows
- VST3: `C:\Program Files\Common Files\VST3\`
- VST2: DAW의 VST 플러그인 폴더

### macOS
- VST3: `/Library/Audio/Plug-Ins/VST3/` 또는 `~/Library/Audio/Plug-Ins/VST3/`
- AU: `/Library/Audio/Plug-Ins/Components/` 또는 `~/Library/Audio/Plug-Ins/Components/`
- VST2: `/Library/Audio/Plug-Ins/VST/` 또는 `~/Library/Audio/Plug-Ins/VST/`

## AU 유효성 검사 (macOS)

AU 플러그인 설치 후 유효성 검사:
```bash
auval -v aufx ELC4 HyAu
```

## 기능

- **4밴드 멀티밴드 컴프레서**
  - LA-2A 스타일 옵토 컴프레서
  - 밴드별 M/S/Delta/Bypass 컨트롤
  - Linkwitz-Riley 4차 크로스오버

- **브릭월 리미터**
  - Lookahead (~1.5ms)
  - ARC 스타일 듀얼 릴리즈
  - 자동 메이크업 게인

- **고급 미터링**
  - 스펙트럼 분석기 (Pro-Q 스타일)
  - IN/OUT 레벨 미터 + 피크 홀드
  - LUFS 미터
  - 밴드별 GR 미터

- **UI/UX**
  - 프리미엄 골드/다크 테마
  - 안티앨리어싱 그래픽
  - HiDPI/Retina 지원
  - 드래그 가능한 크로스오버 라인

## 라이선스

MIT License - 자세한 내용은 `LICENSE` 파일 참조

## 문의

- Website: https://hyeokaudio.com
- Email: contact@hyeokaudio.com
