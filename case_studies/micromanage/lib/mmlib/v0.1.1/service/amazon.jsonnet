local base = import "base.jsonnet";

{
    Credentials:: {
        kind: "Amazon",
        accessKey: error "Amazon credentials must have 'accessKey'",
        secretKey: error "Amazon credentials must have 'secretKey'",
        region: "us-east-1",
    },

    StandardInstance:: {
        machine_type: "m1.small",
        ami: error "StandardInstance must have 'ami'",
        cmds: [],
        bootCmds: [],
    },

    Service:: base.Service {
    },

    Cluster3: base.Service {
        local service = self,
        lbTcpPorts:: [],
        lbUdpPorts:: [],
        fwTcpPorts:: [22],
        fwUdpPorts:: [],
        httpHealthCheckPort:: 80,
        networkName:: "default",
        zones:: error "Cluster3 version (or service) needs an array of zones.",

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

        infrastructure+: {
/*
            google_compute_firewall: {
                "${-}": {
                    name: "${-}",
                    source_ranges: ["0.0.0.0/0"],
                    network: service.networkName,
                    allow: [{ protocol: "tcp", ports: [std.toString(p) for p in service.fwTcpPorts]}],
                    target_tags: ["${-}"],
                }
            },
*/
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

    InstanceBasedService: $.Service {
        local service = self,
        fwTcpPorts:: [22],
        fwUdpPorts:: [],
        //networkName:: "default",

        Mixin:: {
            //networkName: service.networkName,
        },

        Instance:: $.StandardInstance + service.Mixin,

        refAddress(name):: "${aws_instance.%s.public_ip}" % name,

        infrastructure+: {
/*
            google_compute_firewall: {
                "${-}": {
                    name: "${-}",
                    source_ranges: ["0.0.0.0/0"],
                    network: service.networkName,
                    allow: [{ protocol: "tcp", ports: [std.toString(p) for p in service.fwTcpPorts]}],
                    target_tags: ["${-}"],
                }
            },
*/
            google_compute_instance: error "InstanceBasedService should define some instances.",
        },
    },
}
