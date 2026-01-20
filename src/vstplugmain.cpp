//-------------------------------------------------------------------------------------------------------
// VSTPluginMain - Entry point for VST2 plugin
// This is the function called by the host to create the plugin instance
//-------------------------------------------------------------------------------------------------------

#include "HyeokStreamMaster.h"

//-------------------------------------------------------------------------------------------------------
// DLL Export definitions
//-------------------------------------------------------------------------------------------------------
#if defined(WIN32) || defined(_WIN32)
    #define VST_EXPORT extern "C" __declspec(dllexport)
#else
    #define VST_EXPORT extern "C" __attribute__((visibility("default")))
#endif

//-------------------------------------------------------------------------------------------------------
// Main entry point - called by host to create plugin instance
//-------------------------------------------------------------------------------------------------------
VST_EXPORT AEffect* VSTPluginMain(audioMasterCallback audioMaster) {
    // Check host callback
    if (!audioMaster)
        return nullptr;
    
    // Check host VST version (should support at least VST 2.4)
    if (audioMaster(nullptr, audioMasterVersion, 0, 0, nullptr, 0) == 0)
        return nullptr;
    
    // Create plugin instance
    HyeokStreamMaster* effect = new HyeokStreamMaster(audioMaster);
    if (!effect)
        return nullptr;
    
    return effect->getAeffect();
}

//-------------------------------------------------------------------------------------------------------
// Legacy entry point for older hosts
//-------------------------------------------------------------------------------------------------------
VST_EXPORT AEffect* main_plugin(audioMasterCallback audioMaster) {
    return VSTPluginMain(audioMaster);
}

//-------------------------------------------------------------------------------------------------------
// DLL Entry Point (Windows)
//-------------------------------------------------------------------------------------------------------
#if defined(WIN32) || defined(_WIN32)
#include <windows.h>

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
            // Initialize once for each new process
            break;
        case DLL_THREAD_ATTACH:
            break;
        case DLL_THREAD_DETACH:
            break;
        case DLL_PROCESS_DETACH:
            // Cleanup when unloaded
            break;
    }
    return TRUE;
}
#endif
