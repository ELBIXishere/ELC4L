//-------------------------------------------------------------------------------------------------------
// VST Plug-Ins SDK
// Extended header for VST2 plugin development
//-------------------------------------------------------------------------------------------------------

#pragma once

#ifndef __aeffectx__
#define __aeffectx__

#include "aeffect.h"

//-------------------------------------------------------------------------------------------------------
// Extended string constants
//-------------------------------------------------------------------------------------------------------
enum Vst2StringConstants {
    kVstMaxNameLen       = 64,
    kVstMaxLabelLen      = 64,
    kVstMaxShortLabelLen = 8,
    kVstMaxCategLabelLen = 24,
    kVstMaxFileNameLen   = 100
};

//-------------------------------------------------------------------------------------------------------
// Extended Host to Plugin opcodes
//-------------------------------------------------------------------------------------------------------
enum AEffectXOpcodes {
    effProcessEvents = effSetChunk + 1,     // [ptr]: VstEvents* - MIDI events
    effCanBeAutomated,                      // [index]: can parameter be automated?
    effString2Parameter,                    // Convert string to parameter
    
    effGetNumProgramCategories = 26,
    effGetProgramNameIndexed = 29,          // Get program name by index
    
    effCopyProgram = 30,
    
    effConnectInput = 31,
    effConnectOutput,
    effGetInputProperties,
    effGetOutputProperties,
    effGetPlugCategory,
    
    effGetCurrentPosition = 36,
    effGetDestinationBuffer,
    
    effOfflineNotify = 38,
    effOfflinePrepare,
    effOfflineRun,
    
    effProcessVarIo = 41,
    effSetSpeakerArrangement,
    
    effSetBlockSizeAndSampleRate = 43,
    effSetBypass,                           // [value]: 1=bypass, 0=normal
    effGetEffectName,                       // [ptr]: effect name (kVstMaxEffectNameLen)
    
    effGetErrorText = 46,
    
    effGetVendorString = 47,                // [ptr]: vendor string
    effGetProductString,                    // [ptr]: product string
    effGetVendorVersion,                    // [return]: vendor version
    effVendorSpecific,                      // Vendor-specific
    effCanDo,                               // [ptr]: "canDo" string
    effGetTailSize,                         // [return]: tail size (samples)
    
    effIdle = 53,
    effGetIcon = 54,
    effSetViewPosition = 55,
    
    effGetParameterProperties = 56,
    
    effKeysRequired = 57,
    effGetVstVersion,                       // [return]: VST version
    
    // VST 2.1
    effEditKeyDown = 59,
    effEditKeyUp,
    effSetEditKnobMode,
    
    effGetMidiProgramName = 62,
    effGetCurrentMidiProgram,
    effGetMidiProgramCategory,
    effHasMidiProgramsChanged,
    effGetMidiKeyName,
    
    effBeginSetProgram,
    effEndSetProgram,
    
    // VST 2.3
    effGetSpeakerArrangement = 69,
    effShellGetNextPlugin,
    effStartProcess,
    effStopProcess,
    effSetTotalSampleToProcess,
    effSetPanLaw,
    
    effBeginLoadBank,
    effBeginLoadProgram,
    
    // VST 2.4
    effSetProcessPrecision = 77,
    effGetNumMidiInputChannels,
    effGetNumMidiOutputChannels
};

//-------------------------------------------------------------------------------------------------------
// Extended Plugin to Host opcodes
//-------------------------------------------------------------------------------------------------------
enum AudioMasterOpcodesX {
    audioMasterWantMidi = 6,                // [deprecated]
    audioMasterGetTime,                     // [return]: VstTimeInfo*
    audioMasterProcessEvents,               // [ptr]: VstEvents* MIDI send
    
    audioMasterSetTime = 9,
    audioMasterTempoAt,
    audioMasterGetNumAutomatableParameters,
    audioMasterGetParameterQuantization,
    
    audioMasterIOChanged = 13,              // I/O configuration changed
    
    audioMasterNeedIdle = 14,
    audioMasterSizeWindow,                  // [index]: width, [value]: height
    audioMasterGetSampleRate,
    audioMasterGetBlockSize,
    audioMasterGetInputLatency,
    audioMasterGetOutputLatency,
    
    audioMasterGetPreviousPlug = 20,
    audioMasterGetNextPlug,
    audioMasterWillReplaceOrAccumulate,
    
    audioMasterGetCurrentProcessLevel = 23,
    audioMasterGetAutomationState,
    
