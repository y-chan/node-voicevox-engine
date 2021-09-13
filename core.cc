#include "core.h"

Core::Core(const std::string library_file_path, const std::string root_dir_path, bool use_gpu)
{
    HMODULE handler = LoadLibrary(library_file_path.c_str());
    if (handler == nullptr) {
        throw std::runtime_error("コアライブラリの読み込みに失敗しました。");
    }
	INIT initialize = (INIT)GetProcAddress(handler, "initialize");
	FARPROC metas = GetProcAddress(handler, "metas");
	FARPROC yukarin_s_forward = GetProcAddress(handler, "yukarin_s_forward");
	FARPROC yukarin_sa_forward = GetProcAddress(handler, "yukarin_sa_forward");
	FARPROC decode_forward = GetProcAddress(handler, "decode_forward");
	if (
		initialize == nullptr ||
		metas == nullptr ||
		yukarin_s_forward == nullptr ||
		yukarin_sa_forward == nullptr ||
		decode_forward ==nullptr
	) {
		throw std::runtime_error("ライブラリの読み込みには成功しましたが、必要な関数が見つかりません。読み込むライブラリを間違えていませんか？");
	}
	m_handler = handler;
    initialize((char*)root_dir_path.c_str(), use_gpu);
}

Core::~Core()
{
	FreeLibrary(m_handler);
}

const char *Core::metas()
{
	METAS metas = (METAS)GetProcAddress(m_handler, "metas");
	return metas();
}

bool Core::yukarin_s_forward(int length, long *phoneme_list, long *speaker_id, float *output)
{
	YUKARIN_S yukarin = (YUKARIN_S)GetProcAddress(m_handler, "yukarin_s_forward");
	return yukarin(length, phoneme_list, speaker_id, output);
}

bool Core::yukarin_sa_forward(
    int length,
    long* vowel_phoneme_list,
    long* consonant_phoneme_list,
    long* start_accent_list,
    long* end_accent_list,
    long* start_accent_phrase_list,
    long* end_accent_phrase_list,
    long* speaker_id,
    float* output
)
{
	YUKARIN_SA yukarin = (YUKARIN_SA)GetProcAddress(m_handler, "yukarin_sa_forward");
	return yukarin(
        length,
        vowel_phoneme_list,
        consonant_phoneme_list,
        start_accent_list,
        end_accent_list,
        start_accent_phrase_list,
        end_accent_phrase_list,
        speaker_id,
        output
    );
}

bool Core::decode_forward(
    int length,
    int phoneme_size,
    float *f0,
    float *phoneme,
    long *speaker_id,
    float *output
)
{
	DECODE decode = (DECODE)GetProcAddress(m_handler, "decode_forward");
        return decode(
        length,
        phoneme_size,
        f0,
        phoneme,
        speaker_id,
        output
    );
}

