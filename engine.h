#ifndef WRAPPER_H
#define WRAPPER_H

#include <napi.h>

#include "core/core.h"
#include "engine/openjtalk.h"
#include "engine/synthesis_engine.h"

class EngineWrapper : public Napi::ObjectWrap<EngineWrapper> {
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    static Napi::Object NewInstance(Napi::Env env, const Napi::CallbackInfo& info);

    EngineWrapper(const Napi::CallbackInfo& info);
    ~EngineWrapper();

    Napi::Value audio_query(const Napi::CallbackInfo& info);
    Napi::Value metas(const Napi::CallbackInfo& info);

    Napi::Value yukarin_s_forward(const Napi::CallbackInfo& info);
    Napi::Value yukarin_sa_forward(const Napi::CallbackInfo& info);
    // Napi::Value decode_forward(const Napi::CallbackInfo& info);

private:
    void create_execute_error(Napi::Env env, const char* func_name);
    Napi::Array create_accent_phrases(Napi::Env env, Napi::String text, Napi::Number speaker_id);

    Core* m_core;
    OpenJTalk* m_openjtalk;
    SynthesisEngine* m_engine;
};

#endif // WRAPPER_H
