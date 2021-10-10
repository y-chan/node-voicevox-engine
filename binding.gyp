{
  "targets": [
    {
      "target_name": "core",
      "sources": [
        "engine.cc",
        "engine.h",
        "core/core.cc",
        "core/core.h",
        "engine/acoustic_feature_extractor.cc",
        "engine/acoustic_feature_extractor.h",
        "engine/full_context_label.cc",
        "engine/full_context_label.h",
        "engine/mora_list.h",
        "engine/openjtalk.cc",
        "engine/openjtalk.h",
        "engine/synthesis_engine.cc",
        "engine/synthesis_engine.h"
      ],
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
            "msvs_settings": {
              "VCCLCompilerTool": {
                "ExceptionHandling": "2"
              },
            }
          }
        ]
      ]
    }
  ]
}
