#ifndef WRAPPER_H
#define WRAPPER_H

#include <napi.h>

#include "core/core.h"

class CoreWrapper : public Napi::ObjectWrap<CoreWrapper> {
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    static Napi::Object NewInstance(Napi::Env env, const Napi::CallbackInfo& info);

    CoreWrapper(const Napi::CallbackInfo& info);
    ~CoreWrapper();

    Napi::Value metas(const Napi::CallbackInfo& info);

    Napi::Value yukarin_s_forward(const Napi::CallbackInfo& info);
    Napi::Value yukarin_sa_forward(const Napi::CallbackInfo& info);
    Napi::Value decode_forward(const Napi::CallbackInfo& info);

private:
    void create_execute_error(Napi::Env env, const char* func_name);

    Core* m_core;
};

#endif // WRAPPER_H
