#include "kana_parser.h"

static const std::map<std::string, Napi::Object> text2mora_with_unvoice() {
    std::map<std::string, Napi::Object> text2mora_with_unvoice;
    const std::string* mora_list = mora_list_minimum.data();
    int count = 0;
    while (count < mora_list_minimum.size()) {
        std::string text = *mora_list;
        std::string consonant = *(mora_list + 1);
        std::string vowel = *(mora_list + 2);

        Napi::Object mora;
        mora.Set("text", text);
        if (consonant.size() != 0) {
            mora.Set("consonant", consonant);
            mora.Set("consonant_length", 0.0f);
        }
        mora.Set("vowel", vowel);
        mora.Set("vowel_length", 0.0f);
        mora.Set("pitch", 0.0f);

        text2mora_with_unvoice[text] = mora;

        if (
            vowel == "a" ||
            vowel == "i" ||
            vowel == "u" ||
            vowel == "e" ||
            vowel == "o"
        ) {
            Napi::Object unvoice_mora;
            unvoice_mora.Set("text", text);
            if (consonant.size() != 0) {
                unvoice_mora.Set("consonant", consonant);
                unvoice_mora.Set("consonant_length", 0.0f);
            }
            std::string upper_vowel = vowel;
            std::transform(upper_vowel.begin(), upper_vowel.end(), upper_vowel.begin(), ::toupper);
            unvoice_mora.Set("vowel", upper_vowel);
            unvoice_mora.Set("vowel_length", 0.0f);
            unvoice_mora.Set("pitch", 0.0f);

            text2mora_with_unvoice[UNVOICE_SYMBOL + text] = unvoice_mora;
        }

        mora_list += 3;
        count += 3;
    }

    return text2mora_with_unvoice;
}

std::string extract_one_character(const std::string& text, size_t pos, size_t& size) {
    // UTF-8の文字は可変長なので、leadの値で長さを判別する
    unsigned char lead = text[pos];

    if (lead < 0x80) {
        size = 1;
    } else if (lead < 0xE0) {
        size = 2;
    } else if (lead < 0xF0) {
        size = 3;
    } else {
        size = 4;
    }

    return text.substr(pos, size);
}

Napi::Object text_to_accent_phrase(Napi::Env env, std::string phrase) {
    int* accent_index = nullptr;

    Napi::Array moras = Napi::Array::New(env);
    int count = 0;

    int base_index = 0;
    std::string stack = "";
    std::string *matched_text = nullptr;

    const std::map<std::string, Napi::Object> text2mora = text2mora_with_unvoice();

    int outer_loop = 0;
    while (base_index < phrase.size()) {
        outer_loop++;
        size_t char_size;
        std::string letter = extract_one_character(phrase, base_index, char_size);
        if (letter == ACCENT_SYMBOL) {
            if (moras.Length() == 0) {
                throw std::runtime_error("accent cannot be set at beginning of accent phrase: " + phrase);
            }
            if (accent_index == nullptr) {
                throw std::runtime_error("second accent cannot be set at an accent phrase: " + phrase);
            }

            *accent_index = moras.Length();
            base_index++;
            continue;
        }
        size_t watch_char_size;
        for (size_t watch_index = base_index; watch_index < phrase.size(); watch_index += watch_char_size) {
            std::string watch_letter = extract_one_character(phrase, watch_index, watch_char_size);
            if (std::string(&phrase[base_index]) == ACCENT_SYMBOL) break;
            stack += phrase[watch_index];
            if (text2mora.find(stack) != text2mora.end()) {
                matched_text = &stack;
            }
        }
        if (matched_text == nullptr) {
            throw std::runtime_error("unknown text in accent phrase: " + stack);
        } else {
            moras[count] = text2mora.at(*matched_text);
            count++;
            base_index += matched_text->size();
            stack = "";
            matched_text = nullptr;
        }
        if (outer_loop > LOOP_LIMIT) throw std::runtime_error("detect infinity loop!");
    }
    if (accent_index == nullptr) throw std::runtime_error("accent not found in accent phrase: " + phrase);

    Napi::Object accent_phrase = Napi::Object::New(env);
    accent_phrase.Set("moras", moras);
    accent_phrase.Set("accent", *accent_index);
    return accent_phrase;
}

Napi::Array parse_kana(Napi::Env env, std::string text) {
    Napi::Array parsed_results = Napi::Array::New(env);

    std::string phrase = "";
    int count = 0;
    size_t char_size;
    for (size_t pos = 0; pos <= text.size(); pos += char_size) {
        std::string letter;
        if (pos != text.size()) {
            letter = extract_one_character(text, pos, char_size);
        }
        if (pos == text.size() || letter == PAUSE_DELIMITER || letter == NOPAUSE_DELIMITER) {
            if (phrase.size() == 0) {
                throw std::runtime_error(
                    "accent phrase at position of " + std::to_string(parsed_results.Length() + 1) +" is empty"
                );
            }
            bool is_interrogative = phrase.find(WIDE_INTERROGATION_MARK) != std::string::npos;
            if (is_interrogative) {
                if (phrase.find(WIDE_INTERROGATION_MARK) != phrase.length() - 1) {
                    throw std::runtime_error(
                        "interrogative mark cannot be set at not end of accent phrase: " + phrase
                    );
                }
                phrase = phrase.replace(phrase.length() - 1, 1, "");
            }
            Napi::Object accent_phrase = text_to_accent_phrase(env, phrase);
            if (pos < text.size() && letter == PAUSE_DELIMITER) {
                Napi::Object pause_mora = Napi::Object::New(env);
                pause_mora.Set("text", PAUSE_DELIMITER);
                pause_mora.Set("vowel", "pau");
                pause_mora.Set("vowel_length", 0.0f);
                pause_mora.Set("pitch", 0.0f);

                accent_phrase.Set("pause_mora", pause_mora);
            }
            accent_phrase.Set("is_interrogative", is_interrogative);
            parsed_results[count] = accent_phrase;
            count++;
            phrase = "";
        } else {
            phrase += letter;
        }
    }
    return parsed_results;
}

Napi::String create_kana(Napi::Env env, Napi::Array accent_phrases) {
    std::string text = "";
    for (int i = 0; i < accent_phrases.Length(); i++) {
        Napi::Value phrase_val = accent_phrases[i];
        Napi::Object phrase = phrase_val.As<Napi::Object>();
        Napi::Array moras = phrase.Get("moras").As<Napi::Array>();
        for (int j = 0; j < moras.Length(); j++) {
            Napi::Value mora_val = moras[j];
            Napi::Object mora = mora_val.As<Napi::Object>();
            std::string vowel = mora.Get("vowel").As<Napi::String>().Utf8Value();
            if (
                vowel == "A" ||
                vowel == "I" ||
                vowel == "U" ||
                vowel == "E" ||
                vowel == "O"
            ) {
                text += UNVOICE_SYMBOL;
            }
            text += mora.Get("text").As<Napi::String>().Utf8Value();

            if (j + 1 == phrase.Get("accent").As<Napi::Number>().Int32Value()) {
                text += ACCENT_SYMBOL;
            }
        }

        if (phrase.Get("is_interrogative").As<Napi::Boolean>().Value()) {
            text += WIDE_INTERROGATION_MARK;
        }

        if (i < accent_phrases.Length()) {
            if (phrase.Has("accent")) text += PAUSE_DELIMITER;
            else text += NOPAUSE_DELIMITER;
        }
    }
    return Napi::String::New(env, text);
}
