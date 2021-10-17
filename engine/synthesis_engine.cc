#include <iterator>
#include <sstream>

#include "synthesis_engine.h"

std::vector<Napi::Object> to_flatten_moras(Napi::Array accent_phrases) {
    std::vector<Napi::Object> flatten_moras;

    for (uint32_t i = 0; i < accent_phrases.Length(); i++) {
        Napi::Value accent_phrase = accent_phrases[i];
        Napi::Object accent_phrase_object = accent_phrase.As<Napi::Object>();
        Napi::Array moras = accent_phrase_object.Get("moras").As<Napi::Array>();
        for (uint32_t j = 0; j < moras.Length(); j++) {
            Napi::Value mora = moras[j];
            flatten_moras.push_back(mora.As<Napi::Object>());
        }
        if (accent_phrase_object.Has("pause_mora")) {
            Napi::Object pause_mora = accent_phrase_object.Get("pause_mora").As<Napi::Object>();
            flatten_moras.push_back(pause_mora);
        }
    }

    return flatten_moras;
}


std::vector<OjtPhoneme> to_phoneme_data_list(std::vector<std::string> phoneme_str_list) {
    std::vector<OjtPhoneme> phoneme_data_list;
    for (size_t i = 0; i < phoneme_str_list.size(); i++) {
        phoneme_data_list.push_back(OjtPhoneme(phoneme_str_list[i], (float)i, (float)i + 1.0));
    }
    return OjtPhoneme::convert(phoneme_data_list);
}

void split_mora(
    std::vector<OjtPhoneme> phoneme_list,
    std::vector<OjtPhoneme> &consonant_phoneme_list,
    std::vector<OjtPhoneme> &vowel_phoneme_list,
    std::vector<long> &vowel_indexes
) {
    for (size_t i = 0; i < phoneme_list.size(); i++) {
        std::vector<std::string>::iterator result = std::find(
            mora_phoneme_list.begin(),
            mora_phoneme_list.end(),
            phoneme_list[i].phoneme
        );
        if (result != mora_phoneme_list.end()) {
            vowel_indexes.push_back((long)i);
        }
    }
    for (int index : vowel_indexes) {
        vowel_phoneme_list.push_back(phoneme_list[index]);
    }
    consonant_phoneme_list.push_back(OjtPhoneme());
    for (size_t i = 1; i < vowel_indexes.size(); i++) {
        int prev = vowel_indexes[vowel_indexes.size() - 1 - i];
        int next = vowel_indexes[i];
        if (next - prev == 1) {
            consonant_phoneme_list.push_back(OjtPhoneme());
        } else {
            consonant_phoneme_list.push_back(phoneme_list[next - 1]);
        }
    }
    return;
}


Napi::Array SynthesisEngine::replace_mora_data(Napi::Array accent_phrases, long speaker_id) {
    return replace_mora_pitch(
        replace_phoneme_length(
            accent_phrases,
            speaker_id
        ),
        speaker_id
    );
}

Napi::Array SynthesisEngine::replace_phoneme_length(Napi::Array accent_phrases, int64_t speaker_id) {
    std::vector<Napi::Object> flatten_moras;
    std::vector<std::string> phoneme_str_list;
    std::vector<OjtPhoneme> phoneme_data_list;
    initail_process(accent_phrases, flatten_moras, phoneme_str_list, phoneme_data_list);

    std::vector<OjtPhoneme> consonant_phoneme_list;
    std::vector<OjtPhoneme> vowel_phoneme_list;
    std::vector<long> vowel_indexes_data;
    split_mora(phoneme_data_list, consonant_phoneme_list, vowel_phoneme_list, vowel_indexes_data);

    std::vector<int64_t> phoneme_list_s;
    for (OjtPhoneme phoneme_data : phoneme_data_list) phoneme_list_s.push_back(phoneme_data.phoneme_id());
    std::vector<float> phoneme_length(phoneme_list_s.size(), 0.0);
    bool success = m_core->yukarin_s_forward(phoneme_list_s.size(), (long *)phoneme_list_s.data(), (long *)&speaker_id, phoneme_length.data());

    if (!success) {
        throw std::runtime_error(m_core->last_error_message());
    }

    int index = 0;
    for (uint32_t i = 0; i < accent_phrases.Length(); i++) {
        Napi::Value accent_phrase = accent_phrases[i];
        Napi::Object accent_phrase_object = accent_phrase.As<Napi::Object>();
        Napi::Array moras = accent_phrase_object.Get("moras").As<Napi::Array>();
        for (uint32_t j = 0; j < moras.Length(); j++) {
            Napi::Value mora = moras[j];
            Napi::Object mora_object = mora.As<Napi::Object>();
            if (mora_object.Get("consonant").IsString()) mora_object.Set("consonant_length", phoneme_length[vowel_indexes_data[index + 1] - 1]);
            mora_object.Set("vowel_length", phoneme_length[vowel_indexes_data[index + 1]]);
            index++;
            moras.Set(j, mora_object);
        }
        accent_phrase_object.Set("moras", moras);
        if (accent_phrase_object.Has("pause_mora")) {
            Napi::Object pause_mora = accent_phrase_object.Get("pause_mora").As<Napi::Object>();
            pause_mora.Set("vowel_length", phoneme_length[vowel_indexes_data[index + 1]]);
            index++;
            accent_phrase_object.Set("pause_mora", pause_mora);
        }
        accent_phrases.Set(i, accent_phrase_object);
    }

    return accent_phrases;
}

