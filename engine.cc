﻿#include <napi.h>

#include <algorithm>
#include <string>

#include "engine.h"
#include "engine/kana_parser.h"
#include "engine/user_dict.h"
#include "engine/nlohmann/json.hpp"

using namespace Napi;

Napi::Object EngineWrapper::NewInstance(Napi::Env env, const Napi::CallbackInfo& info)
{
    Napi::EscapableHandleScope scope(env);
    if (info.Length() < 5) {
        Napi::TypeError::New(env, "missing arguments").ThrowAsJavaScriptException();
        return Napi::Object::New(env);
    }

    if (!info[0].IsString() || !info[1].IsString() || !info[2].IsString() || !info[3].IsString() || !info[4].IsBoolean()) {
        Napi::TypeError::New(env, "wrong arguments").ThrowAsJavaScriptException();
        return Napi::Object::New(env);
    }

    const std::initializer_list<napi_value> initArgList = { info[0], info[1], info[2], info[3], info[4] };
    Napi::Object obj = env.GetInstanceData<Napi::FunctionReference>()->New(initArgList);
    return scope.Escape(napi_value(obj)).ToObject();
}

Napi::Object EngineWrapper::Init(Napi::Env env, Napi::Object exports)
{
    Napi::Function func = DefineClass(
        env, "EngineWrapper", {
            InstanceMethod("audio_query", &EngineWrapper::audio_query),
            InstanceMethod("accent_phrases", &EngineWrapper::accent_phrases),
            InstanceMethod("mora_data", &EngineWrapper::mora_data),
            InstanceMethod("mora_length", &EngineWrapper::mora_length),
            InstanceMethod("mora_pitch", &EngineWrapper::mora_pitch),
            InstanceMethod("synthesis", &EngineWrapper::synthesis),
            InstanceMethod("metas", &EngineWrapper::metas),
            InstanceMethod("yukarin_s_forward", &EngineWrapper::yukarin_s_forward),
            InstanceMethod("yukarin_sa_forward", &EngineWrapper::yukarin_sa_forward),
            InstanceMethod("decode_forward", &EngineWrapper::decode_forward),
            InstanceMethod("get_user_dict_words", &EngineWrapper::get_user_dict_words),
            InstanceMethod("add_user_dict_word", &EngineWrapper::add_user_dict_word),
            InstanceMethod("rewrite_user_dict_word", &EngineWrapper::rewrite_user_dict_word),
            InstanceMethod("delete_user_dict_word", &EngineWrapper::delete_user_dict_word),
        });

    Napi::FunctionReference* constructor = new Napi::FunctionReference();
    *constructor = Napi::Persistent(func);
    env.SetInstanceData(constructor);

    exports.Set("EngineWrapper", func);
    return exports;
}

EngineWrapper::EngineWrapper(const Napi::CallbackInfo& info)
    : Napi::ObjectWrap<EngineWrapper>(info)
{
    std::string openjtalk_dict = info[0].As<Napi::String>().Utf8Value();
    std::string default_dict_path = info[1].As<Napi::String>().Utf8Value();
    std::string user_dict_root = info[2].As<Napi::String>().Utf8Value();
    std::string core_file_path = info[3].As<Napi::String>().Utf8Value();
    bool use_gpu = info[4].As<Napi::Boolean>().Value();
    try {
        m_core = new Core(core_file_path, use_gpu);
        m_openjtalk = new OpenJTalk(openjtalk_dict);
        std::string user_dict_path = user_dict_root + "user_dict.json";
        std::string compiled_dict_path = user_dict_root + "user.dic";
        m_openjtalk->default_dict_path = default_dict_path;
        m_openjtalk->user_dict_path = user_dict_path;
        m_openjtalk->user_mecab = compiled_dict_path;
        m_openjtalk = user_dict_startup_processing(m_openjtalk);
        m_openjtalk = update_dict(m_openjtalk);
        m_engine = new SynthesisEngine(m_core, m_openjtalk);
    }
    catch (std::exception& err) {
        Napi::Error::New(info.Env(), err.what()).ThrowAsJavaScriptException();
    }
}

