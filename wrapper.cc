#include <napi.h>
#include <string>

#include "wrapper.h"

using namespace Napi;

Napi::Object CoreWrapper::NewInstance(Napi::Env env, const Napi::CallbackInfo& info)
{
    Napi::EscapableHandleScope scope(env);
    if (info.Length() < 2) {
        Napi::TypeError::New(env, "missing arguments").ThrowAsJavaScriptException();
        return Napi::Object::New(env);
    }

    if (!info[0].IsString() || !info[1].IsBoolean()) {
        Napi::TypeError::New(env, "wrong arguments").ThrowAsJavaScriptException();
        return Napi::Object::New(env);
    }

    const std::initializer_list<napi_value> initArgList = { info[0], info[1], info[2] };
    Napi::Object obj = env.GetInstanceData<Napi::FunctionReference>()->New(initArgList);
    return scope.Escape(napi_value(obj)).ToObject();
}

Napi::Object CoreWrapper::Init(Napi::Env env, Napi::Object exports)
{
    Napi::Function func = DefineClass(
        env, "CoreWrapper", {
            InstanceMethod("metas", &CoreWrapper::metas),
            InstanceMethod("yukarin_s_forward", &CoreWrapper::yukarin_s_forward),
            InstanceMethod("yukarin_sa_forward", &CoreWrapper::yukarin_sa_forward),
            InstanceMethod("decode_forward", &CoreWrapper::decode_forward),
        });

    Napi::FunctionReference* constructor = new Napi::FunctionReference();
    *constructor = Napi::Persistent(func);
    env.SetInstanceData(constructor);

    exports.Set("CoreWrapper", func);
    return exports;
}

CoreWrapper::CoreWrapper(const Napi::CallbackInfo& info)
    : Napi::ObjectWrap<CoreWrapper>(info)
{
    std::string core_file_path = info[0].As<Napi::String>().Utf8Value();
    bool use_gpu = info[1].As<Napi::Boolean>().Value();
    std::string root_dir_path;
    if (info[2].IsString()) {
        root_dir_path = info[2].As<Napi::String>().Utf8Value();
    } else {
        std::vector<std::string> split_path;
        std::string item;
        for (char ch: core_file_path) {
            if (ch == '/' || ch == '\\') {
                if (!item.empty()) split_path.push_back(item);
                item.clear();
            } else {
                item += ch;
            }
        }
        if (!item.empty()) split_path.push_back(item);
        // remove file name
        split_path.pop_back();
        if (core_file_path[0] == '/') {
            root_dir_path = std::string("/");
        }
        std::for_each(split_path.begin(), split_path.end(), [&](std::string path) {
            root_dir_path += path + std::string("/");
        });
    }
    try {
        m_core = new Core(core_file_path, root_dir_path, use_gpu);
    }
    catch (std::exception& err) {
        Napi::Error::New(info.Env(), err.what()).ThrowAsJavaScriptException();
    }
}

CoreWrapper::~CoreWrapper()
{
    delete m_core;
    m_core = nullptr;
}

void CoreWrapper::create_execute_error(Napi::Env env, const char* func_name)
{
    std::string err = std::string("failed to execute: ") + std::string(func_name);
    Napi::Error::New(env, err).ThrowAsJavaScriptException();
}

Napi::Value CoreWrapper::metas(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();
    Napi::String metas_string = Napi::String::New(env, m_core->metas());
    return metas_string;
}

Napi::Value CoreWrapper::yukarin_s_forward(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 2) {
        Napi::TypeError::New(env, "missing arguments").ThrowAsJavaScriptException();
        return env.Null();
    }

    bool wrong_arg = false;
    for (int i = 0; i < 2; i++) wrong_arg |= !info[i].IsArray();
    if (wrong_arg) {
        Napi::TypeError::New(env, "wrong arguments").ThrowAsJavaScriptException();
        return env.Null();
    }

    int length = info[0].As<Napi::Array>().Length();

    long *phoneme_list = (long *)malloc(sizeof(long) * length);
    for (int i = 0; i < length; i++) {
        Napi::Value val = info[0].As<Napi::Array>()[i];
        phoneme_list[i] = val.As<Napi::Number>().Int32Value();
    }

    long* speaker_id = (long *)malloc(sizeof(long) * info[1].As<Napi::Array>().Length());
    for (int i = 0; i < (int)info[1].As<Napi::Array>().Length(); i++) {
        Napi::Value val = info[1].As<Napi::Array>()[i];
        speaker_id[i] = val.As<Napi::Number>().Int32Value();
    }

    float* output = (float *)calloc(length, sizeof(float));

    if (!m_core->yukarin_s_forward(length, phoneme_list, speaker_id, output)) {
        create_execute_error(env, __func__);
        return env.Null();
    }

    Napi::Array output_array = Napi::Array::New(env, length);
    for (int i = 0; i < length; i++) {
        output_array[i] = Napi::Number::New(env, output[i]);
    }

    free(phoneme_list);
    free(speaker_id);
    free(output);

    return output_array;
}

