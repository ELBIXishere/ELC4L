//-------------------------------------------------------------------------------------------------------
// VST Plug-Ins SDK
// AudioEffectX C++ wrapper class
//-------------------------------------------------------------------------------------------------------

#pragma once

#ifndef __audioeffectx__
#define __audioeffectx__

#include "aeffectx.h"
#include <string.h>

//-------------------------------------------------------------------------------------------------------
// Forward declaration
//-------------------------------------------------------------------------------------------------------
class AudioEffect;
class AudioEffectX;
class AEffEditor;

//-------------------------------------------------------------------------------------------------------
// AEffEditor - base class for plugin editors
//-------------------------------------------------------------------------------------------------------
class AEffEditor {
public:
    AEffEditor(AudioEffect* effect = nullptr)
        : effect(effect)
        , systemWindow(nullptr)
    {}
    
    virtual ~AEffEditor() {}
    
    virtual AudioEffect* getEffect() { return effect; }
    virtual bool getRect(ERect** rect) { *rect = nullptr; return false; }
    virtual bool open(void* ptr) { systemWindow = ptr; return true; }
    virtual void close() { systemWindow = nullptr; }
    virtual bool isOpen() { return systemWindow != nullptr; }
    virtual void idle() {}
    
    // VST 2.1
    virtual bool onKeyDown(VstKeyCode& keyCode) { return false; }
    virtual bool onKeyUp(VstKeyCode& keyCode) { return false; }
    virtual bool onWheel(float distance) { return false; }
    virtual bool setKnobMode(VstInt32 val) { return false; }
    
    void* getSystemWindow() { return systemWindow; }

protected:
    AudioEffect* effect;
    void* systemWindow;
};

//-------------------------------------------------------------------------------------------------------
// AudioEffect - base class for plugins
//-------------------------------------------------------------------------------------------------------
class AudioEffect {
public:
    AudioEffect(audioMasterCallback audioMaster, VstInt32 numPrograms, VstInt32 numParams);
    virtual ~AudioEffect();
    
    // Dispatcher
    virtual VstIntPtr dispatcher(VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt);
    
    // Lifecycle
    virtual void open() {}
    virtual void close() {}
    virtual void suspend() {}
    virtual void resume() {}
    
    // Audio processing - pure virtual, must be implemented
    virtual void processReplacing(float** inputs, float** outputs, VstInt32 sampleFrames) = 0;
    virtual void processDoubleReplacing(double** inputs, double** outputs, VstInt32 sampleFrames) {}
    
    // Parameters
    virtual void setParameter(VstInt32 index, float value) {}
    virtual float getParameter(VstInt32 index) { return 0.f; }
    virtual void setParameterAutomated(VstInt32 index, float value);
    
    // Programs
    virtual VstInt32 getProgram() { return curProgram; }
    virtual void setProgram(VstInt32 program);
    virtual void setProgramName(char* name) {}
    virtual void getProgramName(char* name);
    
    // Parameter info
    virtual void getParameterLabel(VstInt32 index, char* label) { *label = 0; }
    virtual void getParameterDisplay(VstInt32 index, char* text) { *text = 0; }
    virtual void getParameterName(VstInt32 index, char* text) { *text = 0; }
    
    // Chunks (preset storage)
    virtual VstInt32 getChunk(void** data, bool isPreset = false) { return 0; }
    virtual VstInt32 setChunk(void* data, VstInt32 byteSize, bool isPreset = false) { return 0; }
    
    // Settings
    virtual void setSampleRate(float sampleRate) { this->sampleRate = sampleRate; }
    virtual void setBlockSize(VstInt32 blockSize) { this->blockSize = blockSize; }
    
    void setUniqueID(VstInt32 iD) { cEffect.uniqueID = iD; }
    void setNumInputs(VstInt32 inputs) { cEffect.numInputs = inputs; }
    void setNumOutputs(VstInt32 outputs) { cEffect.numOutputs = outputs; }
    void canProcessReplacing(bool state = true);
    void canDoubleReplacing(bool state = true);
    void programsAreChunks(bool state = true);
    void setInitialDelay(VstInt32 delay) { cEffect.initialDelay = delay; }
    void setEditor(AEffEditor* editor) { this->editor = editor; }
    
