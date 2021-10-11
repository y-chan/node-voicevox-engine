#ifndef FULL_CONTEXT_LABEL_H
#define FULL_CONTEXT_LABEL_H

#include <map>
#include <string>
#include <vector>

#include "openjtalk.h"

class Phoneme {
public:
    std::map<std::string, std::string> contexts;
    std::string label;

    Phoneme(const std::map<std::string, std::string> contexts, const std::string label) {
        this->contexts = contexts;
        this->label = label;
    }

    static Phoneme *from_label(const std::string label);

    const std::string phoneme();
    const bool is_pause();
};

class Mora {
public:
    Phoneme *consonant = nullptr;
    Phoneme *vowel;

    Mora(Phoneme *vowel) {
        this->vowel = vowel;
    }

    Mora(Phoneme *consonant, Phoneme *vowel) {
        this->consonant = consonant;
        this->vowel = vowel;
    }

    void set_context(std::string key, std::string value);
    const std::vector<Phoneme *> phonemes();
    const std::vector<std::string> labels();
};


class AccentPhrase {
public:
    std::vector<Mora *> moras;
    int accent;

    AccentPhrase(std::vector<Mora *> moras, int accent) {
        this->moras = moras;
        this->accent = accent;
    }

    static AccentPhrase *from_phonemes(std::vector<Phoneme *> phonemes);
    void set_context(std::string key, std::string value);
    const std::vector<Phoneme *> phonemes();
    const std::vector<std::string> labels();
    AccentPhrase *merge(AccentPhrase *accent_phrase);
};

class BreathGroup {
public:
    std::vector<AccentPhrase *> accent_phrases;

    BreathGroup(std::vector<AccentPhrase *> accent_phrases) {
        this->accent_phrases = accent_phrases;
    }

    static BreathGroup *from_phonemes(std::vector<Phoneme *> phonemes);
    void set_context(std::string key, std::string value);
    const std::vector<Phoneme *> phonemes();
    const std::vector<std::string> labels();
};

class Utterance {
public:
    std::vector<BreathGroup *> breath_groups;
    std::vector<Phoneme *> pauses;

    Utterance(std::vector<BreathGroup *> breath_groups, std::vector<Phoneme *> pauses) {
        this->breath_groups = breath_groups;
        this->pauses = pauses;
    }

    static Utterance from_phonemes(std::vector<Phoneme *> phonemes);
    void set_context(std::string key, std::string value);
    const std::vector<Phoneme *> phonemes();
    const std::vector<std::string> labels();
};

Utterance extract_full_context_label(OpenJTalk *openjtalk, std::string text);

#endif // FULL_CONTEXT_LABEL_H
