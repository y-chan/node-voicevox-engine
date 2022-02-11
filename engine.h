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
    Napi::Value accent_phrases(const Napi::CallbackInfo& info);
    Napi::Value mora_data(const Napi::CallbackInfo& info);
    Napi::Value mora_length(const Napi::CallbackInfo& info);
    Napi::Value mora_pitch(const Napi::CallbackInfo& info);
    Napi::Value synthesis(const Napi::CallbackInfo& info);

    Napi::Value metas(const Napi::CallbackInfo& info);

    Napi::Value yukarin_s_forward(const Napi::CallbackInfo& info);
    Napi::Value yukarin_sa_forward(const Napi::CallbackInfo& info);
    Napi::Value decode_forward(const Napi::CallbackInfo& info);

private:
    void create_execute_error(Napi::Env env, const char* func_name);

    Core* m_core;
    SynthesisEngine* m_engine;
};

#endif // WRAPPER_H