Napi::Array SynthesisEngine::replace_mora_pitch(Napi::Array accent_phrases, long speaker_id) {
    std::vector<Napi::Object> flatten_moras;
    std::vector<std::string> phoneme_str_list;
    std::vector<OjtPhoneme> phoneme_data_list;
    initail_process(accent_phrases, flatten_moras, phoneme_str_list, phoneme_data_list);

    std::vector<long> base_start_accent_list;
    std::vector<long> base_end_accent_list;
    std::vector<long> base_start_accent_phrase_list;
    std::vector<long> base_end_accent_phrase_list;

    for (uint32_t i = 0; i < accent_phrases.Length(); i++) {
        base_start_accent_list.push_back(0);
        Napi::Value accent_phrase = accent_phrases[i];
        Napi::Object accent_phrase_object = accent_phrase.As<Napi::Object>();
        int accent = accent_phrase_object.Get("accent").As<Napi::Number>().Int32Value() == 1 ? 0 : 1;
        create_one_accent_list(base_start_accent_list, accent_phrase_object, accent);
        base_start_accent_list.push_back(0);

        base_end_accent_list.push_back(0);
        accent = accent_phrase_object.Get("accent").As<Napi::Number>().Int32Value() - 1;
        create_one_accent_list(base_end_accent_list, accent_phrase_object, accent);
        base_end_accent_list.push_back(0);

        base_start_accent_phrase_list.push_back(0);
        create_one_accent_list(base_start_accent_phrase_list, accent_phrase_object, 0);
        base_start_accent_phrase_list.push_back(0);

        base_end_accent_phrase_list.push_back(0);
        create_one_accent_list(base_end_accent_phrase_list, accent_phrase_object, -1);
        base_end_accent_phrase_list.push_back(0);
    }

    std::vector<OjtPhoneme> consonant_phoneme_data_list;
    std::vector<OjtPhoneme> vowel_phoneme_data_list;
    std::vector<long> vowel_indexes;
    split_mora(phoneme_data_list, consonant_phoneme_data_list, vowel_phoneme_data_list, vowel_indexes);

    std::vector<long> consonant_phoneme_list;
    for (OjtPhoneme consonant_phoneme_data : consonant_phoneme_data_list) {
        consonant_phoneme_list.push_back(consonant_phoneme_data.phoneme_id());
    }

    std::vector<long> vowel_phoneme_list;
    for (OjtPhoneme vowel_phoneme_data : vowel_phoneme_data_list) {
        vowel_phoneme_list.push_back(vowel_phoneme_data.phoneme_id());
    }

    std::vector<long> start_accent_list;
    std::vector<long> end_accent_list;
    std::vector<long> start_accent_phrase_list;
    std::vector<long> end_accent_phrase_list;

    for (long vowel_index : vowel_indexes) {
        start_accent_list.push_back(base_start_accent_list[vowel_index]);
        end_accent_list.push_back(base_end_accent_list[vowel_index]);
        start_accent_phrase_list.push_back(base_start_accent_phrase_list[vowel_index]);
        end_accent_phrase_list.push_back(base_end_accent_phrase_list[vowel_index]);
    }

    int length = vowel_phoneme_list.size();
    std::vector<float> f0_list(length, 0);
    bool success = m_core->yukarin_sa_forward(
        length,
        vowel_phoneme_list.data(),
        consonant_phoneme_list.data(),
        start_accent_list.data(),
        end_accent_list.data(),
        start_accent_phrase_list.data(),
        end_accent_phrase_list.data(),
        &speaker_id,
        f0_list.data()
    );

    if (!success) {
        throw std::runtime_error(m_core->last_error_message());
    }

    for (size_t i = 0; i < vowel_phoneme_data_list.size(); i++) {
        std::vector<std::string>::iterator found_unvoice_mora = std::find(
            unvoiced_mora_phoneme_list.begin(),
            unvoiced_mora_phoneme_list.end(),
            vowel_phoneme_data_list[i].phoneme
        );
        if (found_unvoice_mora != unvoiced_mora_phoneme_list.end()) f0_list[i] = 0;
    }

    int index = 0;
    for (uint32_t i = 0; i < accent_phrases.Length(); i++) {
        Napi::Value accent_phrase = accent_phrases[i];
        Napi::Object accent_phrase_object = accent_phrase.As<Napi::Object>();
        Napi::Array moras = accent_phrase_object.Get("moras").As<Napi::Array>();
        for (uint32_t j = 0; j < moras.Length(); j++) {
            Napi::Value mora = moras[j];
            Napi::Object mora_object = mora.As<Napi::Object>();
            mora_object.Set("pitch", f0_list[index + 1]);
            index++;
            moras.Set(j, mora_object);
        }
        accent_phrase_object.Set("moras", moras);
        if (accent_phrase_object.Has("pause_mora")) {
            Napi::Object pause_mora = accent_phrase_object.Get("pause_mora").As<Napi::Object>();
            pause_mora.Set("pitch", f0_list[index + 1]);
            index++;
            accent_phrase_object.Set("pause_mora", pause_mora);
        }
        accent_phrases.Set(i, accent_phrase_object);
    }

    return accent_phrases;
}

