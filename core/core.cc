#include "core.h"

Core::Core(const std::string core_file_path, const std::string root_dir_path, bool use_gpu)
{
    HMODULE handler = LoadLibrary(core_file_path.c_str());
    if (handler == nullptr) {
        throw std::runtime_error("failed to load core library");
    }
	INIT initialize = (INIT)GetProcAddress(handler, "initialize");
	FARPROC metas = GetProcAddress(handler, "metas");
	FARPROC variance_forward = GetProcAddress(handler, "variance_forward");
	FARPROC decode_forward = GetProcAddress(handler, "decode_forward");
	FARPROC last_error_message = GetProcAddress(handler, "last_error_message");
    FARPROC finalize = GetProcAddress(handler, "finalize");
	if (
		initialize == nullptr ||
		metas == nullptr ||
		variance_forward == nullptr ||
		decode_forward == nullptr ||
		last_error_message == nullptr ||
        finalize == nullptr
	) {
		throw std::runtime_error("to load library is succeeded, but can't found needed functions");
	}
	m_handler = handler;
    // TODO: make cpu_num_threads changeable
    if (!initialize((char *)root_dir_path.c_str(), use_gpu, 0, true)) {
        throw std::runtime_error("failed to initialize core library");
    }
}

Core::~Core()
{
	FreeLibrary(m_handler);
}

const char *Core::metas()
{
	RETURN_CHAR metas = (RETURN_CHAR)GetProcAddress(m_handler, "metas");
	return metas();
}

bool Core::variance_forward(
    int length,
    long *phonemes,
    long *accents,
    long *speaker_id,
    float *pitch_output,
    float *duration_output
) {
	VARIANCE variance = (VARIANCE)GetProcAddress(m_handler, "variance_forward");
	return variance(
        length,
        phonemes,
        accents,
        speaker_id,
        pitch_output,
        duration_output
    );
}

bool Core::decode_forward(
    int length,
    long *phonemes,
    float *pitches,
    float *durations,
    long *speaker_id,
    float *output
)
{
    DECODE decode = (DECODE)GetProcAddress(m_handler, "decode_forward");
    return decode(
        length,
        phonemes,
        pitches,
        durations,
        speaker_id,
        output
    );
}

const char *Core::last_error_message()
{
	RETURN_CHAR last_error_message = (RETURN_CHAR)GetProcAddress(m_handler, "last_error_message");
	return last_error_message();
}

void Core::finalize()
{
    FINAL finalize = (FINAL)GetProcAddress(m_handler, "finalize");
    finalize();
}