    audioMasterOfflineStart = 25,
    audioMasterOfflineRead,
    audioMasterOfflineWrite,
    audioMasterOfflineGetCurrentPass,
    audioMasterOfflineGetCurrentMetaPass,
    
    audioMasterSetOutputSampleRate = 30,
    audioMasterGetOutputSpeakerArrangement,
    
    audioMasterGetVendorString = 32,
    audioMasterGetProductString,
    audioMasterGetVendorVersion,
    audioMasterVendorSpecific,
    
    audioMasterSetIcon = 36,
    audioMasterCanDo,
    audioMasterGetLanguage,
    
    audioMasterOpenWindow = 39,
    audioMasterCloseWindow,
    
    audioMasterGetDirectory = 41,
    audioMasterUpdateDisplay,
    audioMasterBeginEdit,
    audioMasterEndEdit,
    
    audioMasterOpenFileSelector,
    audioMasterCloseFileSelector,
    
    audioMasterEditFile = 47,
    audioMasterGetChunkFile,
    audioMasterGetInputSpeakerArrangement
};

//-------------------------------------------------------------------------------------------------------
// Plugin category
//-------------------------------------------------------------------------------------------------------
enum VstPlugCategory {
    kPlugCategUnknown = 0,
    kPlugCategEffect,
    kPlugCategSynth,
    kPlugCategAnalysis,
    kPlugCategMastering,
    kPlugCategSpacializer,
    kPlugCategRoomFx,
    kPlugSurroundFx,
    kPlugCategRestoration,
    kPlugCategOfflineProcess,
    kPlugCategShell,
    kPlugCategGenerator,
    kPlugCategMaxCount
};

//-------------------------------------------------------------------------------------------------------
// VstTimeInfo structure
//-------------------------------------------------------------------------------------------------------
struct VstTimeInfo {
    double samplePos;
    double sampleRate;
    double nanoSeconds;
    double ppqPos;
    double tempo;
    double barStartPos;
    double cycleStartPos;
    double cycleEndPos;
    VstInt32 timeSigNumerator;
    VstInt32 timeSigDenominator;
    VstInt32 smpteOffset;
    VstInt32 smpteFrameRate;
    VstInt32 samplesToNextClock;
    VstInt32 flags;
};

enum VstTimeInfoFlags {
    kVstTransportChanged     = 1,
    kVstTransportPlaying     = 1 << 1,
    kVstTransportCycleActive = 1 << 2,
    kVstTransportRecording   = 1 << 3,
    kVstAutomationWriting    = 1 << 6,
    kVstAutomationReading    = 1 << 7,
    kVstNanosValid           = 1 << 8,
    kVstPpqPosValid          = 1 << 9,
    kVstTempoValid           = 1 << 10,
    kVstBarsValid            = 1 << 11,
    kVstCyclePosValid        = 1 << 12,
    kVstTimeSigValid         = 1 << 13,
    kVstSmpteValid           = 1 << 14,
    kVstClockValid           = 1 << 15
};

//-------------------------------------------------------------------------------------------------------
// MIDI Events
//-------------------------------------------------------------------------------------------------------
enum VstEventTypes {
    kVstMidiType = 1,
    kVstAudioType,
    kVstVideoType,
    kVstParameterType,
    kVstTriggerType,
    kVstSysExType
};

struct VstEvent {
    VstInt32 type;
    VstInt32 byteSize;
    VstInt32 deltaFrames;
    VstInt32 flags;
    char data[16];
};

struct VstEvents {
    VstInt32 numEvents;
    VstIntPtr reserved;
    VstEvent* events[2];
};

struct VstMidiEvent {
    VstInt32 type;
    VstInt32 byteSize;
    VstInt32 deltaFrames;
    VstInt32 flags;
    VstInt32 noteLength;
    VstInt32 noteOffset;
    char midiData[4];
    char detune;
    char noteOffVelocity;
    char reserved1;
    char reserved2;
};

struct VstMidiSysexEvent {
    VstInt32 type;
    VstInt32 byteSize;
    VstInt32 deltaFrames;
    VstInt32 flags;
    VstInt32 dumpBytes;
    VstIntPtr resvd1;
    char* sysexDump;
    VstIntPtr resvd2;
};

