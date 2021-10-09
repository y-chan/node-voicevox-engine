#include "acoustic_feature_extractor.h"

template <typename T>
std::vector<T> SamplingData<T>::resample(float sampling_rate, int index, int stride) {
	if (length == nullptr) {
		int len = (int)(base_array.size() / rate * sampling_rate);
		length = &len;
	}

	std::vector<T> new_array;
	float calc_rate = rate / sampling_rate;
	float rand_value = rand();
	for (int i = 0; i < base_array.size(); i++) {
		int j = (int)((rand_value + (float)(index + i)) * calc_rate);
		for (int k = 0; k < stride; k++) new_array[i * stride + k] = wave[j * stride + k];
	}
	return new_array;
}

long OjtPhoneme::phoneme_id() {
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
