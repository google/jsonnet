local base = import "base.jsonnet";
local cmd = import "../cmd/cmd.jsonnet";
local apt = import "../cmd/apt.jsonnet";
local amis_debian = import "../amis/debian.jsonnet";

{
    Credentials:: {
        kind: "Amazon",
        accessKey: error "Amazon credentials must have 'accessKey'",
        secretKey: error "Amazon credentials must have 'secretKey'",
        region: "us-east-1",
    },


    Image:: {
        sourceAmi: error "Amazon AMI must have 'sourceAmi'",
        instanceType: "t2.small",
        sshUser: error "Amazon AMI must have 'sshUser'",
        cmds: [],
    },


    DebianImage:: $.Image + apt.Mixin + apt.PipMixin {
        sourceAmi: amis_debian.wheezy.amd64["20150128"]["us-west-1"],
        sshUser: "admin",
    },


    StandardInstance:: {
        local instance = self,
        instance_type: "t2.small",
        ami: instance.StandardRootImage,
        associate_public_ip_address: true,
        cmds: [],
        bootCmds: [],
        # TODO(dcunnin): Figure out an equivalent here.
        supportsLogging:: false,
        supportsMonitoring:: false,
        supportsJmxMonitoring:: false,
        enableLogging:: false,
        enableMonitoring:: false,
        enableJmxMonitoring:: false,
        MonitoringLoggingImageMixin:: {
        },
        StandardRootImage:: $.DebianImage + instance.MonitoringLoggingImageMixin,
    },


    Service:: base.Service {
    },


    Network:: $.Service {
        local service = self,
        refId(name):: "${aws_vpc.%s.id}" % name,
        refName(name):: "${aws_vpc.%s.name}" % name,
        refSubnetId(name, zone):: "${aws_subnet.%s-%s.id}" % [name, zone],
        ipv4Range:: "10.0.0.0/16",
        infrastructure: {
            aws_vpc: {
                "${-}": {
                    cidr_block: service.ipv4Range,
                },
            },
            aws_internet_gateway: {
                "${-}": {
                    vpc_id: "${aws_vpc.${-}.id}",
                },
            },
            aws_route_table: {
                "${-}": {
                    vpc_id: "${aws_vpc.${-}.id}",
                    route: {
                        cidr_block: "0.0.0.0/0",
                        gateway_id: "${aws_internet_gateway.${-}.id}",
                    },
                },
            },
            aws_subnet: {
                ["${-}-" + zone]: {
                    vpc_id: "${aws_vpc.${-}.id}",
                    cidr_block: service.subnets[zone],
                    availability_zone: zone,
                }
                for zone in std.objectFields(service.subnets)
            },
            aws_route_table_association: {
                ["${-}-" + zone]: {
                    subnet_id: "${aws_subnet.${-}-" + zone + ".id}",
                    route_table_id: "${aws_route_table.${-}.id}",
                }
                for zone in std.objectFields(service.subnets)
            },
        },
        // Maps zone to CIDR.
        subnets:: {},
    },


    InstanceBasedService: $.Service {
        local service = self,
        fwTcpPorts:: [22],
        fwUdpPorts:: [],
        networkName:: null,
        keyName:: error "InstanceBasedService needs keyName",

        Mixin:: (
            if service.networkName != null then {
                vpc_security_group_ids: ["${aws_security_group.${-}.id}"],
                subnet_id: $.Network.refSubnetId(service.networkName, self.availability_zone),
            } else {
                security_groups: ["${aws_security_group.${-}.name}"],
            }
        ) + {
            [if service.keyName != null then "key_name"]: service.keyName,
        },

        Instance:: $.StandardInstance + service.Mixin,

        infrastructure+: {
            aws_security_group: {
                "${-}": {
                    name: "${-}",

                    local IngressRule(p, protocol) = {
                        from_port: p,
                        to_port: p,
                        protocol: protocol,
                        cidr_blocks: ["0.0.0.0/0"],
                    },
                    ingress: [IngressRule(p, "tcp") for p in service.fwTcpPorts]
                           + [IngressRule(p, "udp") for p in service.fwUdpPorts],

                    [if service.networkName != null then "vpc_id"]: $.Network.refId(service.networkName),
                }
            },
            aws_instance: error "InstanceBasedService should define some instances.",
        },
    },


    Cluster3: base.Service {
        local service = self,
        lbTcpPorts:: [],
        lbUdpPorts:: [],
        fwTcpPorts:: [22],
        fwUdpPorts:: [],
        httpHealthCheckPort:: 80,
        zones:: error "Cluster3 version (or service) needs an array of zones.",

        // In this service, the 'instance' is really an instance template used to create
        // a pool of instances.
        Mixin:: {
            networkName: service.networkName,
            zones:: service.zones,
        },
        versions:: {},
        deployment:: {},
        local instances = std.foldl(function(a, b) a + b, [
            {
                ["${-}-%s-%d" % [vname, i]]:
                    if std.objectHas(service.versions, vname) then
                        service.versions[vname] {
                            name: "${-}-%s-%d" % [vname, i],
                            zone: self.zones[i % std.length(self.zones)],
                            tags+: [vname, "index-%d" % i]
                        }
                    else
                        error "Undefined version: %s" % vname
                for i in std.set(service.deployment[vname].deployed)
            }
            for vname in std.objectFields(service.deployment)
        ], {}),
        local attached_instances = std.join([], [
            local attached = std.set(service.deployment[vname].attached);
            local deployed = std.set(service.deployment[vname].deployed);
            ["${-}-%s-%d" % [vname, i] for i in std.setInter(attached, deployed)]
            for vname in std.objectFields(service.deployment)
        ]),

        refAddress(name):: "${aws_elb.%s.address}" % name,

        infrastructure+: {
            aws_instance: instances,

            aws_elb: {
                "{-}": {
                    name: "{-}",

                    listener: [{
                        instance_port: p,
                        instance_protocol: "tcp",
                        lb_port: p,
                        lb_protocol: "tcp",
                    } for p in service.fwTcpPorts],

                    health_check: {
                        healthy_threshold: 2,
                        unhealthy_threshold: 2,
                        timeout: 5,
                        target: "HTTP:%d/" % service.httpHealthCheckPort,
                        interval: 5,
                    },

                    instances: ["${aws_instance.%s.id}" % iname for iname in attached_instances],
                    cross_zone_load_balancing: true,
                    idle_timeout: 60,
                    connection_draining: true,
                    connection_draining_timeout: 60,

                    tags: {
                        Name: "foobar-terraform-elb",
                    },
                },
            },
        },
    },


    SingleInstance: $.InstanceBasedService {
        local service = self,
        zone:: error "SingleInstance needs a zone.",

        refAddress(name):: "${aws_instance.%s.public_ip}" % name,

        infrastructure+: {
            aws_instance: {
                "${-}": service.Instance {
                    availability_zone: service.zone,
                },
            },
        },
    },
}