EngineWrapper::~EngineWrapper()
{
    m_core->finalize();
    delete m_core;
    m_core = nullptr;
}

void EngineWrapper::create_execute_error(Napi::Env env, const char* func_name)
{
    std::string last_error = std::string(m_core->last_error_message());
    std::string err = "failed to execute: " + std::string(func_name) + "\n" + last_error;
    Napi::Error::New(env, err).ThrowAsJavaScriptException();
}

Napi::Value EngineWrapper::audio_query(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 2) {
        Napi::TypeError::New(env, "missing arguments").ThrowAsJavaScriptException();
        return env.Null();
    }

    if (!info[0].IsString() || !info[1].IsNumber()) {
        Napi::TypeError::New(env, "wrong arguments").ThrowAsJavaScriptException();
        return env.Null();
    }

    Napi::Array accent_phrases;
    try {
        accent_phrases = m_engine->create_accent_phrases(env, info[0].As<Napi::String>(), info[1].As<Napi::Number>());
    } catch (std::exception& err) {
        Napi::Error::New(info.Env(), err.what()).ThrowAsJavaScriptException();
        return env.Null();
    }

    Napi::String kana;
    try {
        kana = create_kana(env, accent_phrases);
    }
    catch (std::exception& err) {
        Napi::Error::New(info.Env(), err.what()).ThrowAsJavaScriptException();
        return env.Null();
    }

    Napi::Object audio_query = Napi::Object::New(env);
    audio_query.Set("accent_phrases", accent_phrases);
    audio_query.Set("speedScale", 1);
    audio_query.Set("pitchScale", 0);
    audio_query.Set("intonationScale", 1);
    audio_query.Set("volumeScale", 1);
    audio_query.Set("prePhonemeLength", 0.1);
    audio_query.Set("postPhonemeLength", 0.1);
    audio_query.Set("outputSamplingRate", m_engine->default_sampling_rate);
    audio_query.Set("outputStereo", false);
    audio_query.Set("kana", kana);

    return audio_query;
}

Napi::Value EngineWrapper::accent_phrases(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 3) {
        Napi::TypeError::New(env, "missing arguments").ThrowAsJavaScriptException();
        return env.Null();
    }

    if (!info[0].IsString() || !info[1].IsNumber() || !info[2].IsBoolean()) {
        Napi::TypeError::New(env, "wrong arguments").ThrowAsJavaScriptException();
        return env.Null();
    }

    Napi::Array accent_phrases;
    if (info[2].As<Napi::Boolean>().Value()) {
        try {
            accent_phrases = parse_kana(env, info[0].As<Napi::String>().Utf8Value());
        }
        catch (std::exception& err) {
            Napi::Error::New(info.Env(), err.what()).ThrowAsJavaScriptException();
            return env.Null();
        }
        accent_phrases = m_engine->replace_mora_data(accent_phrases, info[1].As<Napi::Number>().Int64Value());
    }
    else {
        accent_phrases = m_engine->create_accent_phrases(env, info[0].As<Napi::String>(), info[1].As<Napi::Number>());
    }

    return accent_phrases;
}

Napi::Value EngineWrapper::mora_data(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 2) {
        Napi::TypeError::New(env, "missing arguments").ThrowAsJavaScriptException();
        return env.Null();
    }

    // TODO: 厳密な型検査を行いたいが、パフォーマンスの低下が懸念
    if (!info[0].IsArray() || !info[1].IsNumber()) {
        Napi::TypeError::New(env, "wrong arguments").ThrowAsJavaScriptException();
        return env.Null();
    }

    return m_engine->replace_mora_data(info[0].As<Napi::Array>(), info[1].As<Napi::Number>().Int64Value());
}

Napi::Value EngineWrapper::mora_length(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 2) {
        Napi::TypeError::New(env, "missing arguments").ThrowAsJavaScriptException();
        return env.Null();
    }

    // TODO: 厳密な型検査
    if (!info[0].IsArray() || !info[1].IsNumber()) {
        Napi::TypeError::New(env, "wrong arguments").ThrowAsJavaScriptException();
        return env.Null();
    }

    return m_engine->replace_phoneme_length(info[0].As<Napi::Array>(), info[1].As<Napi::Number>().Int64Value());
}

