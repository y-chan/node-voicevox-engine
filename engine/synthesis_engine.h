#ifndef SYNTHESIS_ENGINE_H
#define SYNTHESIS_ENGINE_H

#include <string>
#include <vector>

#include <napi.h>

#include "acoustic_feature_extractor.h"
#include "../core/core.h"

static std::vector<std::string> unvoiced_mora_phoneme_list = {
    "A", "I", "U", "E", "O", "cl", "pau"
};

static std::vector<std::string> mora_phoneme_list = {
    "a", "i", "u", "e", "o", "N", "A", "I", "U", "E", "O", "cl", "pau"
};

std::vector<Napi::Object> to_flatten_moras(Napi::Array accent_phrases);
std::vector<int64_t> to_phoneme_id_list(std::vector<std::string> phoneme_str_list);
std::vector<int64_t> to_accent_id_list(std::vector<std::string> accent_str_list);
void split_mora(
    std::vector<OjtPhoneme> phoneme_list,
    std::vector<OjtPhoneme> &consonant_phoneme_list,
    std::vector<OjtPhoneme> &vowel_phoneme_list,
    std::vector<long> &vowel_indexes
);
Napi::Array adjust_interrogative_accent_phrases(Napi::Env env, Napi::Array accent_phrases);
Napi::Array adjust_interrogative_moras(Napi::Env env, Napi::Object accent_phrase);
Napi::Object make_interrogative_mora(Napi::Env env, Napi::Object last_mora);

class SynthesisEngine {
public:
    const int default_sampling_rate = 48000;
    // workaround of Hiroshiba/voicevox_engine#128
    const float pre_padding_length = 0.4;

    SynthesisEngine(Core *core, OpenJTalk* openjtalk) {
        m_core = core;
        m_openjtalk = openjtalk;
    }
    void update_openjtalk(OpenJTalk *openjtalk) { m_openjtalk = openjtalk; }

    Napi::Array create_accent_phrases(Napi::Env env, Napi::String text, Napi::Number speaker_id);
    Napi::Array replace_mora_data(Napi::Array accent_phrases, long speaker_id);
    Napi::Array replace_phoneme_length(Napi::Array accent_phrases, int64_t speaker_id, std::vector<float> &pitches);
    Napi::Array replace_mora_pitch(Napi::Array accent_phrases, int64_t speaker_id, float *before_pitches = nullptr);
    Napi::Array synthesis_array(Napi::Env env, Napi::Object query, long speaker_id, bool enable_interrogative_upspeak = true);
    Napi::Buffer<char> synthesis_wave_format(Napi::Env env, Napi::Object query, long speaker_id, bool enable_interrogative_upspeak = true);
private:
    Core *m_core;
    OpenJTalk* m_openjtalk;

    std::vector<float> synthesis(Napi::Env env, Napi::Object query, int64_t speaker_id, bool enable_interrogative_upspeak = true);
    void initail_process(
        Napi::Array accent_phrases,
        std::vector<Napi::Object> &flatten_moras,
        std::vector<int64_t> &phoneme_id_list,
        std::vector<int64_t> &accent_id_list
    );
    void create_one_accent_list(std::vector<long> &accent_list, Napi::Object accent_phrase, int point);
};

#endif // SYNTHESIS_ENGINE_H
