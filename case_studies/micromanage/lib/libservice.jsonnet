local libimgcmd = import "libimgcmd.jsonnet";
local libos = import "libos.jsonnet";

{
    GcpCredentials:: {
        kind: "Google",
        project: error "Google credentials must have 'project'",
        serviceAccount: error "Google credentials must have 'serviceAccount'",
        sshUser: error "Google credentials must have 'sshUser'",
        region: "us-central1",
    },

    GcpImage:: {
        source: error "GcpImage must have 'source'",
        machine_type: "n1-standard-1",
        zone: "us-central1-f",
        cmds: [],
    },

    GcpStandardInstance:: {
        local instance = self,
        machine_type: "f1-micro",
        scopes:: ["devstorage.read_only", "logging.write"],
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
            }
        ],      
        disk: [
            {   
                image: instance.StandardRootImage,
            }   
        ],      

        enableLogging:: false,
        enableMonitoring:: false,
        enableJmxMonitoring:: false,
        jmxHost:: "localhost",
        jmxPort:: 9012,
        jmxHotspotConfig:: {
            host: instance.jmxHost,
            port : instance.jmxPort,
            numQueryThreads : 2,
            SdQuery:: {
                outputWriters: [
                  {
                     "@class" : "com.googlecode.jmxtrans.model.output.StackdriverWriter",
                     settings: {
                        token: "STACKDRIVER_API_KEY",
                        detectInstance: "GCE",
                        url: "https://jmx-gateway.google.stackdriver.com/v1/custom"
                     }
                  }
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
                    resultAlias : "jvm.localhost.os",
                    obj : "java.lang:type=OperatingSystem",
                    attr: [
                        "CommittedVirtualMemorySize",
                        "FreePhysicalMemorySize",
                        "FreeSwapSpaceSize",
                        "OpenFileDescriptorCount",
                        "ProcessCpuTime",
                        "SystemLoadAverage",
                    ]
                },
                self.SdQuery {
                    resultAlias: "jvm.localhost.gc",
                    obj: "java.lang:type=GarbageCollector,name=*",
                    attr: ["CollectionCount", "CollectionTime"],
                }
            ],
        },
        jmxLocalhostConfig:: instance.jmxHotspotConfig,
        jmxConfig:: {
            servers: [
                instance.jmxLocalhostConfig,
            ]
        },

        MonitoringLoggingImageMixin:: {
            local monitoring =
                if instance.enableMonitoring then [
                    "curl -s https://repo.stackdriver.com/stack-install.sh | bash",
                ] else [],
            local jmx =
                if instance.enableJmxMonitoring then [
                    "mkdir -p /opt/jmxtrans/{conf,log}",
                    "curl https://repo.stackdriver.com/jmxtrans/jmxtrans-all.jar -o /opt/jmxtrans/jmxtrans-all.jar",
                    libimgcmd.LiteralFile {
                        to: "/etc/cron.d/jmxtrans",
                        content: "@reboot root /usr/bin/java -Djmxtrans.log.dir=/opt/jmxtrans/log -jar /opt/jmxtrans/jmxtrans-all.jar -j /opt/jmxtrans/conf/ &\n",
                        filePermissions: "700",
                    },
                    libimgcmd.LiteralFile {
                        to: "/opt/jmxtrans/conf/jmx.json",
                        content: std.toString(instance.jmxConfig),
                    },
                ] else [],
            local logging =
                if instance.enableLogging then [
                    "curl -s https://storage.googleapis.com/signals-agents/logging/google-fluentd-install.sh | bash",
                ] else [],
            cmds+: monitoring + jmx + logging,
        },

        StandardRootImage:: $.GcpImage + libos.GcpDebianMixin + instance.MonitoringLoggingImageMixin,

        tags: ["${-}"],
        metadata: {},
    },

    GcpService: {
        local service = self,
        kind: "Google",
        children: {},
        dnsSuffix:: error "No dnsSuffix given.",
        infrastructure: if self.dnsZone == null then {
        } else {
            local instances = if std.objectHas(self, "google_compute_instance") then self.google_compute_instance else { },
            local addresses = if std.objectHas(self, "google_compute_address") then self.google_compute_address else { },
            local DnsRecord = {
                depends_on: ["google_dns_managed_zone." + service.dnsZone],
                managed_zone: service.dnsZone,
                type: "A",
                ttl: 300,
            },
            # Add a record for every address and instance
            google_dns_record_set: {
                [name]: DnsRecord {
                    name: "${google_compute_address." + name + ".name}." + service.dnsSuffix,
                    rrdatas: ["${google_compute_address." + name + ".address}"],
                } for name in std.objectFields(addresses)
            } + {
                [name]: DnsRecord {
                    name: "${google_compute_instance." + name + ".name}." + service.dnsSuffix,
                    rrdatas: ["${google_compute_instance." + name + ".network_interface.0.access_config.0.nat_ip}"],
                } for name in std.objectFields(instances)
            },
        },
        outputs: {},
        dnsZone:: null,
        
    },

    GcpCluster3: self.GcpService {
        local service = self,
        lbTcpPorts:: [],
        lbUdpPorts:: [],
        fwTcpPorts:: [22],
        fwUdpPorts:: [],
        httpHealthCheckPort:: 80,
        networkName:: "default",
        zones:: error "GcpCluster3 version (or service) needs an array of zones.",

        StandardVersion:: $.GcpStandardInstance {
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
            google_compute_address: {
                "${-}": { name: "${-}" }
            },
            google_compute_http_health_check: {
                "${-}": {
                    name: "${-}",
                    port: service.httpHealthCheckPort,
                },
            },
            google_compute_target_pool: {
                "${-}": {
                    name: "${-}",
                    depends_on: "google_compute_http_health_check.${-}",
                    health_checks: ["${-}"],
                    instances: ["%s/%s" % [instances[iname].zone, iname]
                                for iname in attached_instances],
                },
            },
            google_compute_forwarding_rule: {
                ["${-}-%s" % port]: {
                    name: "${-}-%s" % port,
                    ip_address: "${google_compute_address.${-}.address}",
                    target: "${google_compute_target_pool.${-}.self_link}",
                    port_range: port,
                }
                for port in [std.toString(p) for p in service.lbTcpPorts]
            },
            google_compute_firewall: {
                "${-}": {
                    name: "${-}",
                    source_ranges: ["0.0.0.0/0"],
                    network: service.networkName,
                    allow: [{ protocol: "tcp", ports: [std.toString(p) for p in service.fwTcpPorts]}],
                    target_tags: ["${-}"],
                }
            },
            google_compute_instance: instances,
        },
    },

    UniCluster(zone):: {
        local service = self,
        versions: {
            uni: service.StandardVersion {
                zones: [zone],
            },
        },
        deployment: {
            uni: {
                deployed: [1],
                attached: [1],
            },
        },
    },

    GcpZone:: {
        local service = self,
        kind: "Google",
        children: {},
        dnsName:: error "GcpZone must have dnsName, e.g. example.com",
        description:: "Zone for " + self.dnsName,
        infrastructure: {
            google_dns_managed_zone: {
                "${-}": {
                    name: "${-}",
                    dns_name: service.dnsName,
                    description: service.description,
                },
            },
            google_dns_record_set: {
            }
        },
        outputs: {
            "${-}-name_servers": "${google_dns_managed_zone.${-}.name_servers.0}",
        },
    }
}
