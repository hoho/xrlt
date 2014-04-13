{
    "target_defaults": {
        "configurations": {
            "Release": {
                "include_dirs": [
                    ".",
                    "deps/yajl/build/yajl-<(yajl_version)/include",
                    "<(libxml_include_path)",
                ],
            },
            "Debug": {
                "include_dirs": [
                    ".",
                    "deps/yajl/build/yajl-<(yajl_version)/include",
                    "<(libxml_include_path)",
                ],
            },
        },
        "conditions": [
            ["OS in 'linux freebsd openbsd solaris android'", {
                "cflags": ["-Wall", "-Wextra", "-Wno-unused-parameter", "-Werror"],
                "cflags_cc": ["-fno-rtti", "-fno-exceptions"],
                "ldflags": ["-rdynamic"],
            }],
            ["OS=='mac'", {
                "xcode_settings": {
                    "OTHER_CFLAGS": ["-fno-strict-aliasing",],
                    "WARNING_CFLAGS": ["-Wall", "-Wextra", "-Wno-unused-parameter", "-Werror",],
                }
            }],
        ],
    },
    "targets": [
        {
            "target_name": "libxrlt",
            "type": "<(library)",
            "sources": [
                "xrlt.cc",
                "xrlterror.cc",
                "transform.cc",
                "xpathfuncs.cc",
                "json2xml.cc",
                "xml2json.cc",
                "querystring.cc",
                "include.cc",
                "variable.cc",
                "headers.cc",
                "function.cc",
                "import.cc",
                "choose.cc",
                "if.cc",
                "log.cc",
                "response.cc",
                "valueof.cc",
                "copyof.cc",
                "foreach.cc",
                "ccan_json.cc",

                "deps/yajl/src/yajl.c",
                "deps/yajl/src/yajl_alloc.c",
                "deps/yajl/src/yajl_buf.c",
                "deps/yajl/src/yajl_encode.c",
                "deps/yajl/src/yajl_lex.c",
                "deps/yajl/src/yajl_parser.c",
            ],
            "conditions": [
                ["xrlt_js==1", {
                    "dependencies": [
                        "deps/v8/tools/gyp/v8.gyp:v8",
                    ],
                    "sources": [
                        "xml2js.cc",
                        "js.cc",
                    ],
                }, {
                    "defines": [
                        "__XRLT_NO_JAVASCRIPT__"
                    ],
                }],
                ["OS in 'linux freebsd openbsd solaris android'", {
                    "cflags": ["-fPIC"],
                }],
            ],
            "libraries": [
                "-lxml2",
                "-lxslt",
            ],
        },
        {
            "target_name": "transform_test",
            "type": "executable",
            "dependencies": [
                "libxrlt",
            ],
            "sources": [
                "tests/transform/test.c",
            ],
        },
    ],
}
