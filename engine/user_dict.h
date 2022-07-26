#ifndef USER_DICT_H
#define USER_DICT_H

#include <fstream>
#include <iostream>
#include <vector>

#include "nlohmann/json.hpp"
#include "openjtalk.h"

using json = nlohmann::json;

void write_to_json(json user_dict, std::string user_dict_path);
OpenJTalk *user_dict_startup_processing(OpenJTalk *openjtalk);
OpenJTalk *update_dict(OpenJTalk *openjtalk);
json read_dict(std::string user_dict_path);
json create_word(std::string surface, std::string pronunciation, int accent_type, std::string *word_type = nullptr, int *priority = nullptr);
std::pair<std::string, OpenJTalk*> apply_word(
    OpenJTalk *openjtalk,
    std::string surface,
    std::string pronunciation,
    int accent_type,
    std::string *word_type = nullptr,
    int *priority = nullptr
);
OpenJTalk *rewrite_word(
    OpenJTalk *openjtalk,
    std::string word_uuid,
    std::string surface,
    std::string pronunciation,
    int accent_type,
    std::string *word_type = nullptr,
    int *priority = nullptr
);
OpenJTalk *delete_word(OpenJTalk *openjtalk, std::string word_uuid);
void import_user_dict(OpenJTalk *openjtalk, json dict_data, bool override);

std::vector<int> search_cost_candidates(int context_id);
int cost2priority(int context_id, int cost);
int priority2cost(int context_id, int priority);

#endif // USER_DICT_H
