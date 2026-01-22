//-------------------------------------------------------------------------------------------------------
// VST Plug-Ins SDK
// Minimal header for VST2 plugin development
// Based on Steinberg VST2 SDK structure
//-------------------------------------------------------------------------------------------------------

#pragma once

#ifndef __aeffect__
#define __aeffect__

//-------------------------------------------------------------------------------------------------------
// Platform detection
//-------------------------------------------------------------------------------------------------------
#if defined(_WIN32) || defined(_WIN64)
    #define VST_WINDOWS 1
#elif defined(__APPLE__)
    #define VST_MACOS 1
#else
    #define VST_LINUX 1
#endif

//-------------------------------------------------------------------------------------------------------
// Basic types
//-------------------------------------------------------------------------------------------------------
typedef char VstInt8;
typedef unsigned char VstUInt8;

#ifdef VST_WINDOWS
    typedef short VstInt16;
    typedef unsigned short VstUInt16;
    typedef int VstInt32;
    typedef unsigned int VstUInt32;
    typedef __int64 VstInt64;
    typedef unsigned __int64 VstUInt64;
#else
    #include <stdint.h>
    typedef int16_t VstInt16;
    typedef uint16_t VstUInt16;
    typedef int32_t VstInt32;
    typedef uint32_t VstUInt32;
    typedef int64_t VstInt64;
    typedef uint64_t VstUInt64;
#endif

// Pointer-sized integer
#if defined(_WIN64) || defined(__LP64__) || defined(__x86_64__)
    typedef VstInt64 VstIntPtr;
#else
    typedef VstInt32 VstIntPtr;
#endif

//-------------------------------------------------------------------------------------------------------
// Calling convention
//-------------------------------------------------------------------------------------------------------
#if defined(VST_WINDOWS)
    #define VSTCALLBACK __cdecl
#else
    #define VSTCALLBACK
#endif

//-------------------------------------------------------------------------------------------------------
// Magic number macro
//-------------------------------------------------------------------------------------------------------
#define CCONST(a, b, c, d) \
    ((((VstInt32)(a)) << 24) | (((VstInt32)(b)) << 16) | \
     (((VstInt32)(c)) << 8) | (((VstInt32)(d)) << 0))

//-------------------------------------------------------------------------------------------------------
// Constants
//-------------------------------------------------------------------------------------------------------
#define kEffectMagic CCONST('V', 's', 't', 'P')
#define kVstVersion 2400

//-------------------------------------------------------------------------------------------------------
// String length limits
//-------------------------------------------------------------------------------------------------------
enum VstStringConstants {
    kVstMaxProgNameLen   = 24,
    kVstMaxParamStrLen   = 8,
    kVstMaxVendorStrLen  = 64,
    kVstMaxProductStrLen = 64,
    kVstMaxEffectNameLen = 32
};

//-------------------------------------------------------------------------------------------------------
// Forward declaration
//-------------------------------------------------------------------------------------------------------
struct AEffect;

//-------------------------------------------------------------------------------------------------------
// Function pointer types
//-------------------------------------------------------------------------------------------------------
typedef VstIntPtr (VSTCALLBACK *audioMasterCallback)(
    AEffect* effect,
    VstInt32 opcode,
    VstInt32 index,
    VstIntPtr value,
    void* ptr,
    float opt
);

typedef VstIntPtr (VSTCALLBACK *AEffectDispatcherProc)(
    AEffect* effect,
    VstInt32 opcode,
    VstInt32 index,
    VstIntPtr value,
    void* ptr,
    float opt
);

typedef void (VSTCALLBACK *AEffectProcessProc)(
    AEffect* effect,
    float** inputs,
    float** outputs,
    VstInt32 sampleFrames
);

typedef void (VSTCALLBACK *AEffectProcessDoubleProc)(
    AEffect* effect,
    double** inputs,
    double** outputs,
    VstInt32 sampleFrames
);

typedef void (VSTCALLBACK *AEffectSetParameterProc)(
    AEffect* effect,
    VstInt32 index,
    float parameter
);

typedef float (VSTCALLBACK *AEffectGetParameterProc)(
    AEffect* effect,
    VstInt32 index
);

//-------------------------------------------------------------------------------------------------------
// AEffect structure - core VST plugin interface
//-------------------------------------------------------------------------------------------------------
struct AEffect {
    VstInt32 magic;                              // Must be kEffectMagic ('VstP')
    
