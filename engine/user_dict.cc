#include <algorithm>
#include <iostream>
#include <regex>

#include "uuid/uuid_v4.h"
#include "user_dict.h"
#include "kana_parser.h"
#include "part_of_speech_data.h"

void write_to_json(json user_dict, std::string user_dict_path) {
    json converted_user_dict;
    for (auto& word : user_dict.items()) {
        std::string word_uuid = word.key();
        json word_dict = word.value();
        word_dict["cost"] = priority2cost(word_dict["context_id"].get<int>(), word_dict["priority"].get<int>());
        word_dict.erase("priority");
        converted_user_dict[word_uuid] = word_dict;
    }
    std::string user_dict_json = converted_user_dict.dump();
    std::ofstream output_file(user_dict_path);
    output_file << user_dict_json;
}

void user_dict_startup_processing(OpenJTalk *openjtalk) {
    std::string default_dict_path = openjtalk->default_dict_path;
    std::string user_dict_path = openjtalk->user_dict_path;
    create_user_dict(openjtalk->dn_mecab, openjtalk->default_dict_path, openjtalk->user_mecab);
    openjtalk = new OpenJTalk(openjtalk->dn_mecab, openjtalk->user_mecab);
    openjtalk->default_dict_path = default_dict_path;
    openjtalk->user_dict_path = user_dict_path;
}

void update_dict(OpenJTalk *openjtalk) {
    std::string temp_path = std::tmpnam(nullptr);
    std::ofstream temp_file(temp_path);
    std::ifstream default_dict_file(openjtalk->default_dict_path);
    if (!default_dict_file) {
        std::cout << "Warning: Cannot find default dictionary." << std::endl;
        return;
    }
    std::string default_dict((std::istreambuf_iterator<char>(default_dict_file)), std::istreambuf_iterator<char>());
    if (default_dict[default_dict.size() - 1] != '\n') {
        default_dict += "\n";
    }
    temp_file << default_dict;
    json user_dict = read_dict(openjtalk->user_dict_path);
    for (auto &item : user_dict.items()) {
        json &word = item.value();
        std::string word_info = (
            word["surface"].get<std::string>() + "," +
            std::to_string(word["context_id"].get<int>()) + "," +
            std::to_string(word["context_id"].get<int>()) + "," +
            std::to_string(priority2cost(word["context_id"].get<int>(), word["priority"].get<int>())) + "," +
            word["part_of_speech"].get<std::string>() + "," +
            word["part_of_speech_detail_1"].get<std::string>() + "," +
            word["part_of_speech_detail_2"].get<std::string>() + "," +
            word["part_of_speech_detail_3"].get<std::string>() + "," +
            word["inflectional_type"].get<std::string>() + "," +
            word["inflectional_form"].get<std::string>() + "," +
            word["stem"].get<std::string>() + "," +
            word["yomi"].get<std::string>() + "," +
            word["pronunciation"].get<std::string>() + "," +
            std::to_string(word["accent_type"].get<int>()) + "," +
            (word.count("mora_count") >= 1 ? std::to_string(word["mora_count"].get<int>()) : "") + "," +
            word["accent_associative_rule"].get<std::string>() + "\n"
        );
        temp_file << word_info;
    }
    temp_file.close();
    std::string temp_dict_path = std::tmpnam(nullptr);
    create_user_dict(openjtalk->dn_mecab, temp_path, temp_dict_path);
    std::ifstream temp_dict_file(temp_dict_path, std::ios::in | std::ios::binary);
    if (!temp_dict_file) {
        throw std::runtime_error("An error occurred while compiling user dictionary.");
    }
    std::string default_dict_path = openjtalk->default_dict_path;
    std::string user_dict_path = openjtalk->user_dict_path;
    std::string compiled_dict_path = openjtalk->user_mecab;
    openjtalk = new OpenJTalk(openjtalk->dn_mecab);
    std::ofstream compiled_dict_file(compiled_dict_path, std::ios::out | std::ios::trunc | std::ios::binary);
    compiled_dict_file << temp_dict_file.rdbuf();
    compiled_dict_file.close();
    openjtalk = new OpenJTalk(openjtalk->dn_mecab, compiled_dict_path);
    openjtalk->default_dict_path = default_dict_path;
    openjtalk->user_dict_path = user_dict_path;
}

json read_dict(std::string user_dict_path) {
    std::ifstream user_dict_file(user_dict_path);
    std::string user_dict;

    if (!user_dict_file) {
        return json::object();
    }
    user_dict_file >> user_dict;
    json user_dict_json = json::parse(user_dict);

    for (auto &item : user_dict_json.items()) {
        json &word = item.value();
        if (word.contains("context_id")) {
            word["context_id"] = part_of_speech_data["PROPER_NOUN"]["context_id"];
        }
        word["priority"] = cost2priority(word["context_id"], word["cost"]);
        word.erase("cost");
    }

    return user_dict_json;
}

