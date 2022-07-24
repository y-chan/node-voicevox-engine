{
  "targets": [
    {
      "target_name": "openjtalk",
      "type": "static_library",
      "sources": [
        "lib/open_jtalk/src/jpcommon/jpcommon_label.c",
        "lib/open_jtalk/src/jpcommon/jpcommon_node.c",
        "lib/open_jtalk/src/jpcommon/jpcommon.c",
        "lib/open_jtalk/src/mecab/src/char_property.cpp",
        "lib/open_jtalk/src/mecab/src/connector.cpp",
        "lib/open_jtalk/src/mecab/src/context_id.cpp",
        "lib/open_jtalk/src/mecab/src/dictionary.cpp",
        "lib/open_jtalk/src/mecab/src/dictionary_compiler.cpp",
        "lib/open_jtalk/src/mecab/src/dictionary_generator.cpp",
        "lib/open_jtalk/src/mecab/src/dictionary_rewriter.cpp",
        "lib/open_jtalk/src/mecab/src/eval.cpp",
        "lib/open_jtalk/src/mecab/src/feature_index.cpp",
        "lib/open_jtalk/src/mecab/src/iconv_utils.cpp",
        "lib/open_jtalk/src/mecab/src/lbfgs.cpp",
        "lib/open_jtalk/src/mecab/src/learner.cpp",
        "lib/open_jtalk/src/mecab/src/learner_tagger.cpp",
        "lib/open_jtalk/src/mecab/src/libmecab.cpp",
        "lib/open_jtalk/src/mecab/src/mecab.cpp",
        "lib/open_jtalk/src/mecab/src/nbest_generator.cpp",
        "lib/open_jtalk/src/mecab/src/param.cpp",
        "lib/open_jtalk/src/mecab/src/string_buffer.cpp",
        "lib/open_jtalk/src/mecab/src/tagger.cpp",
        "lib/open_jtalk/src/mecab/src/tokenizer.cpp",
        "lib/open_jtalk/src/mecab/src/utils.cpp",
        "lib/open_jtalk/src/mecab/src/viterbi.cpp",
        "lib/open_jtalk/src/mecab/src/writer.cpp",
        "lib/open_jtalk/src/mecab2njd/mecab2njd.c",
        "lib/open_jtalk/src/njd/njd_node.c",
        "lib/open_jtalk/src/njd/njd.c",
        "lib/open_jtalk/src/njd_set_accent_phrase/njd_set_accent_phrase.c",
        "lib/open_jtalk/src/njd_set_accent_type/njd_set_accent_type.c",
        "lib/open_jtalk/src/njd_set_digit/njd_set_digit.c",
        "lib/open_jtalk/src/njd_set_long_vowel/njd_set_long_vowel.c",
        "lib/open_jtalk/src/njd_set_pronunciation/njd_set_pronunciation.c",
        "lib/open_jtalk/src/njd_set_unvoiced_vowel/njd_set_unvoiced_vowel.c",
        "lib/open_jtalk/src/njd2jpcommon/njd2jpcommon.c",
        "lib/open_jtalk/src/text2mecab/text2mecab.c"
      ],
      "include_dirs": [
        "<(module_root_dir)/lib/open_jtalk/src/jpcommon",
        "<(module_root_dir)/lib/open_jtalk/src/mecab/src",
        "<(module_root_dir)/lib/open_jtalk/src/mecab2njd",
        "<(module_root_dir)/lib/open_jtalk/src/mecab-naist-jdic",
        "<(module_root_dir)/lib/open_jtalk/src/njd",
        "<(module_root_dir)/lib/open_jtalk/src/njd_set_accent_phrase",
        "<(module_root_dir)/lib/open_jtalk/src/njd_set_accent_type",
        "<(module_root_dir)/lib/open_jtalk/src/njd_set_digit",
        "<(module_root_dir)/lib/open_jtalk/src/njd_set_long_vowel",
        "<(module_root_dir)/lib/open_jtalk/src/njd_set_pronunciation",
        "<(module_root_dir)/lib/open_jtalk/src/njd_set_unvoiced_vowel",
        "<(module_root_dir)/lib/open_jtalk/src/njd2jpcommon",
        "<(module_root_dir)/lib/open_jtalk/src/text2mecab"
      ],
      "defines": [
        "MECAB_WITHOUT_MUTEX_LOCK",
        "CHARSET_UTF_8",
        "DIC_VERSION=102",
        'VERSION="1.01"',
        'PACKAGE="open_jtalk"'
      ],
      "conditions": [
        [
          "OS=='win'",
          {
            "msbuild_settings": {
              'ClCompile': {
                'AdditionalOptions': ["/utf-8"]
              },
            },
            "msvs_settings": {
              "VCCLCompilerTool": {
                "ExceptionHandling": "2"
              },
            },
            "defines": [
              "HAVE_WINDOWS_H"
            ]
          }
        ],
        [
          "OS!='win'",
          {
            "defines": [
              "HAVE_MMAP",
              "HAVE_SYS_MMAN_H",
              "HAVE_UNISTD_H",
              "HAVE_SYS_STAT_H",
              "HAVE_FCNTL_H",
              "HAVE_DIRENT_H",
              "HAVE_STDINT_H"
            ]
          }
        ]
      ]
    },
    {
      "target_name": "engine",
      "sources": [
        "engine.cc",
        "engine.h",
        "core/core.cc",
        "core/core.h",
        "engine/nlohmann/json.hpp",
        "engine/acoustic_feature_extractor.cc",
        "engine/acoustic_feature_extractor.h",
        "engine/full_context_label.cc",
        "engine/full_context_label.h",
        "engine/kana_parser.cc",
        "engine/kana_parser.h",
        "engine/mora_list.cc",
        "engine/mora_list.h",
        "engine/openjtalk.cc",
        "engine/openjtalk.h",
        "engine/part_of_speech_data.h",
        "engine/synthesis_engine.cc",
        "engine/synthesis_engine.h"
      ],
      "dependencies": ["openjtalk"],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")",
        "<(module_root_dir)/lib/open_jtalk/src/jpcommon",
        "<(module_root_dir)/lib/open_jtalk/src/mecab/src",
        "<(module_root_dir)/lib/open_jtalk/src/mecab2njd",
        "<(module_root_dir)/lib/open_jtalk/src/mecab-naist-jdic",
        "<(module_root_dir)/lib/open_jtalk/src/njd",
        "<(module_root_dir)/lib/open_jtalk/src/njd_set_accent_phrase",
        "<(module_root_dir)/lib/open_jtalk/src/njd_set_accent_type",
        "<(module_root_dir)/lib/open_jtalk/src/njd_set_digit",
        "<(module_root_dir)/lib/open_jtalk/src/njd_set_long_vowel",
        "<(module_root_dir)/lib/open_jtalk/src/njd_set_pronunciation",
        "<(module_root_dir)/lib/open_jtalk/src/njd_set_unvoiced_vowel",
        "<(module_root_dir)/lib/open_jtalk/src/njd2jpcommon",
        "<(module_root_dir)/lib/open_jtalk/src/text2mecab"
      ],
      "cflags!": [ "-fno-exceptions" ],
      "cflags_cc!": [ "-fno-exceptions" ],
      "defines": [ "NAPI_CPP_EXCEPTIONS" ],
      "conditions": [
        [
          "OS=='win'",
          {
            "msbuild_settings": {
              'ClCompile': {
                'AdditionalOptions': ["/utf-8"]
              },
            },
            "msvs_settings": {
              "VCCLCompilerTool": {
                "ExceptionHandling": "2"
              },
            },
            "libraries": [ "<(module_root_dir)/build/Release/openjtalk.lib" ],
          }
        ],
        [
          "OS!='win'",
          {
            "libraries": [ "<(module_root_dir)/build/Release/openjtalk.a" ],
          }
        ],
        [
          "OS=='mac'",
          {
            "xcode_settings": {
              "GCC_ENABLE_CPP_EXCEPTIONS": "YES", # -fno-exceptions
            }
          }
        ]
      ]
    }
  ]
}
