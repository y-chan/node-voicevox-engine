#ifndef CORE_H
#define CORE_H

#include <napi.h>
#include <string>

#if defined(_WIN32) || defined(_WIN64)
#define NOMINMAX
#include <windows.h>
#else
#include <dlfcn.h>
#define LoadLibrary(path) dlopen(path, RTLD_LAZY)
#define GetProcAddress(handler, func_name) dlsym(handler, func_name);
#define FreeLibrary(handler) dlclose(handler);
typedef void* HMODULE;
typedef void* FARPROC;
#endif

typedef bool (*INIT)(bool use_gpu, int cpu_num_threads, bool load_all_models);
typedef const char *(*RETURN_CHAR)();
typedef bool (*YUKARIN_S)(int length, long *phoneme_list, long *speaker_id, float *output);
typedef bool (*YUKARIN_SA)(
    int length,
    long *vowel_phoneme_list,
    long *consonant_phoneme_list,
    long *start_accent_list,
    long *end_accent_list,
    long *start_accent_phrase_list,
    long *end_accent_phrase_list,
    long *speaker_id,
    float *output
);
typedef bool (*DECODE)(
    int length,
    int phoneme_size,
    float *f0,
    float *phoneme,
    long *speaker_id,
    float *output
);


class Core {
public:
    Core(const std::string core_file_path, bool use_gpu);
    ~Core();

    const char *metas();

    bool yukarin_s_forward(int length, long *phoneme_list, long *speaker_id, float *output);

    bool yukarin_sa_forward(
        int length,
        long *vowel_phoneme_list,
        long *consonant_phoneme_list,
        long *start_accent_list,
        long *end_accent_list,
        long *start_accent_phrase_list,
        long *end_accent_phrase_list,
        long *speaker_id,
        float *output
    );

    bool decode_forward(
        int length,
        int phoneme_size,
        float *f0,
        float *phoneme,
        long *speaker_id,
        float *output
    );

    const char *last_error_message();

private:
    HMODULE m_handler;
};

#endif // CORE_H
