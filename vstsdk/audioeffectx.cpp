//-------------------------------------------------------------------------------------------------------
// VST Plug-Ins SDK
// AudioEffectX implementation
//-------------------------------------------------------------------------------------------------------

#include "audioeffectx.h"

//-------------------------------------------------------------------------------------------------------
// Static callback wrappers for AEffect structure
//-------------------------------------------------------------------------------------------------------
static VstIntPtr VSTCALLBACK dispatcherProc(AEffect* effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt) {
    AudioEffect* ae = (AudioEffect*)effect->object;
    if (ae)
        return ae->dispatcher(opcode, index, value, ptr, opt);
    return 0;
}

static void VSTCALLBACK processProc(AEffect* effect, float** inputs, float** outputs, VstInt32 sampleFrames) {
    AudioEffect* ae = (AudioEffect*)effect->object;
    if (ae)
        ae->processReplacing(inputs, outputs, sampleFrames);
}

static void VSTCALLBACK processReplacingProc(AEffect* effect, float** inputs, float** outputs, VstInt32 sampleFrames) {
    AudioEffect* ae = (AudioEffect*)effect->object;
    if (ae)
        ae->processReplacing(inputs, outputs, sampleFrames);
}

static void VSTCALLBACK processDoubleReplacingProc(AEffect* effect, double** inputs, double** outputs, VstInt32 sampleFrames) {
    AudioEffect* ae = (AudioEffect*)effect->object;
    if (ae)
        ae->processDoubleReplacing(inputs, outputs, sampleFrames);
}

static void VSTCALLBACK setParameterProc(AEffect* effect, VstInt32 index, float parameter) {
    AudioEffect* ae = (AudioEffect*)effect->object;
    if (ae)
        ae->setParameter(index, parameter);
}

static float VSTCALLBACK getParameterProc(AEffect* effect, VstInt32 index) {
    AudioEffect* ae = (AudioEffect*)effect->object;
    if (ae)
        return ae->getParameter(index);
    return 0.f;
}

//-------------------------------------------------------------------------------------------------------
// AudioEffect implementation
//-------------------------------------------------------------------------------------------------------
AudioEffect::AudioEffect(audioMasterCallback audioMaster, VstInt32 numPrograms, VstInt32 numParams)
    : audioMaster(audioMaster)
    , editor(nullptr)
    , sampleRate(44100.f)
    , blockSize(1024)
    , numPrograms(numPrograms)
    , numParams(numParams)
    , curProgram(0)
{
    memset(&cEffect, 0, sizeof(AEffect));
    
    cEffect.magic = kEffectMagic;
    cEffect.dispatcher = dispatcherProc;
    cEffect.process = processProc;
    cEffect.setParameter = setParameterProc;
    cEffect.getParameter = getParameterProc;
    cEffect.numPrograms = numPrograms;
    cEffect.numParams = numParams;
    cEffect.numInputs = 2;
    cEffect.numOutputs = 2;
    cEffect.flags = 0;
    cEffect.resvd1 = 0;
    cEffect.resvd2 = 0;
    cEffect.initialDelay = 0;
    cEffect.object = this;
    cEffect.user = nullptr;
    cEffect.uniqueID = 0;
    cEffect.version = 1;
    cEffect.processReplacing = processReplacingProc;
    cEffect.processDoubleReplacing = nullptr;
}

AudioEffect::~AudioEffect() {
    if (editor)
        delete editor;
}

VstIntPtr AudioEffect::dispatcher(VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt) {
    VstIntPtr result = 0;
    
    switch (opcode) {
        case effOpen:
            open();
            break;
            
        case effClose:
            close();
            break;
            
        case effSetProgram:
            setProgram((VstInt32)value);
            break;
            
        case effGetProgram:
            result = getProgram();
            break;
            
        case effSetProgramName:
            setProgramName((char*)ptr);
            break;
            
        case effGetProgramName:
            getProgramName((char*)ptr);
            break;
            
        case effGetParamLabel:
            getParameterLabel(index, (char*)ptr);
            break;
            
        case effGetParamDisplay:
            getParameterDisplay(index, (char*)ptr);
            break;
            
        case effGetParamName:
            getParameterName(index, (char*)ptr);
            break;
            
        case effSetSampleRate:
            setSampleRate(opt);
            break;
            
        case effSetBlockSize:
            setBlockSize((VstInt32)value);
            break;
            
        case effMainsChanged:
            if (value)
                resume();
            else
                suspend();
            break;
            
        case effEditGetRect:
            if (editor)
                result = editor->getRect((ERect**)ptr) ? 1 : 0;
            break;
            
        case effEditOpen:
            if (editor)
                result = editor->open(ptr) ? 1 : 0;
            break;
            
        case effEditClose:
            if (editor)
                editor->close();
            break;
            
        case effEditIdle:
            if (editor)
                editor->idle();
            break;
            
        case effGetChunk:
            result = getChunk((void**)ptr, index != 0);
            break;
            
        case effSetChunk:
            result = setChunk(ptr, (VstInt32)value, index != 0);
            break;
            
        default:
            break;
    }
    
    return result;
}

