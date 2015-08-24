{
    environments: import "testenv.jsonnet",

    empty_service: {
        kind: "Google",
        environment: "google",
        infrastructure: {},
        children: {},
        outputs: {},
    },
}