json create_word(std::string surface, std::string pronunciation, int accent_type, std::string *word_type, int *priority) {
    std::string word_type_str = word_type != nullptr ? *word_type : "PROPER_NOUN";
    bool key_found = false;
    for (auto item : part_of_speech_data.items()) {
        key_found = word_type_str == item.key();
        if (key_found) break;
    }
    if (!key_found) {
        throw std::runtime_error("invalid word type");
    }
    int priority_num = priority != nullptr ? *priority : 5;
    if (!(USER_DICT_MIN_PRIORITY <= priority_num && priority_num <= USER_DICT_MAX_PRIORITY)) {
        throw std::runtime_error("invalid priority");
    }

    // hankaku to zenkaku
    // replace !(exclamation mark) to ~(tilde)
    for (int i = 0; i < 94; i++) {
        std::vector<unsigned char> hankaku{ (unsigned char)(0x21 + i) };
        std::vector<unsigned char> zenkaku;
        if (i < 63) zenkaku = { 0xEF, 0xBC, (unsigned char)(0x81 + i) };
        else zenkaku = { 0xEF, 0xBD, (unsigned char)(0x80 + i - 63) };

        std::string hankaku_str((char *)hankaku.data(), hankaku.size());
        std::string zenkaku_str((char *)zenkaku.data(), zenkaku.size());
        // 正規表現に使われる文字列はエスケープする
        if (
            hankaku_str == "$" ||
            hankaku_str == "(" ||
            hankaku_str == ")" ||
            hankaku_str == "*" ||
            hankaku_str == "+" ||
            hankaku_str == "-" ||
            hankaku_str == "." ||
            hankaku_str == "?" ||
            hankaku_str == "[" ||
            hankaku_str == "\\" ||
            hankaku_str == "]" ||
            hankaku_str == "^" ||
            hankaku_str == "{" ||
            hankaku_str == "|" ||
            hankaku_str == "}"
        ) {
            hankaku_str = "\\" + hankaku_str;
        }
        surface = std::regex_replace(surface, std::regex(hankaku_str), zenkaku_str);
    }

    json pos_detail = part_of_speech_data[word_type_str];
    json result = {
        { "surface", surface },
        { "context_id", pos_detail["context_id"].get<int>() },
        { "priority", priority_num },
        { "part_of_speech", pos_detail["part_of_speech"].get<std::string>() },
        { "part_of_speech_detail_1", pos_detail["part_of_speech_detail_1"].get<std::string>() },
        { "part_of_speech_detail_2", pos_detail["part_of_speech_detail_2"].get<std::string>() },
        { "part_of_speech_detail_3", pos_detail["part_of_speech_detail_3"].get<std::string>() },
        { "inflectional_type", "*" },
        { "inflectional_form", "*" },
        { "stem", "*" },
        { "yomi", pronunciation },
        { "pronunciation", pronunciation },
        { "accent_type", accent_type },
        { "accent_associative_rule", "*" },
    };
    return result;
}

std::string apply_word(
    OpenJTalk *openjtalk,
    std::string surface,
    std::string pronunciation,
    int accent_type,
    std::string *word_type,
    int *priority
) {
    json word = create_word(surface, pronunciation, accent_type, word_type, priority);
    json user_dict = read_dict(openjtalk->user_dict_path);
    UUIDv4::UUIDGenerator<std::mt19937_64> uuidGenerator;
    UUIDv4::UUID word_uuid = uuidGenerator.getUUID();
    std::string word_uuid_str = word_uuid.str();
    user_dict[word_uuid_str] = word;
    write_to_json(user_dict, openjtalk->user_dict_path);
    update_dict(openjtalk);
    return word_uuid_str;
}

void rewrite_word(
    OpenJTalk *openjtalk,
    std::string word_uuid,
    std::string surface,
    std::string pronunciation,
    int accent_type,
    std::string *word_type,
    int *priority
) {
    json word = create_word(surface, pronunciation, accent_type, word_type, priority);
    json user_dict = read_dict(openjtalk->user_dict_path);
    user_dict[word_uuid] = word;
    write_to_json(user_dict, openjtalk->user_dict_path);
    update_dict(openjtalk);
}

void delete_word(OpenJTalk *openjtalk, std::string word_uuid) {
    json user_dict = read_dict(openjtalk->user_dict_path);
    bool key_found = false;
    for (auto item : user_dict.items()) {
        key_found = word_uuid == item.key();
        if (key_found) break;
    }
    if (!key_found) {
        throw std::runtime_error("not found uuid");
    }
    user_dict.erase(word_uuid);
    write_to_json(user_dict, openjtalk->user_dict_path);
    update_dict(openjtalk);
}

// TODO: 実装する
void import_user_dict(
    OpenJTalk openjtalk,
    json dict_data,
    bool override
) {
}

std::vector<int> search_cost_candidates(int context_id) {
    for (auto &elem : part_of_speech_data.items()) {
        auto &value = elem.value();
        if (value["context_id"].get<int>() == context_id) {
            return value["cost_candidates"].get<std::vector<int>>();
        }
    }
    throw std::runtime_error("invalid context id");
}

int cost2priority(int context_id, int cost) {
    if (cost < -32768 || 32767 < cost) {
        throw std::runtime_error("invalid cost value");
    }
    auto cost_candidates = search_cost_candidates(context_id);
    int min_value = 0;
    int min_index = -1;
    for (int i = 0; i < cost_candidates.size(); i++) {
        int cost_candidate = cost_candidates[i] - cost;
        cost_candidate = cost_candidate < 0 ? -cost_candidate : cost_candidate;
        if (min_value >= cost_candidate || min_index == -1) {
            min_value = cost_candidate;
            min_index = i;
        }
    }
    return USER_DICT_MAX_PRIORITY - min_index;
}

int priority2cost(int context_id, int priority) {
    if (priority < USER_DICT_MIN_PRIORITY || USER_DICT_MAX_PRIORITY < priority) {
        throw std::runtime_error("invalid priority value");
    }
    auto cost_candidates = search_cost_candidates(context_id);
    return cost_candidates[USER_DICT_MAX_PRIORITY - priority];
}
