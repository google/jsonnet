package(default_visibility = ["//visibility:public"])

filegroup(
    name = "std",
    srcs = ["std.jsonnet"],
)

genrule(
    name = "gen-std-jsonnet-h",
    srcs = ["std.jsonnet"],
    outs = ["std.jsonnet.h"],
    cmd = "((od -v -Anone -t u1 $< | tr \" \" \"\n\" | grep -v \"^$$\" " +
          "| tr \"\n\" \",\" ) && echo \"0\") > $@; " +
          "echo >> $@",
)