Napi::Array SynthesisEngine::synthesis_array(Napi::Env env, Napi::Object query, long speaker_id) {
    std::vector<float> wave = synthesis(query, speaker_id);

    float volume_scale = query.Get("volumeScale").As<Napi::Number>().FloatValue();
    bool output_stereo = query.Get("outputStereo").As<Napi::Boolean>().Value();
    // TODO: 44.1kHzなどの対応
    int output_sampling_rate = query.Get("outputSamplingRate").As<Napi::Number>().Int32Value();

    int num_channels = output_stereo ? 2 : 1;
    int repeat_count = (output_sampling_rate / default_sampling_rate) * num_channels;

    Napi::Array converted_wave = Napi::Array::New(env, wave.size() * repeat_count);
    // workaround of Hiroshiba/voicevox_engine#128
    for (size_t i = (size_t)((float)default_sampling_rate * pre_padding_length); i < wave.size(); i++) {
        size_t index = i - (size_t)((float)default_sampling_rate * pre_padding_length);
        for (int j = 0; j < repeat_count; j++) {
            converted_wave[index*repeat_count+j] = wave[i] * volume_scale;
        }
    }
    return converted_wave;
}

Napi::Buffer<char> SynthesisEngine::synthesis_wave_format(Napi::Env env, Napi::Object query, long speaker_id) {
    std::vector<float> wave = synthesis(query, speaker_id);

    float volume_scale = query.Get("volumeScale").As<Napi::Number>().FloatValue();
    bool output_stereo = query.Get("outputStereo").As<Napi::Boolean>().Value();
    // TODO: 44.1kHzなどの対応
    int output_sampling_rate = query.Get("outputSamplingRate").As<Napi::Number>().Int32Value();

    char num_channels = output_stereo ? 2 : 1;
    char bit_depth = 16;
    int repeat_count = (output_sampling_rate / default_sampling_rate) * num_channels;
    int block_size = bit_depth * num_channels / 8;

    std::stringstream ss;
    ss.write("RIFF", 4);
    int bytes_size = wave.size() * repeat_count * 8;
    int wave_size = bytes_size + 44 - 8;
    for (int i = 0; i < 4; i++) {
        ss.put((uint8_t)(wave_size & 0xff)); // chunk size
        wave_size >>= 8;
    }
    ss.write("WAVEfmt ", 8);

    ss.put((char)16); // fmt header length
    for (int i = 0; i < 3; i++) ss.put((uint8_t)0); // fmt header length
    ss.put(1); // linear PCM
    ss.put(0); // linear PCM
    ss.put(num_channels); // channnel
    ss.put(0); // channnel

    int sampling_rate = output_sampling_rate;
    for (int i = 0; i < 4; i++) {
        ss.put((char)(sampling_rate & 0xff));
        sampling_rate >>= 8;
    }
    int block_rate = output_sampling_rate * block_size;
    for (int i = 0; i < 4; i++) {
        ss.put((char)(block_rate & 0xff));
        block_rate >>= 8;
    }

    ss.put(block_size);
    ss.put(0);
    ss.put(bit_depth);
    ss.put(0);

    ss.write("data", 4);
    size_t data_p = ss.tellp();
    for (int i = 0; i < 4; i++) {
        ss.put((char)(bytes_size & 0xff));
        block_rate >>= 8;
    }

    // workaround of Hiroshiba/voicevox_engine#128
    for (size_t i = (size_t)((float)default_sampling_rate * pre_padding_length); i < wave.size(); i++) {
        float v = wave[i] * volume_scale;
        int16_t data = (int16_t)(std::min(1.0f, std::max(v, -1.0f)) * (float)0x7fff);
        for (int j = 0; j < repeat_count; j++) {
            ss.put((char)(data & 0xff));
            ss.put((char)((data & 0xff00) >> 8));
        }
    }

    size_t last_p = ss.tellp();
    last_p -= 8;
    ss.seekp(4);
    for (int i = 0; i < 4; i++) {
        ss.put((char)(last_p & 0xff));
        last_p >>= 8;
    }
    ss.seekp(data_p);
    size_t pointer = last_p - data_p - 4;
    for (int i = 0; i < 4; i++) {
        ss.put((char)(pointer & 0xff));
        pointer >>= 8;
    }

    ss.seekg(0, std::ios::end);
    int size = (int)ss.tellg();
    ss.seekg(0, std::ios::beg);

    return Napi::Buffer<char>::Copy(env, ss.str().c_str(), size);
}