void AudioEffect::setParameterAutomated(VstInt32 index, float value) {
    setParameter(index, value);
    if (audioMaster)
        audioMaster(&cEffect, audioMasterAutomate, index, 0, nullptr, value);
}

void AudioEffect::setProgram(VstInt32 program) {
    if (program >= 0 && program < numPrograms)
        curProgram = program;
}

void AudioEffect::getProgramName(char* name) {
    strcpy(name, "Default");
}

void AudioEffect::canProcessReplacing(bool state) {
    if (state)
        cEffect.flags |= effFlagsCanReplacing;
    else
        cEffect.flags &= ~effFlagsCanReplacing;
}

void AudioEffect::canDoubleReplacing(bool state) {
    if (state) {
        cEffect.flags |= effFlagsCanDoubleReplacing;
        cEffect.processDoubleReplacing = processDoubleReplacingProc;
    } else {
        cEffect.flags &= ~effFlagsCanDoubleReplacing;
        cEffect.processDoubleReplacing = nullptr;
    }
}

void AudioEffect::programsAreChunks(bool state) {
    if (state)
        cEffect.flags |= effFlagsProgramChunks;
    else
        cEffect.flags &= ~effFlagsProgramChunks;
}

VstInt32 AudioEffect::getMasterVersion() {
    if (audioMaster)
        return (VstInt32)audioMaster(&cEffect, audioMasterVersion, 0, 0, nullptr, 0);
    return 0;
}

VstInt32 AudioEffect::getCurrentUniqueId() {
    if (audioMaster)
        return (VstInt32)audioMaster(&cEffect, audioMasterCurrentId, 0, 0, nullptr, 0);
    return 0;
}

void AudioEffect::masterIdle() {
    if (audioMaster)
        audioMaster(&cEffect, audioMasterIdle, 0, 0, nullptr, 0);
}

//-------------------------------------------------------------------------------------------------------
// AudioEffectX implementation
//-------------------------------------------------------------------------------------------------------
AudioEffectX::AudioEffectX(audioMasterCallback audioMaster, VstInt32 numPrograms, VstInt32 numParams)
    : AudioEffect(audioMaster, numPrograms, numParams)
{
    canProcessReplacing();
}

VstIntPtr AudioEffectX::dispatcher(VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt) {
    VstIntPtr result = 0;
    
    switch (opcode) {
        // Extended opcodes
        case effProcessEvents:
            result = processEvents((VstEvents*)ptr);
            break;
            
        case effCanBeAutomated:
            result = canParameterBeAutomated(index) ? 1 : 0;
            break;
            
        case effString2Parameter:
            result = string2parameter(index, (char*)ptr) ? 1 : 0;
            break;
            
        case effGetProgramNameIndexed:
            result = getProgramNameIndexed((VstInt32)value, index, (char*)ptr) ? 1 : 0;
            break;
            
        case effGetInputProperties:
            result = getInputProperties(index, (VstPinProperties*)ptr) ? 1 : 0;
            break;
            
        case effGetOutputProperties:
            result = getOutputProperties(index, (VstPinProperties*)ptr) ? 1 : 0;
            break;
            
        case effGetPlugCategory:
            result = (VstIntPtr)getPlugCategory();
            break;
            
        case effSetBypass:
            result = setBypass(value != 0) ? 1 : 0;
            break;
            
        case effGetEffectName:
            result = getEffectName((char*)ptr) ? 1 : 0;
            break;
            
        case effGetVendorString:
            result = getVendorString((char*)ptr) ? 1 : 0;
            break;
            
        case effGetProductString:
            result = getProductString((char*)ptr) ? 1 : 0;
            break;
            
        case effGetVendorVersion:
            result = getVendorVersion();
            break;
            
        case effCanDo:
            result = canDo((char*)ptr);
            break;
            
        case effGetTailSize:
            result = getGetTailSize();
            break;
            
        case effGetParameterProperties:
            result = getParameterProperties(index, (VstParameterProperties*)ptr) ? 1 : 0;
            break;
            
        case effGetVstVersion:
            result = getVstVersion();
            break;
            
        // VST 2.1
        case effEditKeyDown:
            if (editor) {
                VstKeyCode keyCode;
                keyCode.character = index;
                keyCode.virt = (VstUInt8)value;
                keyCode.modifier = (VstUInt8)opt;
                result = editor->onKeyDown(keyCode) ? 1 : 0;
            }
            break;
            
        case effEditKeyUp:
            if (editor) {
                VstKeyCode keyCode;
                keyCode.character = index;
                keyCode.virt = (VstUInt8)value;
                keyCode.modifier = (VstUInt8)opt;
                result = editor->onKeyUp(keyCode) ? 1 : 0;
            }
            break;
            
        case effSetEditKnobMode:
            if (editor)
                result = editor->setKnobMode((VstInt32)value) ? 1 : 0;
            break;
            
        case effBeginSetProgram:
            result = beginSetProgram() ? 1 : 0;
            break;
            
        case effEndSetProgram:
            result = endSetProgram() ? 1 : 0;
            break;
            
        case effStartProcess:
            result = startProcess();
            break;
            
        case effStopProcess:
            result = stopProcess();
            break;
            
        case effSetProcessPrecision:
            result = setProcessPrecision((VstInt32)value) ? 1 : 0;
            break;
            
        default:
            // Pass to base class
            result = AudioEffect::dispatcher(opcode, index, value, ptr, opt);
            break;
    }
    
    return result;
}

