#include <cstring>

#include "openjtalk.h"

#include <mecab2njd.h>
#include <njd2jpcommon.h>
#include <njd_set_accent_phrase.h>
#include <njd_set_accent_type.h>
#include <njd_set_digit.h>
#include <njd_set_long_vowel.h>
#include <njd_set_pronunciation.h>
#include <njd_set_unvoiced_vowel.h>
#include <text2mecab.h>

BOOL Mecab_load_ex(Mecab* m, const char* dicdir, const char* userdic)
{
    int i;

    if (userdic == NULL || strlen(userdic) == 0) {
        return Mecab_load(m, dicdir);
    }

    if (m == NULL || dicdir == NULL || strlen(dicdir) == 0) {
        return FALSE;
    }

    Mecab_clear(m);

    std::vector<char *> argv = { strdup("mecab"), strdup("-d"), strdup(dicdir), strdup("-u"), strdup(userdic) };

    MeCab::Model* model = MeCab::createModel(argv.size(), argv.data());

    if (model == NULL) {
        fprintf(stderr, "ERROR: Mecab_load_ex() in openjtalk.cc: Cannot open %s.\n", dicdir);
        return FALSE;
    }

    MeCab::Tagger* tagger = model->createTagger();
    if (tagger == NULL) {
        delete model;
        fprintf(stderr, "ERROR: Mecab_load_ex() in openjtalk.cc: Cannot open %s.\n", dicdir);
        return FALSE;
    }

    MeCab::Lattice* lattice = model->createLattice();
    if (lattice == NULL) {
        delete model;
        delete tagger;
        fprintf(stderr, "ERROR: Mecab_load_ex() in openjtalk.cc: Cannot open %s.\n", dicdir);
        return FALSE;
    }

    m->model = (void*)model;
    m->tagger = (void*)tagger;
    m->lattice = (void*)lattice;

    return TRUE;
}


void create_user_dict(std::string dn_mecab, std::string path, std::string out_path) {
    std::vector<char*> argv = { strdup("mecab-dict-index"), strdup("-d"), strdup(dn_mecab.c_str()), strdup("-u"), strdup(out_path.c_str()), strdup("-f"), strdup("utf-8"), strdup("-t"), strdup("utf-8"), strdup(path.c_str()) };
    mecab_dict_index(argv.size(), argv.data());
}

std::vector<std::string> OpenJTalk::extract_fullcontext(std::string text) {
    char buff[8192];
    text2mecab(buff, text.c_str());
    Mecab_analysis(mecab, buff);
    mecab2njd(njd, Mecab_get_feature(mecab), Mecab_get_size(mecab));
    njd_set_pronunciation(njd);
    njd_set_digit(njd);
    njd_set_accent_phrase(njd);
    njd_set_accent_type(njd);
    njd_set_unvoiced_vowel(njd);
    njd_set_long_vowel(njd);
    njd2jpcommon(jpcommon, njd);
    JPCommon_make_label(jpcommon);

    std::vector<std::string> labels;

    int label_size = JPCommon_get_label_size(jpcommon);
    char **label_feature = JPCommon_get_label_feature(jpcommon);

    labels.clear();
    for (int i = 0; i < label_size; i++) labels.push_back(label_feature[i]);

    JPCommon_refresh(jpcommon);
    NJD_refresh(njd);
    Mecab_refresh(mecab);

    return labels;
}

void OpenJTalk::load(std::string dn_mecab) {
    this->dn_mecab = dn_mecab;
    BOOL result = Mecab_load(mecab, dn_mecab.c_str());
    if (result != 1) {
        clear();
        throw std::runtime_error("failed to initialize mecab");
    }
}

void OpenJTalk::load_ex(std::string dn_mecab, std::string user_mecab) {
    this->dn_mecab = dn_mecab;
    this->user_mecab = user_mecab;
    BOOL result = Mecab_load_ex(mecab, dn_mecab.c_str(), user_mecab.c_str());
    if (result != 1) {
        clear();
        throw std::runtime_error("failed to initialize mecab");
    }
}


void OpenJTalk::clear() {
    Mecab_clear(mecab);
    NJD_clear(njd);
    JPCommon_clear(jpcommon);
}
