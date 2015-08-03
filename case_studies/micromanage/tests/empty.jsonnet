{
    environments: {
        google: {
            kind: "Google",
            project: std.extVar("GCP_PROJECT"),
            region: "us-central1",
            serviceAccount: {
                client_email: std.extVar("GCP_EMAIL"),
                private_key: std.extVar("GCP_KEY"),
            },
            sshUser: std.extVar("USER"),
        },
    },

    empty_service: {
        kind: "Google",
        environment: "google",
        infrastructure: {},
        children: {},
        outputs: {},
    },
}