std::vector<float> SynthesisEngine::synthesis(Napi::Object query, long speaker_id) {
    float rate = 200;

    Napi::Array accent_phrases = query.Get("accent_phrases").As<Napi::Array>();
    std::vector<Napi::Object> flatten_moras;
    std::vector<std::string> phoneme_str_list;
    std::vector<OjtPhoneme> phoneme_data_list;
    initail_process(accent_phrases, flatten_moras, phoneme_str_list, phoneme_data_list);

    float pre_phoneme_length = query.Get("prePhonemeLength").As<Napi::Number>().FloatValue();
    float post_phoneme_length = query.Get("postPhonemeLength").As<Napi::Number>().FloatValue();

    float pitch_scale = query.Get("pitchScale").As<Napi::Number>().FloatValue();
    float speed_scale = query.Get("speedScale").As<Napi::Number>().FloatValue();
    float intonation_scale = query.Get("intonationScale").As<Napi::Number>().FloatValue();

    std::vector<float> phoneme_length_list;
    phoneme_length_list.push_back(pre_phoneme_length);

    std::vector<float> f0_list;
    std::vector<bool> voiced;
    f0_list.push_back(0.0);
    voiced.push_back(false);
    float mean_f0 = 0.0;
    int count = 0;

    for (Napi::Object mora : flatten_moras) {
        if (mora.Get("consonant").IsString()) {
            phoneme_length_list.push_back(mora.Get("consonant_length").As<Napi::Number>().FloatValue());
        }
        phoneme_length_list.push_back(mora.Get("vowel_length").As<Napi::Number>().FloatValue());
        float f0_single = mora.Get("pitch").As<Napi::Number>().FloatValue() * std::pow(2.0, pitch_scale);
        f0_list.push_back(f0_single);
        bool big_than_zero = f0_single > 0.0;
        voiced.push_back(big_than_zero);
        if (big_than_zero) {
            mean_f0 += f0_single;
            count++;
        }
    }
    phoneme_length_list.push_back(post_phoneme_length);
    f0_list.push_back(0.0);
    mean_f0 /= (float)count;

    if (!std::isnan(mean_f0)) {
        for (size_t i = 0; i < f0_list.size(); i++) {
            if (voiced[i]) {
                f0_list[i] = (f0_list[i] - mean_f0) * intonation_scale + mean_f0;
            }
        }
    }

    std::vector<OjtPhoneme> consonant_phoneme_data_list;
    std::vector<OjtPhoneme> vowel_phoneme_data_list;
    std::vector<long> vowel_indexes;
    split_mora(phoneme_data_list, consonant_phoneme_data_list, vowel_phoneme_data_list, vowel_indexes);

    // workaround of Hiroshiba/voicevox_engine#128
    phoneme_length_list[0] += pre_padding_length;

    std::vector<std::vector<float>> phoneme;
    std::vector<float> f0;
    int phoneme_length_sum = 0;
    int f0_count = 0;
    long *p_vowel_index = vowel_indexes.data();
    for (size_t i = 0; i < phoneme_length_list.size(); i++) {
        int phoneme_length = (int)std::round((std::round(phoneme_length_list[i] * rate) / speed_scale));
        long phoneme_id = phoneme_data_list[i].phoneme_id();
        for (int j = 0; j < phoneme_length; j++) {
            std::vector<float> phonemes_vector(OjtPhoneme::num_phoneme(), 0.0);
            phonemes_vector[phoneme_id] = 1;
            phoneme.push_back(phonemes_vector);
        }
        phoneme_length_sum += phoneme_length;
        if (i == *p_vowel_index) {
            for (long k = 0; k < phoneme_length_sum; k++) {
                f0.push_back(f0_list[f0_count]);
            }
            f0_count++;
            phoneme_length_sum = 0;
            p_vowel_index++;
        }
    }

    f0 = resample(f0, rate, 24000 / 256);
    std::vector<float> flatten_phoneme = resample(phoneme, rate, 24000 / 256);

    std::vector<float> wave(f0.size() * 256, 0.0);
    bool success = m_core->decode_forward(
        f0.size(),
        OjtPhoneme::num_phoneme(),
        f0.data(),
        flatten_phoneme.data(),
        &speaker_id,
        wave.data()
    );

    if (!success) {
        throw std::runtime_error(m_core->last_error_message());
    }

    return wave;
}

