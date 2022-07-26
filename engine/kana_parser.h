#ifndef KANA_PARSER_H
#define KANA_PARSER_H

#include <napi.h>

#include <map>
#include <string>
#include <vector>

#include "mora_list.h"

const int LOOP_LIMIT = 300;
const std::string UNVOICE_SYMBOL = "_";
const std::string ACCENT_SYMBOL = "'";
const std::string NOPAUSE_DELIMITER = "/";
const std::string PAUSE_DELIMITER = "、";
const std::string WIDE_INTERROGATION_MARK = "？";

static const std::map<std::string, Napi::Object> text2mora_with_unvoice(Napi::Env env);
std::string extract_one_character(const std::string& text, size_t pos, size_t& size);

Napi::Object text_to_accent_phrase(Napi::Env env, std::string phrase);
Napi::Array parse_kana(Napi::Env env, std::string text);
Napi::String create_kana(Napi::Env env, Napi::Array accent_phrases);

#endif // KANA_PARSER_H