    AEffectDispatcherProc dispatcher;            // Host to plugin dispatcher
    AEffectProcessProc process;                  // [deprecated] accumulating process
    AEffectSetParameterProc setParameter;        // Set parameter value
    AEffectGetParameterProc getParameter;        // Get parameter value
    
    VstInt32 numPrograms;                        // Number of programs
    VstInt32 numParams;                          // Number of parameters
    VstInt32 numInputs;                          // Number of audio inputs
    VstInt32 numOutputs;                         // Number of audio outputs
    
    VstInt32 flags;                              // VstAEffectFlags
    
    VstIntPtr resvd1;                            // Reserved for host (set to 0)
    VstIntPtr resvd2;                            // Reserved for host (set to 0)
    
    VstInt32 initialDelay;                       // Latency in samples
    
    VstInt32 realQualities;                      // [deprecated]
    VstInt32 offQualities;                       // [deprecated]
    float ioRatio;                               // [deprecated]
    
    void* object;                                // AudioEffect class pointer
    void* user;                                  // User-defined pointer
    
    VstInt32 uniqueID;                           // Unique plugin ID
    VstInt32 version;                            // Plugin version
    
    AEffectProcessProc processReplacing;         // 32-bit replacing process
    AEffectProcessDoubleProc processDoubleReplacing; // 64-bit replacing process
    
    char future[56];                             // Reserved for future use
};

//-------------------------------------------------------------------------------------------------------
// VstAEffectFlags - plugin capability flags
//-------------------------------------------------------------------------------------------------------
enum VstAEffectFlags {
    effFlagsHasEditor          = 1 << 0,    // Has custom editor
    effFlagsCanReplacing       = 1 << 4,    // Supports processReplacing (required for VST 2.4)
    effFlagsProgramChunks      = 1 << 5,    // Uses chunk-based program data
    effFlagsIsSynth            = 1 << 8,    // Is a VSTi (instrument)
    effFlagsNoSoundInStop      = 1 << 9,    // No output when input is silent
    effFlagsCanDoubleReplacing = 1 << 12    // Supports 64-bit processing
};

//-------------------------------------------------------------------------------------------------------
// Host to Plugin opcodes
//-------------------------------------------------------------------------------------------------------
enum AEffectOpcodes {
    effOpen = 0,                // Initialize plugin
    effClose,                   // Deinitialize plugin
    
    effSetProgram,              // [value]: program number
    effGetProgram,              // [return]: current program number
    effSetProgramName,          // [ptr]: char* program name
    effGetProgramName,          // [ptr]: char buffer (kVstMaxProgNameLen)
    
    effGetParamLabel,           // [ptr]: parameter unit ("dB", "Hz", etc.)
    effGetParamDisplay,         // [ptr]: parameter value string
    effGetParamName,            // [ptr]: parameter name
    
    effGetVu,                   // [deprecated]
    
    effSetSampleRate,           // [opt]: sample rate (float)
    effSetBlockSize,            // [value]: block size
    effMainsChanged,            // [value]: 0=off, 1=on (suspend/resume)
    
    effEditGetRect,             // [ptr]: ERect** - editor dimensions
    effEditOpen,                // [ptr]: HWND (Windows) / WindowRef (Mac)
    effEditClose,               // Close editor
    effEditDraw,                // [deprecated]
    effEditMouse,               // [deprecated]
    effEditKey,                 // [deprecated]
    effEditIdle,                // Editor idle callback
    effEditTop,                 // [deprecated]
    effEditSleep,               // [deprecated]
    effIdentify,                // [deprecated]
    
    effGetChunk,                // [ptr]: void** chunk address, [index]: 0=bank, 1=program
    effSetChunk,                // [ptr]: chunk data, [value]: size
    
    effNumOpcodes
};

//-------------------------------------------------------------------------------------------------------
// Plugin to Host opcodes
//-------------------------------------------------------------------------------------------------------
enum AudioMasterOpcodes {
    audioMasterAutomate = 0,    // Parameter automation notification
    audioMasterVersion,         // [return]: host VST version
    audioMasterCurrentId,       // [return]: shell plugin ID
    audioMasterIdle,            // Request host idle
    audioMasterPinConnected     // [deprecated]
};

//-------------------------------------------------------------------------------------------------------
// ERect - editor rectangle
//-------------------------------------------------------------------------------------------------------
struct ERect {
    VstInt16 top;
    VstInt16 left;
    VstInt16 bottom;
    VstInt16 right;
};

#endif // __aeffect__