Napi::Value CoreWrapper::yukarin_sa_forward(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 7) {
        Napi::TypeError::New(env, "missing arguments").ThrowAsJavaScriptException();
        return env.Null();
    }

    bool wrong_arg = false;
    for (int i = 0; i < 7; i++) wrong_arg |= !info[i].IsArray();
    if (wrong_arg) {
        Napi::TypeError::New(env, "wrong arguments").ThrowAsJavaScriptException();
        return env.Null();
    }

    Napi::Value length_base_array = info[0].As<Napi::Array>()[(uint32_t)0];
    int length = length_base_array.As<Napi::Array>().Length();

    int vpl_length = info[0].As<Napi::Array>().Length();
    long **vowel_phoneme_list = (long **)malloc(sizeof(long *) * vpl_length);
    long *base_vowel_phoneme_list = (long *)malloc(sizeof(long) * vpl_length * length);
    for (int i = 0; i < vpl_length; i++) {
        vowel_phoneme_list[i] = base_vowel_phoneme_list + i * length;
        Napi::Value val_array = info[0].As<Napi::Array>()[i];
        for (int j = 0; j < length; j++) {
            Napi::Value val = val_array.As<Napi::Array>()[j];
            vowel_phoneme_list[i][j] = val.As<Napi::Number>().Int32Value();
        }
    }

    int cpl_length = info[1].As<Napi::Array>().Length();
    long **consonant_phoneme_list = (long **)malloc(sizeof(long *) * cpl_length);
    long *base_consonant_phoneme_list = (long *)malloc(sizeof(long) * cpl_length * length);
    for (int i = 0; i < cpl_length; i++) {
        consonant_phoneme_list[i] = base_consonant_phoneme_list + i * length;
        Napi::Value val_array = info[1].As<Napi::Array>()[i];
        for (int j = 0; j < length; j++) {
            Napi::Value val = val_array.As<Napi::Array>()[j];
            consonant_phoneme_list[i][j] = val.As<Napi::Number>().Int32Value();
        }
    }

    int sal_length = info[2].As<Napi::Array>().Length();
    long **start_accent_list = (long **)malloc(sizeof(long *) * sal_length);
    long *base_start_accent_list = (long *)malloc(sizeof(long) * sal_length * length);
    for (int i = 0; i < sal_length; i++) {
        start_accent_list[i] = base_start_accent_list + i * length;
        Napi::Value val_array = info[2].As<Napi::Array>()[i];
        for (int j = 0; j < length; j++) {
            Napi::Value val = val_array.As<Napi::Array>()[j];
            start_accent_list[i][j] = val.As<Napi::Number>().Int32Value();
        }
    }

    int eal_length = info[3].As<Napi::Array>().Length();
    long **end_accent_list = (long **)malloc(sizeof(long *) * eal_length);
    long *base_end_accent_list = (long *)malloc(sizeof(long) * eal_length * length);
    for (int i = 0; i < eal_length; i++) {
        end_accent_list[i] = base_end_accent_list + i * length;
        Napi::Value val_array = info[3].As<Napi::Array>()[i];
        for (int j = 0; j < length; j++) {
            Napi::Value val = val_array.As<Napi::Array>()[j];
            end_accent_list[i][j] = val.As<Napi::Number>().Int32Value();
        }
    }

    int sapl_length = info[4].As<Napi::Array>().Length();
    long **start_accent_phrase_list = (long **)malloc(sizeof(long *) * sapl_length);
    long *base_start_accent_phrase_list = (long *)malloc(sizeof(long) * sapl_length * length);
    for (int i = 0; i < sapl_length; i++) {
        start_accent_phrase_list[i] = base_start_accent_phrase_list + i * length;
        Napi::Value val_array = info[4].As<Napi::Array>()[i];
        for (int j = 0; j < length; j++) {
            Napi::Value val = val_array.As<Napi::Array>()[j];
            start_accent_phrase_list[i][j] = val.As<Napi::Number>().Int32Value();
        }
    }

    int eapl_length = info[5].As<Napi::Array>().Length();
    long **end_accent_phrase_list = (long **)malloc(sizeof(long *) * eapl_length);
    long *base_end_accent_phrase_list = (long *)malloc(sizeof(long) * eapl_length * length);
    for (int i = 0; i < eapl_length; i++) {
        end_accent_phrase_list[i] = base_end_accent_phrase_list + i * length;
        Napi::Value val_array = info[5].As<Napi::Array>()[i];
        for (int j = 0; j < length; j++) {
            Napi::Value val = val_array.As<Napi::Array>()[j];
            end_accent_phrase_list[i][j] = val.As<Napi::Number>().Int32Value();
        }
    }

    int speakers = info[6].As<Napi::Array>().Length();
    long *speaker_id = (long *)malloc(sizeof(long) * speakers);
    for (int i = 0; i < speakers; i++) {
        Napi::Value val = info[6].As<Napi::Array>()[i];
        speaker_id[i] = val.As<Napi::Number>().Int32Value();
    }

    float **output = (float **)malloc(sizeof(float *) * speakers);
    float *base_output = (float *)calloc(length * speakers, sizeof(float));
    for (int i = 0; i < speakers; i++) {
        output[i] = base_output + i * length;
    }

    if (!m_core->yukarin_sa_forward(
        length,
        (long *)vowel_phoneme_list,
        (long *)consonant_phoneme_list,
        (long *)start_accent_list,
        (long *)end_accent_list,
        (long *)start_accent_phrase_list,
        (long *)end_accent_phrase_list,
        speaker_id,
        (float *)output
    )) {
        create_execute_error(env, __func__);
        return env.Null();
    }

    Napi::Array output_array = Napi::Array::New(env, speakers);
    for (int i = 0; i < speakers; i++) {
        Napi::Array new_array = Napi::Array::New(env, length);
        for (int j = 0; j < length; j++) {
            new_array[j] = Napi::Number::New(env, output[i][j]);
        }
        output_array[i] = new_array;
    }

    free(vowel_phoneme_list);
    free(base_vowel_phoneme_list);
    free(consonant_phoneme_list);
    free(base_consonant_phoneme_list);
    free(start_accent_list);
    free(base_start_accent_list);
    free(end_accent_list);
    free(base_end_accent_list);
    free(start_accent_phrase_list);
    free(base_start_accent_phrase_list);
    free(end_accent_phrase_list);
    free(base_end_accent_phrase_list);
    free(speaker_id);
    free(output);
    free(base_output);

    return output_array;
}