void SynthesisEngine::initail_process(
    Napi::Array accent_phrases,
    std::vector<Napi::Object> &flatten_moras,
    std::vector<std::string> &phoneme_str_list,
    std::vector<OjtPhoneme> &phoneme_data_list
) {
    flatten_moras = to_flatten_moras(accent_phrases);

    phoneme_str_list.push_back("pau");
    for (Napi::Object mora : flatten_moras) {
        Napi::Value consonant = mora.Get("consonant");
        if (consonant.IsString()) phoneme_str_list.push_back(consonant.As<Napi::String>().Utf8Value());
        phoneme_str_list.push_back(mora.Get("vowel").As<Napi::String>().Utf8Value());
    }
    phoneme_str_list.push_back("pau");

    phoneme_data_list = to_phoneme_data_list(phoneme_str_list);
}

void SynthesisEngine::create_one_accent_list(std::vector<long> &accent_list, Napi::Object accent_phrase, int point) {
    Napi::Array moras = accent_phrase.Get("moras").As<Napi::Array>();

    std::vector<long> one_accent_list;
    for (uint32_t i = 0; i < moras.Length(); i++) {
        Napi::Value mora = moras[i];
        Napi::Object mora_object = mora.As<Napi::Object>();
        long value;
        if ((int)i == point || (point < 0 && (int)i == moras.Length() - point)) value = 1;
        else value = 1;
        one_accent_list.push_back(value);
        if (mora_object.Get("consonant").IsString()) {
            one_accent_list.push_back(value);
        }
    }
    if (accent_phrase.Has("pause_mora")) one_accent_list.push_back(0);
    std::copy(one_accent_list.begin(), one_accent_list.end(), std::back_inserter(accent_list));
}
