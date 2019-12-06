local base = import "base.libsonnet";
local cmd = import "../cmd/cmd.libsonnet";
local apt = import "../cmd/apt.libsonnet";

{
    Credentials:: {
        kind: "Google",
        project: error "Google credentials must have 'project'",
        serviceAccount: error "Google credentials must have 'serviceAccount'",
        sshUser: error "Google credentials must have 'sshUser'",
        region: "us-central1",
    },


    Image:: {
        source: error "Google Image must have 'source'",
        machineType: "n1-standard-1",
        zone: "us-central1-f",
        cmds: [],
    },


    DebianImage:: $.Image + apt.Mixin + apt.PipMixin {
        source: "debian-9-stretch-v20191121",
    },


    StandardInstance(service):: {
        local instance = self,
        machine_type: "f1-micro",
        scopes:: ["cloud-platform"],
        networkName:: "default",
        cmds: [],
        bootCmds: [],
        network_interface: {
            network: instance.networkName,
            access_config: {
            },
        },
        service_account: [
            {
                scopes: ["https://www.googleapis.com/auth/" + s for s in instance.scopes],
            },
        ],
        boot_disk: {
            initialize_params: {
                image: instance.StandardRootImage,
            }
        },

        supportsLogging:: true,
        supportsMonitoring:: true,
        supportsJmxMonitoring:: true,
        enableLogging:: false,
        enableMonitoring:: false,
        enableJmxMonitoring:: false,
        jmxHost:: "localhost",
        jmxPort:: 9012,
        jmxHotspotConfig:: {
            host: instance.jmxHost,
            port: instance.jmxPort,
            numQueryThreads: 2,
            SdQuery:: {
                outputWriters: [
                    {
                        "@class": "com.googlecode.jmxtrans.model.output.StackdriverWriter",
                        settings: {
                            token: "STACKDRIVER_API_KEY",
                            detectInstance: "GCE",
                            url: "https://jmx-gateway.google.stackdriver.com/v1/custom",
                        },
                    },
                ],
            },
            queries: [
                self.SdQuery {
                    resultAlias: "jvm.localhost.Threading",
                    obj: "java.lang:type=Threading",
                    attr: ["DaemonThreadCount", "ThreadCount", "PeakThreadCount"],
                },
                self.SdQuery {
                    resultAlias: "jvm.localhost.Memory",
                    obj: "java.lang:type=Memory",
                    attr: ["HeapMemoryUsage", "NonHeapMemoryUsage"],
                },
                self.SdQuery {
                    resultAlias: "jvm.localhost.Runtime",
                    obj: "java.lang:type=Runtime",
                    attr: ["Uptime"],
                },
                self.SdQuery {
                    resultAlias: "jvm.localhost.os",
                    obj: "java.lang:type=OperatingSystem",
                    attr: [
                        "CommittedVirtualMemorySize",
                        "FreePhysicalMemorySize",
                        "FreeSwapSpaceSize",
                        "OpenFileDescriptorCount",
                        "ProcessCpuTime",
                        "SystemLoadAverage",
                    ],
                },
                self.SdQuery {
                    resultAlias: "jvm.localhost.gc",
                    obj: "java.lang:type=GarbageCollector,name=*",
                    attr: ["CollectionCount", "CollectionTime"],
                },
            ],
        },
        jmxLocalhostConfig:: instance.jmxHotspotConfig,
        jmxConfig:: {
            servers: [
                instance.jmxLocalhostConfig,
            ],
        },

        MonitoringLoggingImageMixin:: {
            local monitoring =
                if instance.enableMonitoring then [
                    "curl -s https://dl.google.com/cloudagents/install-monitoring-agent.sh | bash",
                ] else [],
            local jmx =
                if instance.enableJmxMonitoring then [
                    "mkdir -p /opt/jmxtrans/{conf,log}",
                    "curl https://repo.stackdriver.com/jmxtrans/jmxtrans-all.jar -o /opt/jmxtrans/jmxtrans-all.jar",
                    cmd.LiteralFile {
                        to: "/etc/cron.d/jmxtrans",
                        content: "@reboot root /usr/bin/java -Djmxtrans.log.dir=/opt/jmxtrans/log -jar /opt/jmxtrans/jmxtrans-all.jar -j /opt/jmxtrans/conf/ &\n",
                        filePermissions: "700",
                    },
                    cmd.LiteralFile {
                        to: "/opt/jmxtrans/conf/jmx.json",
                        content: std.toString(instance.jmxConfig),
                    },
                ] else [],
            local logging =
                if instance.enableLogging then [
                    "curl -s https://dl.google.com/cloudagents/install-logging-agent.sh | bash",
                ] else [],
            cmds+: monitoring + jmx + logging,
        },

        StandardRootImage:: $.DebianImage + instance.MonitoringLoggingImageMixin,

        allow_stopping_for_update: true,

        tags: [service.fullName],
        metadata: {},
    },


    Service(parent, name): base.Service(parent, name) {
        local service = self,
        infrastructure+: if self.dnsZone == null then {
        } else {
            local instances = if std.objectHas(self, "google_compute_instance") then self.google_compute_instance else {},
            local addresses = if std.objectHas(self, "google_compute_address") then self.google_compute_address else {},
            local DnsRecord = {
                managed_zone: service.dnsZone.nameRef,
                type: "A",
                ttl: 300,
            },
            # Add a record for every address and instance
            google_dns_record_set: {
                [name]: DnsRecord {
                    name: "${google_compute_address." + name + ".name}." + service.dnsZone.dnsName,
                    rrdatas: ["${google_compute_address." + name + ".address}"],
                } for name in std.objectFields(addresses)
            } + {
                [name]: DnsRecord {
                    name: "${google_compute_instance." + name + ".name}." + service.dnsZone.dnsName,
                    rrdatas: ["${google_compute_instance." + name + ".network_interface.0.access_config.0.nat_ip}"],
                    ttl: 5,
                } for name in std.objectFields(instances)
            },
        },
        dnsZone:: null,
    },


    Network(parent, name):: $.Service(parent, name) {
        local service = self,
        nameRef::  "${google_compute_network.%s.name}" % service.fullName,
        infrastructure: {
            google_compute_network: {
                [service.fullName]: {
                    name: service.fullName,
                },
            },
        },
    },


    InstanceBasedService(parent, name): $.Service(parent, name) {
        local service = self,
        fwTcpPorts:: [22],
        fwUdpPorts:: [],
        networkName:: "default",
        perServiceFirewalls:: true,

        Mixin:: {
            networkName: service.networkName,
        },

        Instance:: $.StandardInstance(service) + service.Mixin,

        addressRef:: "${google_compute_address.%s.address}" % service.fullName,

        infrastructure+: {
            google_compute_address: {
                [service.fullName]: { name: service.fullName },
            },
            google_compute_firewall: if !service.perServiceFirewalls then {} else {
                [service.fullName]: {
                    name: service.fullName,
                    source_ranges: ["0.0.0.0/0"],
                    network: service.networkName,
                    allow: [{ protocol: "tcp", ports: [std.toString(p) for p in service.fwTcpPorts] }],
                    target_tags: [self.fullName],
                },
            },
            google_compute_instance: error "InstanceBasedService should define some instances.",
        },
    },


    Cluster3(parent, name): self.InstanceBasedService(parent, name) {
        local service = self,
        lbTcpPorts:: [],
        lbUdpPorts:: [],
        httpHealthCheckPort:: 80,
        zones:: error "Cluster3 version (or service) needs an array of zones.",

        Mixin+: {
            zones:: service.zones,
        },

        versions:: {},
        deployment:: {},
        local instances = std.foldl(function(a, b) a + b, [
            {
                [service.prefixName("%s-%d" % [vname, i])]:
                    if std.objectHas(service.versions, vname) then
                        service.versions[vname] {
                            name: service.prefixName("%s-%d" % [vname, i]),
                            zone: self.zones[i % std.length(self.zones)],
                            tags+: [vname, "index-%d" % i],
                        }
                    else
                        error "Undefined version: %s" % vname
                for i in std.set(service.deployment[vname].deployed)
            }
            for vname in std.objectFields(service.deployment)
        ], {}),
        local attached_instances = std.join(
            [], [
                local attached = std.set(service.deployment[vname].attached);
                local deployed = std.set(service.deployment[vname].deployed);
                [service.prefixName("%s-%d" % [vname, i]) for i in std.setInter(attached, deployed)]
                for vname in std.objectFields(service.deployment)
            ]),

        infrastructure+: {
            google_compute_http_health_check: {
                [service.fullName]: {
                    name: service.fullName,
                    port: service.httpHealthCheckPort,
                },
            },
            google_compute_target_pool: {
                [service.fullName]: {
                    name: service.fullName,
                    depends_on: ["google_compute_http_health_check." + service.fullName],
                    health_checks: [service.fullName],
                    instances: ["%s/%s" % [instances[iname].zone, iname]
                                for iname in attached_instances],
                },
            },
            google_compute_forwarding_rule: {
                [service.prefixName(port)]: {
                    name: service.prefixName(port),
                    ip_address: "${google_compute_address.%s.address}" % service.fullName,
                    target: "${google_compute_target_pool.%s.self_link}" % service.fullName,
                    port_range: port,
                }
                for port in [std.toString(p) for p in service.lbTcpPorts]
            },
            google_compute_instance: instances,
        },
    },


    SingleInstance(parent, name): self.InstanceBasedService(parent, name) {
        local service = self,
        zone:: error "SingleInstance needs a zone.",

        infrastructure+: {
            google_compute_instance: {
                [service.fullName]: service.Instance {
                    name: service.fullName,
                    zone: service.zone,
                    network_interface+: {
                        access_config: {
                            nat_ip: "${google_compute_address.%s.address}" % service.fullName,
                        },
                    },
                },
            },
        },
    },


    DnsZone(parent, name):: self.Service(parent, name) {
        local service = self,
        dnsName:: error "DnsZone must have dnsName, e.g. example.com",
        nameRef:: "${google_dns_managed_zone.%s.name}" % self.fullName,
        description:: "Zone for " + self.dnsName,
        infrastructure+: {
            google_dns_managed_zone: {
                [service.fullName]: {
                    name: service.fullName,
                    dns_name: service.dnsName,
                    description: service.description,
                },
            },
            google_dns_record_set: {
            },
        },
        outputs+: {
            [service.prefixName('name_servers')]: "${google_dns_managed_zone.%s.name_servers.0}" % service.fullName,
        },
    },


    DnsRecordWww(parent, name):: self.Service(parent, name) {
        local service = self,
        dnsName:: service.zone.dnsName,
        zone:: error "DnsRecordWww requires zone.",
        target:: error "DnsRecordWww requires target.",
        infrastructure+: {
            google_dns_record_set: {
                [service.fullName]: {
                    managed_zone: service.zone.nameRef,
                    name: "www." + service.dnsName,
                    type: "CNAME",
                    ttl: 300,
                    rrdatas: [service.target + "." + service.dnsName],
                },
            },
        },
    },
}
