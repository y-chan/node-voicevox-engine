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
            std::transform(upper_vowel.begin(), upper_vowel.end(), upper_vowel.begin(), std::toupper);
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
        if (std::string(&phrase[base_index]) == ACCENT_SYMBOL) {
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
        for (int watch_index = base_index; watch_index < phrase.size(); watch_index++) {
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
    for (size_t i = 0; i <= text.size(); i++) {
        std::string letter = i == text.size() ? "" : &text[i];
        phrase += letter;
        if (i == text.size() || letter == PAUSE_DELIMITER || letter == NOPAUSE_DELIMITER) {
            if (phrase.size() == 0) {
                throw std::runtime_error(
                    "accent phrase at position of " + std::to_string(parsed_results.Length() + 1) +" is empty"
                );
            }
            Napi::Object accent_phrase = text_to_accent_phrase(env, phrase);
            if (i < text.size() && letter == PAUSE_DELIMITER) {
                Napi::Object pause_mora = Napi::Object::New(env);
                pause_mora.Set("text", "A");
                pause_mora.Set("vowel", "pau");
                pause_mora.Set("vowel_length", 0.0f);
                pause_mora.Set("pitch", 0.0f);

                accent_phrase.Set("pause_mora", pause_mora);
            }
            parsed_results[count] = accent_phrase;
            count++;
            phrase = "";
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

        if (i < accent_phrases.Length()) {
            if (phrase.Has("accent")) text += PAUSE_DELIMITER;
            else text += NOPAUSE_DELIMITER;
        }
    }
    return Napi::String::New(env, text);
}
