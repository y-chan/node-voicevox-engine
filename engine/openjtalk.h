#ifndef OPENJTALK_H
#define OPENJTALK_H

#include <stdexcept>
#include <string>
#include <vector>

#include <mecab.h>
#include <njd.h>
#include <jpcommon.h>

class OpenJTalk {
public:
    Mecab* mecab;
    NJD* njd;
    JPCommon* jpcommon;

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

    ~OpenJTalk() {
        clear();
    }

    std::vector<std::string> extract_fullcontext(std::string text);

    void load(std::string dn_mecab);
    void clear();
};

#endif // OPENJTALK_H