    // Getters
    AEffect* getAeffect() { return &cEffect; }
    float getSampleRate() { return sampleRate; }
    VstInt32 getBlockSize() { return blockSize; }
    AEffEditor* getEditor() { return editor; }
    VstInt32 getNumPrograms() { return numPrograms; }
    VstInt32 getNumParams() { return numParams; }
    
    // Host communication
    VstInt32 getMasterVersion();
    VstInt32 getCurrentUniqueId();
    void masterIdle();

protected:
    audioMasterCallback audioMaster;
    AEffEditor* editor;
    float sampleRate;
    VstInt32 blockSize;
    VstInt32 numPrograms;
    VstInt32 numParams;
    VstInt32 curProgram;
    AEffect cEffect;
};

//-------------------------------------------------------------------------------------------------------
// AudioEffectX - extended class with VST 2.x features
//-------------------------------------------------------------------------------------------------------
class AudioEffectX : public AudioEffect {
public:
    AudioEffectX(audioMasterCallback audioMaster, VstInt32 numPrograms, VstInt32 numParams);
    virtual ~AudioEffectX() {}
    
    // Extended dispatcher
    virtual VstIntPtr dispatcher(VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt);
    
    // Parameter extensions
    virtual bool canParameterBeAutomated(VstInt32 index) { return true; }
    virtual bool string2parameter(VstInt32 index, char* text) { return false; }
    virtual bool getParameterProperties(VstInt32 index, VstParameterProperties* p) { return false; }
    virtual bool beginEdit(VstInt32 index);
    virtual bool endEdit(VstInt32 index);
    
    // Program extensions
    virtual bool getProgramNameIndexed(VstInt32 category, VstInt32 index, char* text) { return false; }
    virtual bool beginSetProgram() { return false; }
    virtual bool endSetProgram() { return false; }
    
    // Real-time
    virtual VstTimeInfo* getTimeInfo(VstInt32 filter);
    virtual VstInt32 getCurrentProcessLevel();
    virtual VstInt32 getAutomationState();
    virtual VstInt32 processEvents(VstEvents* events) { return 0; }
    bool sendVstEventsToHost(VstEvents* events);
    virtual VstInt32 startProcess() { return 0; }
    virtual VstInt32 stopProcess() { return 0; }
    
    // I/O
    virtual bool ioChanged();
    virtual double updateSampleRate();
    virtual VstInt32 updateBlockSize();
    virtual VstInt32 getInputLatency();
    virtual VstInt32 getOutputLatency();
    virtual bool getInputProperties(VstInt32 index, VstPinProperties* properties) { return false; }
    virtual bool getOutputProperties(VstInt32 index, VstPinProperties* properties) { return false; }
    virtual bool setBypass(bool onOff) { return false; }
    virtual bool setProcessPrecision(VstInt32 precision) { return false; }
    
    // Plugin info
    virtual void isSynth(bool state = true);
    virtual void noTail(bool state = true);
    virtual VstInt32 getGetTailSize() { return 0; }
    virtual bool getEffectName(char* name) { return false; }
    virtual bool getVendorString(char* text) { return false; }
    virtual bool getProductString(char* text) { return false; }
    virtual VstInt32 getVendorVersion() { return 0; }
    virtual VstInt32 canDo(char* text) { return 0; }
    virtual VstInt32 getVstVersion() { return kVstVersion; }
    virtual VstPlugCategory getPlugCategory() { return kPlugCategEffect; }
    
    // Host info
    virtual bool getHostVendorString(char* text);
    virtual bool getHostProductString(char* text);
    virtual VstInt32 getHostVendorVersion();
    virtual VstInt32 canHostDo(char* text);
    virtual VstInt32 getHostLanguage();
    
    // UI
    virtual bool updateDisplay();
    virtual bool sizeWindow(VstInt32 width, VstInt32 height);
    
    // Resume override
    virtual void resume();
};

//-------------------------------------------------------------------------------------------------------
// Static callback functions (for AEffect structure)
//-------------------------------------------------------------------------------------------------------
extern "C" {
    VstIntPtr VSTCALLBACK hostCallback(AEffect* effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt);
}

#endif // __audioeffectx__
