licenses(["permissive"])
package(default_visibility = ["//visibility:private"])

cc_library(
    name = "debugbreak",
    hdrs = glob(["rapidyaml/ext/c4core/src/c4/ext/debugbreak/debugbreak.h"]),
    srcs = [],
)

cc_library(
    name = "fastfloat",
    includes = ["rapidyaml/ext/c4core/src/c4/ext"],
    hdrs = glob(["rapidyaml/ext/c4core/src/c4/ext/fast_float.hpp", "rapidyaml/ext/c4core/src/c4/ext/fast_float/**/*.h"]),
    srcs = [],
)

cc_library(
    name = "c4core",
    deps = [
      ":debugbreak",
      ":fastfloat",
    ],
    includes = ["rapidyaml/ext/c4core/src"],
    hdrs = glob(
        ["rapidyaml/ext/c4core/src/**/*.hpp"],
        exclude = ["rapidyaml/ext/c4core/src/c4/ext/**/*"],
    ),
    srcs = glob(
        ["rapidyaml/ext/c4core/src/**/*.cpp"],
        exclude = ["rapidyaml/ext/c4core/src/c4/ext/**/*"],
    ),
)

cc_library(
    name = "ryml",
    visibility = ["//visibility:public"],
    deps = [
      ":c4core",
    ],
    includes = ["rapidyaml/src"],
    hdrs = glob(["rapidyaml/src/**/*.hpp"]),
    srcs = glob(["rapidyaml/src/**/*.cpp"]),
)
