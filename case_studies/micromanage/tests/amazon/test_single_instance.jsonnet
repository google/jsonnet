local libimgcmd = import "lib/libimgcmd.jsonnet";

{
    environments: import "../testenv.jsonnet",

    local SingleAmazonInstance = {
        local service = self,

        // Cut off the last letter
        local region_from_zone(z) = std.substr(z, 0, std.length(z) - 1),

        environment: "amazon",

        zone:: error "Must override zone.",
        amiMap:: error "Must override amiMap or ami.",
        ami:: null,
        machineType:: "t2.small",
        keyName:: null,
        cmds:: [],

        externalIp:: false,
        infrastructure: {
            aws_instance: {
                "${-}": {
                    [if service.keyName != null then "key_name"]: service.keyName,
                    instance_type: service.machineType,
                    availability_zone: service.zone,
                    ami: if service.ami != null then service.ami else service.amiMap[region_from_zone(service.zone)],
                    associate_public_ip_address: service.externalIp,
                    cmds: service.cmds,
                }
            }
        },
        outputs: {
            "${-}-address": "${aws_instance.${-}.public_ip}",
            "${-}-id": "${aws_instance.${-}.id}",
        },
    },

    local ubuntu_ami_map = {
        "ap-northeast-1": "ami-48c27448",
        "ap-southeast-1": "ami-86e3e1d4",
        "eu-central-1": "ami-88333695",
        "eu-west-1": "ami-c8a5eebf",
        "sa-east-1": "ami-1319960e",
        "us-east-1": "ami-d96cb0b2",
        "us-west-1": "ami-6988752d",
        "cn-north-1": "ami-9871eca1",
        "us-gov-west-1": "ami-25fc9c06",
        "ap-southeast-2": "ami-21eea81b",
        "us-west-2": "ami-d9353ae9",
    },

    ubuntu_aws: SingleAmazonInstance {
        externalIp: true,
        keyName: "kp",
        amiMap: ubuntu_ami_map,
        zone: "us-west-1b",
        cmds: [
            "echo hi > /hi.txt",
            libimgcmd.LiteralFile {
                to: "/var/log/bye.txt",
                content: |||
                    bye
                |||,
                filePermissions: "700",
            },
        ],
    },


    ubuntu_aws_ami: SingleAmazonInstance {
        externalIp: true,
        keyName: "kp",
        ami: {
            sourceAmi: "ami-6988752d",
            instanceType: "t2.small",
            sshUser: "ubuntu",
            cmds: [
                "echo hi > /hi.txt",
                libimgcmd.LiteralFile {
                    to: "/var/log/bye.txt",
                    content: |||
                        bye
                    |||,
                    filePermissions: "700",
                },
            ],
        },
        zone: "us-west-1b",
    },
}
