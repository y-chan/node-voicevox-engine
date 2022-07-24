#ifndef OPENJTALK_H
#define OPENJTALK_H

#include <stdexcept>
#include <string>
#include <vector>

#include <mecab.h>
#include <njd.h>
#include <jpcommon.h>

BOOL Mecab_load_ex(Mecab* m, const char* dicdir, const char* userdic);
void create_user_dict(std::string dn_mecab, std::string path, std::string out_path);

class OpenJTalk {
public:
    Mecab* mecab;
    NJD* njd;
    JPCommon* jpcommon;
    std::string dn_mecab;
    std::string user_mecab;
    std::string default_dict_path;
    std::string user_dict_path;

    OpenJTalk() {
        mecab = new Mecab();
        njd = new NJD();
        jpcommon = new JPCommon();

        Mecab_initialize(mecab);
        NJD_initialize(njd);
        JPCommon_initialize(jpcommon);
    }

    OpenJTalk(std::string dn_mecab) : OpenJTalk() {
        load(dn_mecab);
    }

    OpenJTalk(std::string dn_mecab, std::string user_mecab) : OpenJTalk() {
        load_ex(dn_mecab, user_mecab);
    }

    ~OpenJTalk() {
        clear();
    }

    std::vector<std::string> extract_fullcontext(std::string text);

    void load(std::string dn_mecab);
    void load_ex(std::string dn_mecab, std::string user_mecab);
    void clear();
};

#endif // OPENJTALK_H
