#ifndef ACOUSTIC_FEATURE_EXTRACTOR_H
#define ACOUSTIC_FEATURE_EXTRACTOR_H

#include <map>
#include <random>
#include <string>
#include <vector>

template <typename T>
inline std::vector<T> resample(std::vector<T> base_array, float rate, float sampling_rate, int index = 0, int stride = 1) {
    int length = (int)(base_array.size() / rate * sampling_rate);

    std::vector<T> new_array;
    float calc_rate = rate / sampling_rate;
    std::random_device seed_gen;
    std::mt19937 engine(seed_gen());
    std::uniform_real_distribution<float> dist(0.0, 1.0);
    float rand_value = dist(engine);
    for (int i = 0; i < base_array.size(); i++) {
        int j = (int)((rand_value + (float)(index + i)) * calc_rate);
        for (int k = 0; k < stride; k++) new_array[i * stride + k] = base_array[j * stride + k];
    }
    return new_array;
}

// TODO: 現状のHiroshiba/voiceovox_engineではOjtしか使われていないので、一旦これのみ実装した
class OjtPhoneme {
public:
    std::string phoneme;
    float start;
    float end;

    static const std::map<std::string, int> phoneme_map() {
        std::map<std::string, int> phoneme_map = {
            {"pau", 0}, {"A", 1},   {"E", 2},   {"I", 3},   {"N", 4},   {"O", 5},
            {"U", 6},   {"a", 7},   {"b", 8},   {"by", 9},  {"ch", 10}, {"cl", 11},
            {"d", 12},  {"dy", 13}, {"e", 14},  {"f", 15},  {"g", 16},  {"gw", 17},
            {"gy", 18}, {"h", 19},  {"hy", 20}, {"i", 21},  {"j", 22},  {"k", 23},
            {"kw", 24}, {"ky", 25}, {"m", 26},  {"my", 27}, {"n", 28},  {"ny", 29},
            {"o", 30},  {"p", 31},  {"py", 32}, {"r", 33},  {"ry", 34}, {"s", 35},
            {"sh", 36}, {"t", 37},  {"ts", 38}, {"ty", 39}, {"u", 40},  {"v", 41},
            {"w", 42},  {"y", 43},  {"z", 44}
        };
        return phoneme_map;
    }
    static const int num_phoneme() { return phoneme_map().size(); }
    static const std::string space_phoneme() { return std::string("pau"); }

    OjtPhoneme(std::string c_phoneme, float c_start, float c_end) {
        phoneme = c_phoneme;
        start = c_start;
        end = c_end;
    }

    long phoneme_id();
    static std::vector<OjtPhoneme> convert(std::vector<OjtPhoneme> phonemes);
};

#endif // ACOUSTIC_FEATURE_EXTRACTOR_H