Napi::Value EngineWrapper::mora_pitch(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 2) {
        Napi::TypeError::New(env, "missing arguments").ThrowAsJavaScriptException();
        return env.Null();
    }

    // TODO: 厳密な型検査
    if (!info[0].IsArray() || !info[1].IsNumber()) {
        Napi::TypeError::New(env, "wrong arguments").ThrowAsJavaScriptException();
        return env.Null();
    }

    return m_engine->replace_mora_pitch(info[0].As<Napi::Array>(), info[1].As<Napi::Number>().Int64Value());
}

Napi::Value EngineWrapper::synthesis(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 3) {
        Napi::TypeError::New(env, "missing arguments").ThrowAsJavaScriptException();
        return env.Null();
    }

    if (!info[0].IsObject() || !info[1].IsNumber() || !info[2].IsBoolean()) {
        Napi::TypeError::New(env, "wrong arguments").ThrowAsJavaScriptException();
        return env.Null();
    }

    Napi::Object audio_query = info[0].As<Napi::Object>();
    if (
        !audio_query.Has("accent_phrases") ||
        !audio_query.Has("speedScale") ||
        !audio_query.Has("pitchScale") ||
        !audio_query.Has("intonationScale") ||
        !audio_query.Has("volumeScale") ||
        !audio_query.Has("prePhonemeLength") ||
        !audio_query.Has("postPhonemeLength") ||
        !audio_query.Has("outputSamplingRate") ||
        !audio_query.Has("outputStereo") ||
        !audio_query.Has("kana")
    ) {
        Napi::TypeError::New(env, "wrong audio query").ThrowAsJavaScriptException();
        return env.Null();
    }

    // TODO: accent_phraseの厳密な型検査
    Napi::Value accent_phrases = audio_query.Get("accent_phrases");
    Napi::Value speed_scale = audio_query.Get("speedScale");
    Napi::Value pitch_scale = audio_query.Get("pitchScale");
    Napi::Value intonation_scale = audio_query.Get("intonationScale");
    Napi::Value volume_scale = audio_query.Get("volumeScale");
    Napi::Value pre_phoneme_length = audio_query.Get("prePhonemeLength");
    Napi::Value post_phoneme_length = audio_query.Get("postPhonemeLength");
    Napi::Value output_sampling_rate = audio_query.Get("outputSamplingRate");
    Napi::Value output_stereo = audio_query.Get("outputStereo");
    Napi::Value kana = audio_query.Get("kana");

    if (!accent_phrases.IsArray() ||
        !speed_scale.IsNumber() ||
        !pitch_scale.IsNumber() ||
        !intonation_scale.IsNumber() ||
        !volume_scale.IsNumber() ||
        !pre_phoneme_length.IsNumber() ||
        !post_phoneme_length.IsNumber() ||
        !output_sampling_rate.IsNumber() ||
        !output_stereo.IsBoolean() ||
        !kana.IsString()
    ) {
        Napi::TypeError::New(env, "wrong audio query params").ThrowAsJavaScriptException();
        return env.Null();
    }

    return m_engine->synthesis_wave_format(env, audio_query, info[1].As<Napi::Number>().Int64Value(), info[2].As<Napi::Boolean>().Value());
}

Napi::Value EngineWrapper::metas(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();
    Napi::String metas_string = Napi::String::New(env, m_core->metas());
    return metas_string;
}

Napi::Value EngineWrapper::yukarin_s_forward(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 2) {
        Napi::TypeError::New(env, "missing arguments").ThrowAsJavaScriptException();
        return env.Null();
    }

    if (!info[0].IsArray() || !info[1].IsNumber()) {
        Napi::TypeError::New(env, "wrong arguments").ThrowAsJavaScriptException();
        return env.Null();
    }

    int length = (int)info[0].As<Napi::Array>().Length();

    std::vector<long> phoneme_list(length);
    for (int i = 0; i < length; i++) {
        Napi::Value val = info[0].As<Napi::Array>()[i];
        phoneme_list[i] = (long)val.As<Napi::Number>().Int64Value();
    }

    long speaker_id = info[1].As<Napi::Number>().Int64Value();

    std::vector<float> output(length, 0);

    bool success = m_core->yukarin_s_forward(length, phoneme_list.data(), &speaker_id, output.data());

    if (!success) {
        create_execute_error(env, __func__);
        return env.Null();
    }

    Napi::Array output_array = Napi::Array::New(env, length);
    for (int i = 0; i < length; i++) {
        output_array[i] = Napi::Number::New(env, output[i]);
    }

    return output_array;
}