Napi::Value CoreWrapper::decode_forward(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 3) {
        Napi::TypeError::New(env, "missing arguments").ThrowAsJavaScriptException();
        return env.Null();
    }

    bool wrong_arg = false;
    for (int i = 0; i < 3; i++) wrong_arg |= !info[i].IsArray();
    if (wrong_arg) {
        Napi::TypeError::New(env, "wrong arguments").ThrowAsJavaScriptException();
        return env.Null();
    }

    int length = info[1].As<Napi::Array>().Length();

    int f0_length = info[0].As<Napi::Array>().Length();
    float **f0 = (float**)malloc(sizeof(float *) * f0_length);
    float *base_f0 = (float*)malloc(sizeof(float) * f0_length * length);
    for (int i = 0; i < f0_length; i++) {
        f0[i] = base_f0 + i * length;
        Napi::Value val_array = info[0].As<Napi::Array>()[i];
        for (int j = 0; j < length; j++) {
            Napi::Value val = val_array.As<Napi::Array>()[j];
            f0[i][j] = val.As<Napi::Number>().FloatValue();
        }
    }

    Napi::Value phoneme_item = info[1].As<Napi::Array>()[(uint32_t)0];
    int phoneme_size = phoneme_item.As<Napi::Array>().Length();
    float **phoneme = (float **)malloc(sizeof(float *) * length);
    float *base_phoneme = (float*)malloc(sizeof(float) * length * phoneme_size);
    for (int i = 0; i < length; i++) {
        phoneme[i] = base_phoneme + i * length;
        Napi::Value val_array = info[1].As<Napi::Array>()[i];
        for (int j = 0; j < length; j++) {
            Napi::Value val = val_array.As<Napi::Array>()[j];
            phoneme[i][j] = val.As<Napi::Number>().FloatValue();
        }
    }

    int speakers = info[2].As<Napi::Array>().Length();
    long *speaker_id = (long *)malloc(sizeof(long) * speakers);
    for (int i = 0; i < speakers; i++) {
        Napi::Value val = info[2].As<Napi::Array>()[i];
        speaker_id[i] = val.As<Napi::Number>().Int32Value();
    }

    float* output = (float*)calloc(length * 256, sizeof(float));

    if (!m_core->decode_forward(
        length, phoneme_size, (float *)f0, (float *)phoneme, speaker_id, output
    )) {
        create_execute_error(env, __func__);
        return env.Null();
    }

    Napi::Array output_array = Napi::Array::New(env, length);
    for (int i = 0; i < length; i++) {
        output_array[i] = Napi::Number::New(env, output[i]);
    }

    free(f0);
    free(base_f0);
    free(phoneme);
    free(base_phoneme);
    free(speaker_id);
    free(output);

    return output_array;
}

Napi::Object CreateObject(const Napi::CallbackInfo& info) {
    return CoreWrapper::NewInstance(info.Env(), info);
}

Napi::Object Initialize(Napi::Env env, Napi::Object exports) {
    Napi::Object new_exports = Napi::Function::New(env, CreateObject);
    return CoreWrapper::Init(env, new_exports);
}

NODE_API_MODULE(core, Initialize)
