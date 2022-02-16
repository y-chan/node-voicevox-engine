#include <regex>

#include "full_context_label.h"

Phoneme *Phoneme::from_label(const std::string label) {
    std::regex re(
        "^(.+?)\\^(.+?)\\-(.+?)\\+(.+?)\\=(.+?)"
        "/A\\:(.+?)\\+(.+?)\\+(.+?)/B\\:(.+?)\\-(.+?)\\_(.+?)"
        "/C\\:(.+?)\\_(.+?)\\+(.+?)/D\\:(.+?)\\+(.+?)\\_(.+?)"
        "/E\\:(.+?)\\_(.+?)\\!(.+?)\\_(.+?)\\-(.+?)"
        "/F\\:(.+?)\\_(.+?)\\#(.+?)\\_(.+?)\\@(.+?)\\_(.+?)\\|(.+?)\\_(.+?)"
        "/G\\:(.+?)\\_(.+?)\\%(.+?)\\_(.+?)\\_(.+?)/H\\:(.+?)\\_(.+?)"
        "/I\\:(.+?)\\-(.+?)\\@(.+?)\\+(.+?)\\&(.+?)\\-(.+?)\\|(.+?)\\+(.+?)"
        "/J\\:(.+?)\\_(.+?)/K\\:(.+?)\\+(.+?)\\-(.+?)$"
    );
    std::smatch match;
    if (std::regex_match(label, match, re)) {
        std::map<std::string, std::string> contexts;
        std::vector<std::string> keys = {
            "p1", "p2", "p3", "p4", "p5", "a1", "a2", "a3", "b1", "b2",
            "b3", "c1", "c2", "c3", "d1", "d2", "d3", "e1", "e2", "e3",
            "e4", "e5", "f1", "f2", "f3", "f4", "f5", "f6", "f7", "f8",
            "g1", "g2", "g3", "g4", "g5", "h1", "h2", "i1", "i2", "i3",
            "i4", "i5", "i6", "i7", "i8", "j1", "j2", "k1", "k2", "k3"
        };
        for (int i = 1; i < match.size(); i++) contexts[keys[i-1]] = match[i];
        return new Phoneme(contexts, label);
    } else {
        throw std::runtime_error("label is broken");
    }
}

const std::string Phoneme::phoneme() {
    return contexts.at("p3");
}

const bool Phoneme::is_pause() {
    return contexts.at("f1") == "xx";
}

void Mora::set_context(std::string key, std::string value) {
    vowel->contexts[key] = value;
    if (consonant != nullptr) consonant->contexts[key] = value;
}

const std::vector<Phoneme *> Mora::phonemes() {
    std::vector<Phoneme *> phonemes;
    if (consonant != nullptr) {
        phonemes = { consonant, vowel };
    } else {
        phonemes = { vowel };
    }
    return phonemes;
}

const std::vector<std::string> Mora::labels() {
    std::vector<std::string> labels;
    for (Phoneme *phoneme : phonemes()) {
        labels.push_back(phoneme->label);
    }
    return labels;
}

AccentPhrase *AccentPhrase::from_phonemes(std::vector<Phoneme *> phonemes) {
    std::vector<Mora *> moras;
    std::vector<Phoneme *> mora_phonemes;

    for (size_t i = 0; i < phonemes.size(); i++) {
        // workaround for Hihosiba/voicevox_engine#57
        if (phonemes[i]->contexts.at("a2") == "49") break;

        mora_phonemes.push_back(phonemes[i]);
        if (
            i + 1 == phonemes.size() ||
            phonemes[i]->contexts.at("a2") != phonemes[i + 1]->contexts.at("a2")
        ) {
            Mora *mora;
            if (mora_phonemes.size() == 1) {
                mora = new Mora(mora_phonemes[0]);
            } else if (mora_phonemes.size() == 2) {
                mora = new Mora(mora_phonemes[0], mora_phonemes[1]);
            } else {
                throw std::runtime_error("too long mora");
            }
            moras.push_back(mora);
            mora_phonemes.clear();
        }
    }

    int accent = std::stoi(moras[0]->vowel->contexts.at("f2"));
    bool is_interrogative = moras[moras.size() - 1]->vowel->contexts.at("f3") == "1";
    // workaround for Hihosiba / voicevox_engine#55
    if (accent > moras.size()) accent = moras.size();
    return new AccentPhrase(moras, accent, is_interrogative);
 }

void AccentPhrase::set_context(std::string key, std::string value) {
    for (Mora *mora : moras) mora->set_context(key, value);
}

const std::vector<Phoneme *> AccentPhrase::phonemes() {
    std::vector<Phoneme *> phonemes;
    for (Mora *mora : moras) {
        std::vector<Phoneme *> mora_phonemes = mora->phonemes();
        std::copy(
            mora_phonemes.begin(),
            mora_phonemes.end(),
            std::back_inserter(phonemes)
        );
    }
    return phonemes;
}

