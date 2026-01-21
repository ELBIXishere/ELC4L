//-------------------------------------------------------------------------------------------------------
// ELC4L VST3 - Plugin Factory (Entry Point)
//-------------------------------------------------------------------------------------------------------

#include "public.sdk/source/main/pluginfactory.h"
#include "ELC4Lids.h"
#include "ELC4Lprocessor.h"
#include "ELC4Lcontroller.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "version.h"

#define stringPluginName "ELC4L"

using namespace Steinberg::Vst;
using namespace ELC4L;

//-------------------------------------------------------------------------------------------------------
// Plugin Factory
//-------------------------------------------------------------------------------------------------------
BEGIN_FACTORY_DEF("Hyeok Audio",
                  "https://github.com/hyeokaudio",
                  "mailto:hyeok@example.com")

    // Register Audio Processor
    DEF_CLASS2(INLINE_UID_FROM_FUID(kProcessorUID),
               PClassInfo::kManyInstances,
               kVstAudioEffectClass,
               stringPluginName,
               Vst::kDistributable,
               Vst::PlugType::kFxDynamics,
               FULL_VERSION_STR,
               kVstVersionString,
               ELC4LProcessor::createInstance)

    // Register Edit Controller
    DEF_CLASS2(INLINE_UID_FROM_FUID(kControllerUID),
               PClassInfo::kManyInstances,
               kVstComponentControllerClass,
               stringPluginName "Controller",
               0,
               "",
               FULL_VERSION_STR,
               kVstVersionString,
               ELC4LController::createInstance)

END_FACTORY
