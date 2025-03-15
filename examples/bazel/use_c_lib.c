#include <libjsonnet.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    int ret = 0;
    fprintf(stderr, "Jsonnet version: %s\n", jsonnet_version());
    struct JsonnetVm *vm = jsonnet_make();
    if (!vm) { return 1; }

    int err = 0;
    char *result = jsonnet_evaluate_file(vm, "example.jsonnet", &err);
    if (!err) {
        fprintf(stdout, "%s\n", result ? result : "-- no output --");
    } else {
        fprintf(stderr, "%s\n", result ? result : "-- no output --");
        ret = 2;
    }

    jsonnet_destroy(vm);
    return ret;
}