Napi::Value EngineWrapper::yukarin_sa_forward(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 7) {
        Napi::TypeError::New(env, "missing arguments").ThrowAsJavaScriptException();
        return env.Null();
    }

    bool wrong_arg = false;
    for (int i = 0; i < 6; i++) wrong_arg |= !info[i].IsArray();
    if (wrong_arg || !info[6].IsNumber()) {
        Napi::TypeError::New(env, "wrong arguments").ThrowAsJavaScriptException();
        return env.Null();
    }

    int length = info[0].As<Napi::Array>().Length();
    std::vector<long> vowel_phoneme_list(length);
    std::vector<long> consonant_phoneme_list(length);
    std::vector<long> start_accent_list(length);
    std::vector<long> end_accent_list(length);
    std::vector<long> start_accent_phrase_list(length);
    std::vector<long> end_accent_phrase_list(length);

    for (int i = 0; i < length; i++) {
        Napi::Value val0 = info[0].As<Napi::Array>()[i];
        vowel_phoneme_list[i] = val0.As<Napi::Number>().Int32Value();

        Napi::Value val1 = info[1].As<Napi::Array>()[i];
        consonant_phoneme_list[i] = val1.As<Napi::Number>().Int32Value();

        Napi::Value val2 = info[2].As<Napi::Array>()[i];
        start_accent_list[i] = val2.As<Napi::Number>().Int32Value();

        Napi::Value val3 = info[3].As<Napi::Array>()[i];
        end_accent_list[i] = val3.As<Napi::Number>().Int32Value();

        Napi::Value val4 = info[4].As<Napi::Array>()[i];
        start_accent_phrase_list[i] = val4.As<Napi::Number>().Int32Value();

        Napi::Value val5 = info[5].As<Napi::Array>()[i];
        end_accent_phrase_list[i] = val5.As<Napi::Number>().Int32Value();
    }

    long speaker_id = info[6].As<Napi::Number>().Int64Value();

    std::vector<float> output(length, 0);

    if (!m_core->yukarin_sa_forward(
        length,
        vowel_phoneme_list.data(),
        consonant_phoneme_list.data(),
        start_accent_list.data(),
        end_accent_list.data(),
        start_accent_phrase_list.data(),
        end_accent_phrase_list.data(),
        &speaker_id,
        output.data()
    )) {
        create_execute_error(env, __func__);
        return env.Null();
    }

    Napi::Array output_array = Napi::Array::New(env, length);
    for (int i = 0; i < length; i++) {
        output_array[i] = Napi::Number::New(env, output[i]);
    }

    return output_array;
}


