//-------------------------------------------------------------------------------------------------------
// ELC4L VST3 - Edit Controller (Parameter handling + UI)
//-------------------------------------------------------------------------------------------------------
#pragma once

#include "public.sdk/source/vst/vsteditcontroller.h"
#include "vstgui/plugin-bindings/vst3editor.h"
#include "ELC4Lids.h"

namespace ELC4L {

class ELC4LController : public Steinberg::Vst::EditController {
public:
    ELC4LController() = default;
    ~ELC4LController() override = default;
    
    static Steinberg::FUnknown* createInstance(void*) {
        return static_cast<Steinberg::Vst::IEditController*>(new ELC4LController());
    }
    
    // EditController overrides
    Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown* context) override;
    Steinberg::tresult PLUGIN_API setComponentState(Steinberg::IBStream* state) override;
    
    // Editor
    Steinberg::IPlugView* PLUGIN_API createView(Steinberg::FIDString name) override;
};

} // namespace ELC4L
