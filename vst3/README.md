# ELC4L VST3 (WIP)

이 디렉터리는 VST3 전용 버전으로 전환하기 위한 작업 공간입니다.

## 다음 단계
1. VST3 SDK 경로를 준비합니다. (예: `C:\SDKs\vst3sdk`)
2. 아래 `CMakeLists.txt`의 `VST3_SDK_PATH`를 설정합니다.
3. VST3 기본 스켈레톤(Processor/Controller)을 추가합니다.
4. 기존 VST2 코드에서 DSP/파라미터/에디터를 포팅합니다.

## 참고
- VST3 SDK는 Steinberg 라이선스에 따라 별도 배포됩니다.
- 현재는 스켈레톤만 준비되어 있으며 실제 빌드는 아직 설정되지 않았습니다.