Napi::Value EngineWrapper::decode_forward(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 3) {
        Napi::TypeError::New(env, "missing arguments").ThrowAsJavaScriptException();
        return env.Null();
    }

    if (!info[0].IsArray() || !info[1].IsArray() || !info[2].IsNumber()) {
        Napi::TypeError::New(env, "wrong arguments").ThrowAsJavaScriptException();
        return env.Null();
    }

    Napi::Array f0_array = info[0].As<Napi::Array>();
    Napi::Array phoneme_array = info[1].As<Napi::Array>();

    if (!phoneme_array.IsArray()) {
        Napi::TypeError::New(env, "wrong arguments").ThrowAsJavaScriptException();
        return env.Null();
    }

    Napi::Value phoneme_array_value = phoneme_array[(uint32_t)0];
    Napi::Array phoneme_array_array = phoneme_array_value.As<Napi::Array>();

    Napi::Value phoneme_value = phoneme_array_array[(uint32_t)0];

    if (!phoneme_value.IsNumber()) {
        Napi::TypeError::New(env, "wrong arguments").ThrowAsJavaScriptException();
        return env.Null();
    }

    int length = info[0].As<Napi::Array>().Length();
    int phoneme_size = phoneme_array_array.Length();

    std::vector<float> f0(length);
    std::vector<float> phoneme(length * phoneme_size);

    for (int i = 0; i < length; i++) {
        Napi::Value val1 = f0_array[i];
        f0[i] = val1.As<Napi::Number>().FloatValue();

        phoneme_array_value = phoneme_array[i];
        phoneme_array_array = phoneme_array_value.As<Napi::Array>();
        for (int j = 0; j < phoneme_size; j++) {
            Napi::Value val2 = phoneme_array_array[j];
            phoneme[i * length + j] = val2.As<Napi::Number>().FloatValue();
        }
    }

    long speaker_id = info[2].As<Napi::Number>().Int64Value();

    int output_size = length * 256;
    std::vector<float> output(output_size, 0.0);

    if (!m_core->decode_forward(
        length, phoneme_size, f0.data(), phoneme.data(), &speaker_id, output.data()
    )) {
        create_execute_error(env, __func__);
        return env.Null();
    }

    Napi::Array output_array = Napi::Array::New(env, output_size);
    for (int i = 0; i < output_size; i++) {
        output_array[i] = Napi::Number::New(env, output[i]);
    }

    return output_array;
}

Napi::Value EngineWrapper::get_user_dict_words(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    json user_dict;
    try {
        user_dict = read_dict(m_openjtalk->user_dict_path);
    } catch (std::exception& err) {
        Napi::Error::New(info.Env(), err.what()).ThrowAsJavaScriptException();
        return env.Null();
    }
    Napi::Object result = Napi::Object::New(env);
    for (auto &item : user_dict.items()) {
        std::string key = item.key();
        json value = item.value();
        Napi::Object result_child = Napi::Object::New(env);
        result_child.Set("surface", value["surface"].get<std::string>());
        result_child.Set("priority", value["priority"].get<int>());
        result_child.Set("context_id", value["context_id"].get<int>());
        result_child.Set("part_of_speech", value["part_of_speech"].get<std::string>());
        result_child.Set("part_of_speech_detail_1", value["part_of_speech_detail_1"].get<std::string>());
        result_child.Set("part_of_speech_detail_2", value["part_of_speech_detail_2"].get<std::string>());
        result_child.Set("part_of_speech_detail_3", value["part_of_speech_detail_3"].get<std::string>());
        result_child.Set("inflectional_type", value["inflectional_type"].get<std::string>());
        result_child.Set("inflectional_form", value["inflectional_form"].get<std::string>());
        result_child.Set("stem", value["stem"].get<std::string>());
        result_child.Set("yomi", value["yomi"].get<std::string>());
        result_child.Set("pronunciation", value["pronunciation"].get<std::string>());
        result_child.Set("accent_type", value["accent_type"].get<int>());
        result_child.Set("mora_count", value["mora_count"].get<int>());
        result_child.Set("accent_associative_rule", value["accent_associative_rule"].get<std::string>());
        result[key] = result_child;
    }
    return result;
}

Napi::Value EngineWrapper::add_user_dict_word(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 3) {
        Napi::TypeError::New(env, "missing arguments").ThrowAsJavaScriptException();
        return env.Null();
    }

    if (!info[0].IsString() || !info[1].IsString() || !info[2].IsNumber()) {
        Napi::TypeError::New(env, "wrong arguments").ThrowAsJavaScriptException();
        return env.Null();
    }

    if (info.Length() >= 4 && !(info[3].IsUndefined() || info[3].IsString())) {
        Napi::TypeError::New(env, "wrong arguments").ThrowAsJavaScriptException();
        return env.Null();
    }

    if (info.Length() >= 5 && !(info[4].IsUndefined() || info[4].IsNumber())) {
        Napi::TypeError::New(env, "wrong arguments").ThrowAsJavaScriptException();
        return env.Null();
    }

    std::string surface = info[0].As<Napi::String>().Utf8Value();
    std::string pronunciation = info[1].As<Napi::String>().Utf8Value();
    int accent_type = info[2].As<Napi::Number>().Int32Value();
    std::string *word_type = nullptr;
    std::string word_type_value;
    if (info[3].IsString()) {
        word_type_value = info[3].As<Napi::String>().Utf8Value();
        word_type = &word_type_value;
    }
    int *priority = nullptr;
    int priority_value;
    if (info[4].IsNumber()) {
        priority_value = info[4].As<Napi::Number>().Int32Value();
        priority = &priority_value;
    }
    std::string word_uuid;
    try {
        auto result = apply_word(
            m_openjtalk,
            surface,
            pronunciation,
            accent_type,
            word_type,
            priority
        );
        word_uuid = result.first;
        m_openjtalk = result.second;
        m_engine->update_openjtalk(m_openjtalk);
    } catch (std::exception& err) {
        Napi::Error::New(info.Env(), err.what()).ThrowAsJavaScriptException();
        return env.Null();
    }

    return Napi::String::New(env, word_uuid);
}

