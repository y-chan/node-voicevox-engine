#include "acoustic_feature_extractor.h"

long OjtPhoneme::phoneme_id() {
    if (phoneme == "") return (long)-1;
    return (long)phoneme_map().at(phoneme);
}

std::vector<OjtPhoneme> OjtPhoneme::convert(std::vector<OjtPhoneme> phonemes) {
    if (phonemes[0].phoneme.find("sil") != std::string::npos) {
        phonemes[0].phoneme = OjtPhoneme::space_phoneme();
    }
    if (phonemes.back().phoneme.find("sil") != std::string::npos) {
        phonemes.back().phoneme = OjtPhoneme::space_phoneme();
    }
    return phonemes;
}