const std::vector<std::string> AccentPhrase::labels() {
    std::vector<std::string> labels;
    for (Phoneme *phoneme : phonemes()) {
        labels.push_back(phoneme->label);
    }
    return labels;
}

AccentPhrase *AccentPhrase::merge(AccentPhrase *accent_phrase) {
    std::vector<Mora *> moras;
    std::copy(
        this->moras.begin(),
        this->moras.end(),
        std::back_inserter(moras)
    );
    std::copy(
        accent_phrase->moras.begin(),
        accent_phrase->moras.end(),
        std::back_inserter(moras)
    );
    return new AccentPhrase(moras, this->accent, accent_phrase->is_interrogative);
}

BreathGroup *BreathGroup::from_phonemes(std::vector<Phoneme *> phonemes) {
    std::vector<AccentPhrase *> accent_phrases;
    std::vector<Phoneme *> accent_phonemes;

    for (size_t i = 0; i < phonemes.size(); i++) {
        accent_phonemes.push_back(phonemes[i]);

        if (
            i + 1 == phonemes.size() ||
            phonemes[i]->contexts.at("i3") != phonemes[i + 1]->contexts.at("i3") ||
            phonemes[i]->contexts.at("f5") != phonemes[i + 1]->contexts.at("f5")
        ) {
            accent_phrases.push_back(AccentPhrase::from_phonemes(accent_phonemes));
            accent_phonemes.clear();
        }
    }

    return new BreathGroup(accent_phrases);
};

void BreathGroup::set_context(std::string key, std::string value) {
    for (AccentPhrase *accent_phrase : accent_phrases)
        accent_phrase->set_context(key, value);
}

const std::vector<Phoneme *> BreathGroup::phonemes() {
    std::vector<Phoneme *> phonemes;
    for (AccentPhrase *accent_phrase : accent_phrases) {
        std::vector<Phoneme *> accent_phrase_phonemes = accent_phrase->phonemes();
        std::copy(
            accent_phrase_phonemes.begin(),
            accent_phrase_phonemes.end(),
            std::back_inserter(phonemes)
        );
    }
    return phonemes;
}

const std::vector<std::string> BreathGroup::labels() {
    std::vector<std::string> labels;
    for (Phoneme *phoneme : phonemes()) {
        labels.push_back(phoneme->label);
    }
    return labels;
}

Utterance Utterance::from_phonemes(std::vector<Phoneme *> phonemes) {
    std::vector<BreathGroup *> breath_groups;
    std::vector<Phoneme *> group_phonemes;
    std::vector<Phoneme *> pauses;

    for (Phoneme *phoneme : phonemes) {
        if (!phoneme->is_pause()) {
            group_phonemes.push_back(phoneme);
        } else {
            pauses.push_back(phoneme);

            if (group_phonemes.size() > 0) {
                breath_groups.push_back(BreathGroup::from_phonemes(group_phonemes));
                group_phonemes.clear();
            }
        }
    }
    return Utterance(breath_groups, pauses);
}

void Utterance::set_context(std::string key, std::string value) {
    for (BreathGroup *breath_group : breath_groups) breath_group->set_context(key, value);
}

const std::vector<Phoneme *> Utterance::phonemes() {
    std::vector<AccentPhrase *> accent_phrases;
    for (BreathGroup *breath_group : breath_groups) {
        std::vector<AccentPhrase *> b_accent_phrases = breath_group->accent_phrases;
        std::copy(
            b_accent_phrases.begin(),
            b_accent_phrases.end(),
            std::back_inserter(accent_phrases)
        );
    }

    std::vector<Phoneme *> phonemes;
    for (size_t i = 0; i < pauses.size(); i++) {
        // if (pauses[i])
        phonemes.push_back(pauses[i]);
        if (i < pauses.size() - 1) {
            std::copy(
                breath_groups[i]->phonemes().begin(),
                breath_groups[i]->phonemes().end(),
                std::back_inserter(phonemes)
            );
        }
    }
    return phonemes;
}

const std::vector<std::string> Utterance::labels() {
    std::vector<std::string> labels;
    for (Phoneme *phoneme : phonemes()) {
        labels.push_back(phoneme->label);
    }
    return labels;
}

Utterance extract_full_context_label(OpenJTalk *openjtalk, std::string text) {
    std::vector<std::string> labels = openjtalk->extract_fullcontext(text);
    std::vector<Phoneme *> phonemes;
    for (std::string label : labels) phonemes.push_back(Phoneme::from_label(label));
    return Utterance::from_phonemes(phonemes);
}