Napi::Value EngineWrapper::rewrite_user_dict_word(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 4) {
        Napi::TypeError::New(env, "missing arguments").ThrowAsJavaScriptException();
        return env.Null();
    }

    if (!info[0].IsString() || !info[1].IsString() || !info[2].IsNumber() || !info[3].IsString()) {
        Napi::TypeError::New(env, "wrong arguments").ThrowAsJavaScriptException();
        return env.Null();
    }

    if (info.Length() >= 5 && !(info[4].IsUndefined() || info[4].IsString())) {
        Napi::TypeError::New(env, "wrong arguments").ThrowAsJavaScriptException();
        return env.Null();
    }

    if (info.Length() == 6 && !(info[5].IsUndefined() || info[5].IsNumber())) {
        Napi::TypeError::New(env, "wrong arguments").ThrowAsJavaScriptException();
        return env.Null();
    }

    std::string surface = info[0].As<Napi::String>().Utf8Value();
    std::string pronunciation = info[1].As<Napi::String>().Utf8Value();
    int accent_type = info[2].As<Napi::Number>().Int32Value();
    std::string word_uuid = info[3].As<Napi::String>().Utf8Value();
    std::string *word_type = nullptr;
    std::string word_type_value;
    if (info[4].IsString()) {
        word_type_value = info[4].As<Napi::String>().Utf8Value();
        word_type = &word_type_value;
    }
    int *priority = nullptr;
    int priority_value;
    if (info[5].IsNumber()) {
        priority_value = info[5].As<Napi::Number>().Int32Value();
        priority = &priority_value;
    }
    try {
        m_openjtalk = rewrite_word(
            m_openjtalk,
            word_uuid,
            surface,
            pronunciation,
            accent_type,
            word_type,
            priority
        );
        m_engine->update_openjtalk(m_openjtalk);
    } catch (std::exception& err) {
        Napi::Error::New(info.Env(), err.what()).ThrowAsJavaScriptException();
        return env.Null();
    }

    return env.Null();
}

Napi::Value EngineWrapper::delete_user_dict_word(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1) {
        Napi::TypeError::New(env, "missing arguments").ThrowAsJavaScriptException();
        return env.Null();
    }

    if (!info[0].IsString()) {
        Napi::TypeError::New(env, "wrong arguments").ThrowAsJavaScriptException();
        return env.Null();
    }

    std::string word_uuid = info[0].As<Napi::String>().Utf8Value();
    try {
        m_openjtalk = delete_word(
            m_openjtalk,
            word_uuid
        );
        m_engine->update_openjtalk(m_openjtalk);
    } catch (std::exception& err) {
        Napi::Error::New(info.Env(), err.what()).ThrowAsJavaScriptException();
        return env.Null();
    }

    return env.Null();
}

Napi::Object CreateObject(const Napi::CallbackInfo& info) {
    return EngineWrapper::NewInstance(info.Env(), info);
}

Napi::Object Initialize(Napi::Env env, Napi::Object exports) {
    Napi::Object new_exports = Napi::Function::New(env, CreateObject);
    return EngineWrapper::Init(env, new_exports);
}

NODE_API_MODULE(core, Initialize)
