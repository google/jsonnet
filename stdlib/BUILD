package(default_visibility = ["//visibility:private"])

filegroup(
    name = "stdlib",
    srcs = ["std.jsonnet"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "std",
    includes = ["."],
    linkstatic = 1,
    textual_hdrs = [":gen-std-jsonnet-h"],
    visibility = ["//core:__pkg__"],
)

genrule(
    name = "gen-std-jsonnet-h",
    srcs = ["std.jsonnet"],
    outs = ["std.jsonnet.h"],
    cmd = "((od -v -Anone -t u1 $< | tr \" \" \"\n\" | grep -v \"^$$\" " +
          "| tr \"\n\" \",\" ) && echo \"0\") > $@; " +
          "echo >> $@",
)