bool AudioEffectX::beginEdit(VstInt32 index) {
    if (audioMaster)
        return audioMaster(&cEffect, audioMasterBeginEdit, index, 0, nullptr, 0) != 0;
    return false;
}

bool AudioEffectX::endEdit(VstInt32 index) {
    if (audioMaster)
        return audioMaster(&cEffect, audioMasterEndEdit, index, 0, nullptr, 0) != 0;
    return false;
}

VstTimeInfo* AudioEffectX::getTimeInfo(VstInt32 filter) {
    if (audioMaster)
        return (VstTimeInfo*)audioMaster(&cEffect, audioMasterGetTime, 0, filter, nullptr, 0);
    return nullptr;
}

VstInt32 AudioEffectX::getCurrentProcessLevel() {
    if (audioMaster)
        return (VstInt32)audioMaster(&cEffect, audioMasterGetCurrentProcessLevel, 0, 0, nullptr, 0);
    return 0;
}

VstInt32 AudioEffectX::getAutomationState() {
    if (audioMaster)
        return (VstInt32)audioMaster(&cEffect, audioMasterGetAutomationState, 0, 0, nullptr, 0);
    return 0;
}

bool AudioEffectX::sendVstEventsToHost(VstEvents* events) {
    if (audioMaster)
        return audioMaster(&cEffect, audioMasterProcessEvents, 0, 0, events, 0) != 0;
    return false;
}

bool AudioEffectX::ioChanged() {
    if (audioMaster)
        return audioMaster(&cEffect, audioMasterIOChanged, 0, 0, nullptr, 0) != 0;
    return false;
}

double AudioEffectX::updateSampleRate() {
    if (audioMaster) {
        VstIntPtr res = audioMaster(&cEffect, audioMasterGetSampleRate, 0, 0, nullptr, 0);
        if (res > 0)
            sampleRate = (float)res;
    }
    return sampleRate;
}

VstInt32 AudioEffectX::updateBlockSize() {
    if (audioMaster) {
        VstIntPtr res = audioMaster(&cEffect, audioMasterGetBlockSize, 0, 0, nullptr, 0);
        if (res > 0)
            blockSize = (VstInt32)res;
    }
    return blockSize;
}

VstInt32 AudioEffectX::getInputLatency() {
    if (audioMaster)
        return (VstInt32)audioMaster(&cEffect, audioMasterGetInputLatency, 0, 0, nullptr, 0);
    return 0;
}

VstInt32 AudioEffectX::getOutputLatency() {
    if (audioMaster)
        return (VstInt32)audioMaster(&cEffect, audioMasterGetOutputLatency, 0, 0, nullptr, 0);
    return 0;
}

void AudioEffectX::isSynth(bool state) {
    if (state)
        cEffect.flags |= effFlagsIsSynth;
    else
        cEffect.flags &= ~effFlagsIsSynth;
}

void AudioEffectX::noTail(bool state) {
    if (state)
        cEffect.flags |= effFlagsNoSoundInStop;
    else
        cEffect.flags &= ~effFlagsNoSoundInStop;
}

bool AudioEffectX::getHostVendorString(char* text) {
    if (audioMaster)
        return audioMaster(&cEffect, audioMasterGetVendorString, 0, 0, text, 0) != 0;
    return false;
}

bool AudioEffectX::getHostProductString(char* text) {
    if (audioMaster)
        return audioMaster(&cEffect, audioMasterGetProductString, 0, 0, text, 0) != 0;
    return false;
}

VstInt32 AudioEffectX::getHostVendorVersion() {
    if (audioMaster)
        return (VstInt32)audioMaster(&cEffect, audioMasterGetVendorVersion, 0, 0, nullptr, 0);
    return 0;
}

VstInt32 AudioEffectX::canHostDo(char* text) {
    if (audioMaster)
        return (VstInt32)audioMaster(&cEffect, audioMasterCanDo, 0, 0, text, 0);
    return 0;
}

VstInt32 AudioEffectX::getHostLanguage() {
    if (audioMaster)
        return (VstInt32)audioMaster(&cEffect, audioMasterGetLanguage, 0, 0, nullptr, 0);
    return 0;
}

bool AudioEffectX::updateDisplay() {
    if (audioMaster)
        return audioMaster(&cEffect, audioMasterUpdateDisplay, 0, 0, nullptr, 0) != 0;
    return false;
}

bool AudioEffectX::sizeWindow(VstInt32 width, VstInt32 height) {
    if (audioMaster)
        return audioMaster(&cEffect, audioMasterSizeWindow, width, height, nullptr, 0) != 0;
    return false;
}

void AudioEffectX::resume() {
    AudioEffect::resume();
}
