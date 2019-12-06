local cmd = import "mmlib/v0.1.2/cmd/cmd.libsonnet";

{
    environments: import "../testenv.libsonnet",

    local SingleGoogleInstance(name) = {
        local service = self,

        environment: "google",

        zone:: error "Must override zone.",
        image:: error "Must override zone.",
        machineType:: "g1-small",
        cmds:: [],

        externalIp:: false,
        infrastructure: {
            google_compute_instance: {
                name: {
                    name: name,
                    machine_type: service.machineType,
                    zone: service.zone,
                    boot_disk: {
                        initialize_params: {
                            image: service.image,
                        },
                    },
                    network_interface: {
                        network: "default",
                        access_config:
                            if service.externalIp then [{}] else [],
                    },
                    metadata: {
                    },
                    cmds: service.cmds,
                    bootCmds: [],
                },
            },
        },
        outputs: {
            [if service.externalIp then name + "-address"]:
                "${google_compute_instance.%s.network_interface[0].access_config.0.nat_ip}" % name,
        },
    },

    wheezy: SingleGoogleInstance('wheezy') {
        image: "debian-9-stretch-v20191121",
        zone: "us-central1-b",
    },
    ubuntu: SingleGoogleInstance('ubuntu') {
        image: "ubuntu-1804-bionic-v20191113",
        zone: "us-central1-c",
    },
    "core-os": SingleGoogleInstance('core-os') {
        image: "coreos-stable-2303-3-0-v20191203",
        zone: "us-central1-f",
        externalIp: true,
        cmds: [
            "echo hi > /hi.txt",
            cmd.LiteralFile {
                to: "/var/log/bye.txt",
                content: |||
                    bye
                |||,
                filePermissions: "700",
            },
        ],
    },
    custom: SingleGoogleInstance('custom') {
        image: {
            source: "ubuntu-1804-bionic-v20191113",
            zone: "us-central1-f",
            cmds: [
                "echo hi > /hi.txt",
                cmd.LiteralFile {
                    to: "/var/log/bye.txt",
                    content: |||
                        bye
                    |||,
                    filePermissions: "700",
                },
            ],
        },
        zone: "us-central1-f",
        externalIp: true,
    },
}
