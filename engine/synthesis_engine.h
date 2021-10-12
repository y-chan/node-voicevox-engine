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
std::vector<OjtPhoneme> to_phoneme_data_list(std::vector<std::string> phoneme_str_list);
void split_mora(
    std::vector<OjtPhoneme> phoneme_list,
    std::vector<OjtPhoneme *> &consonant_phoneme_list,
    std::vector<OjtPhoneme> &vowel_phoneme_list,
    std::vector<long> &vowel_indexes
);

class SynthesisEngine {
public:
    const int default_sampling_rate = 24000;
    // workaround of Hiroshiba/voicevox_engine#128
    const float pre_padding_length = 0.4;

    SynthesisEngine(Core *core) {
        m_core = core;
    }

    Napi::Array replace_mora_data(Napi::Array accent_phrases, long speaker_id);
    Napi::Array replace_phoneme_length(Napi::Array accent_phrases, long speaker_id);
    Napi::Array replace_mora_pitch(Napi::Array accent_phrases, long speaker_id);
    Napi::Array synthesis_array(Napi::Env env, Napi::Object query, long speaker_id);
    Napi::Buffer<char> synthesis_wave_format(Napi::Env env, Napi::Object query, long speaker_id);
private:
    Core *m_core;

    std::vector<float> synthesis(Napi::Object query, long speaker_id);
    void initail_process(
        Napi::Array accent_phrases,
        std::vector<Napi::Object> &flatten_moras,
        std::vector<std::string> &phoneme_str_list,
        std::vector<OjtPhoneme> &phoneme_data_list
    );
    void create_one_accent_list(std::vector<long> &accent_list, Napi::Object accent_phrase, int point);
};

#endif // SYNTHESIS_ENGINE_H
