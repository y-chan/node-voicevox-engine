#include <napi.h>

#include <algorithm>
#include <string>
#include <iostream>

#include "engine.h"
#include "engine/full_context_label.h"
#include "engine/kana_parser.h"
#include "engine/mora_list.h"

using namespace Napi;

Napi::Object EngineWrapper::NewInstance(Napi::Env env, const Napi::CallbackInfo& info)
{
    Napi::EscapableHandleScope scope(env);
    if (info.Length() < 3) {
        Napi::TypeError::New(env, "missing arguments").ThrowAsJavaScriptException();
        return Napi::Object::New(env);
    }

    if (!info[0].IsString() || !info[1].IsString() || !info[2].IsBoolean()) {
        Napi::TypeError::New(env, "wrong arguments").ThrowAsJavaScriptException();
        return Napi::Object::New(env);
    }

    const std::initializer_list<napi_value> initArgList = { info[0], info[1], info[2], info[3] };
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
            // InstanceMethod("decode_forward", &EngineWrapper::decode_forward),
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
    std::string core_file_path = info[1].As<Napi::String>().Utf8Value();
    bool use_gpu = info[2].As<Napi::Boolean>().Value();
    std::string root_dir_path;
    if (info[3].IsString()) {
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
        m_openjtalk = new OpenJTalk(openjtalk_dict);
        m_engine = new SynthesisEngine(m_core);
    }
    catch (std::exception& err) {
        Napi::Error::New(info.Env(), err.what()).ThrowAsJavaScriptException();
    }
}

EngineWrapper::~EngineWrapper()
{
    delete m_core;
    m_core = nullptr;
}

void EngineWrapper::create_execute_error(Napi::Env env, const char* func_name)
{
    std::string last_error = std::string(m_core->last_error_message());
    std::string err = std::string("failed to execute: ") + std::string(func_name) + std::string("\n") + last_error;
    Napi::Error::New(env, err).ThrowAsJavaScriptException();
}

Napi::Array EngineWrapper::create_accent_phrases(Napi::Env env, Napi::String text, Napi::Number speaker_id) {
    std::string str_text = text.Utf8Value();
    if (str_text.size() == 0) {
        return Napi::Array::New(env);
    }

    Utterance utterance = extract_full_context_label(m_openjtalk, str_text);
    if (utterance.breath_groups.size() == 0) {
        return Napi::Array::New(env);
    }

    int accent_phrases_size = 0;
    for (BreathGroup *breath_group : utterance.breath_groups) accent_phrases_size += breath_group->accent_phrases.size();
    Napi::Array accent_phrases = Napi::Array::New(env, accent_phrases_size);

    int accent_phrases_count = 0;
    for (size_t i = 0; i < utterance.breath_groups.size(); i++) {
        BreathGroup *breath_group = utterance.breath_groups[i];
        for (size_t j = 0; j < breath_group->accent_phrases.size(); j++) {
            AccentPhrase *accent_phrase = breath_group->accent_phrases[j];
            Napi::Object new_accent_phrase = Napi::Object::New(env);

            Napi::Array moras = Napi::Array::New(env, accent_phrase->moras.size());
            for (size_t k = 0; k < accent_phrase->moras.size(); k++) {
                Mora *mora = accent_phrase->moras[k];
                Napi::Object new_mora = Napi::Object::New(env);

                std::string moras_text = "";
                for (Phoneme *phoneme : mora->phonemes()) moras_text += phoneme->phoneme();
                std::transform(moras_text.begin(), moras_text.end(), moras_text.begin(), ::tolower);
                if (moras_text == "n") moras_text = "N";
                new_mora.Set("text", mora2text(moras_text));

                if (mora->consonant != nullptr) {
                    new_mora.Set("consonant", mora->consonant->phoneme());
                    new_mora.Set("consonant_length", 0.0f);
                }
                new_mora.Set("vowel", mora->vowel->phoneme());
                new_mora.Set("vowel_length", 0.0f);
                new_mora.Set("pitch", 0.0f);

                moras[k] = new_mora;
            }

            new_accent_phrase.Set("moras", moras);
            new_accent_phrase.Set("accent", accent_phrase->accent);

            if (i != utterance.breath_groups.size() - 1 && j == breath_group->accent_phrases.size() - 1) {
                Napi::Object pause_mora = Napi::Object::New(env);
                pause_mora.Set("text", "、");
                pause_mora.Set("vowel", "pau");
                pause_mora.Set("vowel_length", 0.0f);
                pause_mora.Set("pitch", 0.0f);

                new_accent_phrase.Set("pause_mora", pause_mora);
            }
            accent_phrases[accent_phrases_count] = new_accent_phrase;
            accent_phrases_count++;
        }
    }

    accent_phrases = m_engine->replace_mora_data(accent_phrases, speaker_id.Int64Value());

    return accent_phrases;
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
        accent_phrases = create_accent_phrases(env, info[0].As<Napi::String>(), info[1].As<Napi::Number>());
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
        accent_phrases = create_accent_phrases(env, info[0].As<Napi::String>(), info[1].As<Napi::Number>());
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
    if (info.Length() < 2) {
        Napi::TypeError::New(env, "missing arguments").ThrowAsJavaScriptException();
        return env.Null();
    }

    if (!info[0].IsObject() || !info[1].IsNumber()) {
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

    return m_engine->synthesis_wave_format(env, audio_query, info[1].As<Napi::Number>().Int64Value());
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
        std::cout << phoneme_list[i] << std::endl;
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

/*
Napi::Value EngineWrapper::decode_forward(const Napi::CallbackInfo& info)
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
 */

Napi::Object CreateObject(const Napi::CallbackInfo& info) {
    return EngineWrapper::NewInstance(info.Env(), info);
}

Napi::Object Initialize(Napi::Env env, Napi::Object exports) {
    Napi::Object new_exports = Napi::Function::New(env, CreateObject);
    return EngineWrapper::Init(env, new_exports);
}

NODE_API_MODULE(core, Initialize)
