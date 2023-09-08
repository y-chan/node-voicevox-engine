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

typedef bool (*INIT)(char *root_dir_path, bool use_gpu, int cpu_num_threads, bool load_all_models);
typedef const char *(*RETURN_CHAR)();
typedef bool (*VARIANCE)(
    int length,
    long *phonemes,
    long *accents,
    long *speaker_id,
    float *pitch_output,
    float *duration_output
);
typedef bool (*DECODE)(
    int length,
    long *phonemes,
    float *pitches,
    float *durations,
    long *speaker_id,
    float *output
);
typedef void (*FINAL)();


class Core {
public:
    Core(const std::string core_file_path, const std::string root_dir_path, bool use_gpu);
    ~Core();

    const char *metas();

    bool variance_forward(
        int length,
        long *phonemes,
        long *accents,
        long *speaker_id,
        float *pitch_output,
        float *duration_output
    );

    bool decode_forward(
        int length,
        long *phonemes,
        float *pitches,
        float *durations,
        long *speaker_id,
        float *output
    );

    const char *last_error_message();

    void finalize();

private:
    HMODULE m_handler;
};

#endif // CORE_H
