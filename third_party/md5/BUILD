licenses(["permissive"])

package(default_visibility = ["//visibility:private"])

cc_library(
    name = "libmd5",
    srcs = [
        "md5.cpp",
    ],
    hdrs = [
        "md5.h",
    ],
    includes = ["."],
    linkopts = ["-lm"],
    visibility = ["//core:__pkg__"],
)
