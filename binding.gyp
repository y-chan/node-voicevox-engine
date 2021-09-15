{
  "targets": [
    {
      "target_name": "core",
      "sources": [
        "wrapper.cc",
        "wrapper.h",
        "core.cc",
        "core.h"
      ],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")"
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