//-------------------------------------------------------------------------------------------------------
// Parameter properties
//-------------------------------------------------------------------------------------------------------
struct VstParameterProperties {
    float stepFloat;
    float smallStepFloat;
    float largeStepFloat;
    char label[kVstMaxLabelLen];
    VstInt32 flags;
    VstInt32 minInteger;
    VstInt32 maxInteger;
    VstInt32 stepInteger;
    VstInt32 largeStepInteger;
    char shortLabel[kVstMaxShortLabelLen];
    VstInt16 displayIndex;
    VstInt16 category;
    VstInt16 numParametersInCategory;
    VstInt16 reserved;
    char categoryLabel[kVstMaxCategLabelLen];
    char future[16];
};

enum VstParameterFlags {
    kVstParameterIsSwitch                = 1 << 0,
    kVstParameterUsesIntegerMinMax       = 1 << 1,
    kVstParameterUsesFloatStep           = 1 << 2,
    kVstParameterUsesIntStep             = 1 << 3,
    kVstParameterSupportsDisplayIndex    = 1 << 4,
    kVstParameterSupportsDisplayCategory = 1 << 5,
    kVstParameterCanRamp                 = 1 << 6
};

//-------------------------------------------------------------------------------------------------------
// Pin properties
//-------------------------------------------------------------------------------------------------------
struct VstPinProperties {
    char label[kVstMaxLabelLen];
    VstInt32 flags;
    VstInt32 arrangementType;
    char shortLabel[kVstMaxShortLabelLen];
    char future[48];
};

enum VstPinPropertiesFlags {
    kVstPinIsActive   = 1 << 0,
    kVstPinIsStereo   = 1 << 1,
    kVstPinUseSpeaker = 1 << 2
};

//-------------------------------------------------------------------------------------------------------
// VstKeyCode for keyboard events
//-------------------------------------------------------------------------------------------------------
struct VstKeyCode {
    VstInt32 character;
    VstUInt8 virt;
    VstUInt8 modifier;
};

enum VstVirtualKey {
    VKEY_BACK = 1,
    VKEY_TAB,
    VKEY_CLEAR,
    VKEY_RETURN,
    VKEY_PAUSE,
    VKEY_ESCAPE,
    VKEY_SPACE,
    VKEY_NEXT,
    VKEY_END,
    VKEY_HOME,
    VKEY_LEFT,
    VKEY_UP,
    VKEY_RIGHT,
    VKEY_DOWN,
    VKEY_PAGEUP,
    VKEY_PAGEDOWN,
    VKEY_SELECT,
    VKEY_PRINT,
    VKEY_ENTER,
    VKEY_SNAPSHOT,
    VKEY_INSERT,
    VKEY_DELETE,
    VKEY_HELP,
    VKEY_NUMPAD0,
    VKEY_NUMPAD1,
    VKEY_NUMPAD2,
    VKEY_NUMPAD3,
    VKEY_NUMPAD4,
    VKEY_NUMPAD5,
    VKEY_NUMPAD6,
    VKEY_NUMPAD7,
    VKEY_NUMPAD8,
    VKEY_NUMPAD9,
    VKEY_MULTIPLY,
    VKEY_ADD,
    VKEY_SEPARATOR,
    VKEY_SUBTRACT,
    VKEY_DECIMAL,
    VKEY_DIVIDE,
    VKEY_F1,
    VKEY_F2,
    VKEY_F3,
    VKEY_F4,
    VKEY_F5,
    VKEY_F6,
    VKEY_F7,
    VKEY_F8,
    VKEY_F9,
    VKEY_F10,
    VKEY_F11,
    VKEY_F12,
    VKEY_NUMLOCK,
    VKEY_SCROLL,
    VKEY_SHIFT,
    VKEY_CONTROL,
    VKEY_ALT,
    VKEY_EQUALS
};

enum VstModifierKey {
    MODIFIER_SHIFT     = 1 << 0,
    MODIFIER_ALTERNATE = 1 << 1,
    MODIFIER_COMMAND   = 1 << 2,
    MODIFIER_CONTROL   = 1 << 3
};

//-------------------------------------------------------------------------------------------------------
// Knob mode
//-------------------------------------------------------------------------------------------------------
enum VstKnobMode {
    kCircularMode = 0,
    kRelativCircularMode,
    kLinearMode
};

//-------------------------------------------------------------------------------------------------------
// Process precision
//-------------------------------------------------------------------------------------------------------
enum VstProcessPrecision {
    kVstProcessPrecision32 = 0,
    kVstProcessPrecision64
};

#endif // __aeffectx__
