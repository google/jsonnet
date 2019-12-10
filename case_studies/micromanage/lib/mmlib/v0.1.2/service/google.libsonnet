local apt = import '../cmd/apt.libsonnet';
local cmd = import '../cmd/cmd.libsonnet';
local base = import 'base.libsonnet';

{
  // A helper for defining Micromange credentials for a Google environment.
  Credentials:: {
    kind: 'Google',
    project: error "Google credentials must have 'project'",
    // A path to a file on disk where the service account can be found.
    serviceAccount: error "Google credentials must have 'serviceAccount'",
    // Packer uses ssh to run commands on the instance during an image build.
    sshUser: error "Google credentials must have 'sshUser'",
    // Unused.
    region: 'us-central1',
  },


  // A helper for building a Google image with Packer.
  Image:: {

    // The image from which we derive a new image.
    source: error "Google Image must have 'source'",

    // The kind of instane used to build the new image.
    machineType: 'n1-standard-1',

    // The zone in which to deploy the instance that builds the image.
    zone: 'us-central1-f',

    // The commands that are executed to build the new image.
    cmds: [],
  },


  // A higher level image config that lets you add Apt and Pip packages.
  DebianImage:: $.Image + apt.Mixin + apt.PipMixin {
    source: 'debian-9-stretch-v20191121',
  },


  // A base instance config for a GCE instance resource in Terraform.  It sets up:
  // - Default service account scopes.
  // - Default values for the network and machine type.
  // - A default image
  // - Basic system-level monitoring / logging
  // Instances are intended to be created in services, so the service must be provided as an
  // argument here.
  StandardInstance(service):: {
    local instance = self,
    machine_type: 'f1-micro',

    // Either the string 'default' or create a hard dependency on a custom network, use
    // somenetwork.nameRef, where somenetwork evaluates to a service derived from $.Network.
    networkName:: 'default',

    // Commands that are run when the instance is booted for the first time.
    // This is useful for deploying configuration files and other artefacts.
    cmds: [],

    // Commands that are run every boot. This is useful for running processes.
    bootCmds: [],

    // The default network interface has a generated external ip.
    network_interface: {
      network: instance.networkName,
      access_config: {
      },
    },

    // The capabiltiies of the service account whose credentials are available within the instance.
    scopes:: ['cloud-platform'],

    // Use the 'scopes' field as an easier way to override the default service account scopes.
    service_account: [
      {
        scopes: ['https://www.googleapis.com/auth/' + s for s in instance.scopes],
      },
    ],

    // By default, the root disk is built at VM creation time from the StandardRootImage
    // which can be overidden at the top level to e.g. add commands or packages.
    boot_disk: {
      initialize_params: {
        image: instance.StandardRootImage,
      },
    },

    // By default the OS is debian and has support for monitoring and loging agents.
    StandardRootImage:: $.DebianImage + instance.MonitoringLoggingImageMixin,

    supportsLogging:: true,
    supportsMonitoring:: true,
    supportsJmxMonitoring:: true,
    enableLogging:: false,
    enableMonitoring:: false,
    enableJmxMonitoring:: false,
    jmxHost:: 'localhost',
    jmxPort:: 9012,
    jmxHotspotConfig:: {
      host: instance.jmxHost,
      port: instance.jmxPort,
      numQueryThreads: 2,
      SdQuery:: {
        outputWriters: [
          {
            '@class': 'com.googlecode.jmxtrans.model.output.StackdriverWriter',
            settings: {
              token: 'STACKDRIVER_API_KEY',
              detectInstance: 'GCE',
              url: 'https://jmx-gateway.google.stackdriver.com/v1/custom',
            },
          },
        ],
      },
      queries: [
        self.SdQuery {
          resultAlias: 'jvm.localhost.Threading',
          obj: 'java.lang:type=Threading',
          attr: ['DaemonThreadCount', 'ThreadCount', 'PeakThreadCount'],
        },
        self.SdQuery {
          resultAlias: 'jvm.localhost.Memory',
          obj: 'java.lang:type=Memory',
          attr: ['HeapMemoryUsage', 'NonHeapMemoryUsage'],
        },
        self.SdQuery {
          resultAlias: 'jvm.localhost.Runtime',
          obj: 'java.lang:type=Runtime',
          attr: ['Uptime'],
        },
        self.SdQuery {
          resultAlias: 'jvm.localhost.os',
          obj: 'java.lang:type=OperatingSystem',
          attr: [
            'CommittedVirtualMemorySize',
            'FreePhysicalMemorySize',
            'FreeSwapSpaceSize',
            'OpenFileDescriptorCount',
            'ProcessCpuTime',
            'SystemLoadAverage',
          ],
        },
        self.SdQuery {
          resultAlias: 'jvm.localhost.gc',
          obj: 'java.lang:type=GarbageCollector,name=*',
          attr: ['CollectionCount', 'CollectionTime'],
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
          'curl -s https://dl.google.com/cloudagents/install-monitoring-agent.sh | bash',
        ] else [],
      local jmx =
        if instance.enableJmxMonitoring then [
          'mkdir -p /opt/jmxtrans/{conf,log}',
          'curl https://repo.stackdriver.com/jmxtrans/jmxtrans-all.jar -o /opt/jmxtrans/jmxtrans-all.jar',
          cmd.LiteralFile {
            to: '/etc/cron.d/jmxtrans',
            content: '@reboot root /usr/bin/java -Djmxtrans.log.dir=/opt/jmxtrans/log -jar /opt/jmxtrans/jmxtrans-all.jar -j /opt/jmxtrans/conf/ &\n',
            filePermissions: '700',
          },
          cmd.LiteralFile {
            to: '/opt/jmxtrans/conf/jmx.json',
            content: std.toString(instance.jmxConfig),
          },
        ] else [],
      local logging =
        if instance.enableLogging then [
          'curl -s https://dl.google.com/cloudagents/install-logging-agent.sh | bash',
        ] else [],
      cmds+: monitoring + jmx + logging,
    },

    // This Terraform policy allows changing some properties by stopping the instance.
    allow_stopping_for_update: true,

    // By default, the only tag is the name of the service owning the instance.
    tags: [service.fullName],

    // By default there is no metadata (startup scripts are managed by micromanage so do not appear
    // here.
    metadata: {},
  },


  // The base class of all Google services.  This provides support for automatic but optional
  // creation of DNS names for any instances or address resources with external IPs.
  Service(outer, name): base.Service(outer, name) {
    local service = self,
    infrastructure+: if self.dnsZone == null then {
      // Don't do anything if we have no DNS zone.
    } else {
      local infra = self,
      local DnsRecord = {
        managed_zone: service.dnsZone.nameRef,
        type: 'A',
        ttl: 300,
      },
      // Ensure the fields exist, since we access them below.
      google_compute_instance+: { },
      google_compute_address+: { },
      // Add a record for every address and instance
      google_dns_record_set+: {
        [name]: DnsRecord {
          name: '${google_compute_instance.' + name + '.name}.' + service.dnsZone.dnsName,
          rrdatas: [
            '${google_compute_instance.' + name + '.network_interface.0.access_config.0.nat_ip}',
          ],
          // Use a short TTL for instance addresses, because instances churn a fair amount and the
          // majority of traffic does not go to specific instances but load balancer frontends.
          ttl: 5,
        }
        for name in std.objectFields(infra.google_compute_instance)
      } + {
        // Note that if an address and an instance has the same name, the domain will go to the
        // address's IP, not the instance's IP.
        [name]: DnsRecord {
          name: '${google_compute_address.' + name + '.name}.' + service.dnsZone.dnsName,
          rrdatas: ['${google_compute_address.' + name + '.address}'],
        }
        for name in std.objectFields(infra.google_compute_address)
      },
    },

    // A reference to a service which is derived from $.DnsZone, or null.
    dnsZone:: null,
  },


  // A custom network into which other services can be deployed.  To put a resource in this network
  // use the .nameRef field, which tells Terraform to create the network first.
  Network(outer, name):: $.Service(outer, name) {
    local service = self,
    nameRef:: '${google_compute_network.%s.name}' % service.fullName,
    infrastructure: {
      google_compute_network: {
        [service.fullName]: {
          name: service.fullName,
        },
      },
    },
  },


  // A base class for a service whose compute facility is provided by instances.  This supports:
  // - Optional automatic creation of firewall rules that assume the instances in this service
  // have the service's full name as a tag (by default, this is the case).
  // - Creation of a reserved address for the front end of the service, which might be a load
  // balancer or a VM.
  InstanceBasedService(outer, name): $.Service(outer, name) {
    local service = self,

    // The firewall rule generation can be turned off here.  This is useful if your firewalls
    // are centrally managed using tags.
    perServiceFirewalls:: true,
    // Ports to allow through the firewall.
    fwTcpPorts:: [22],
    fwUdpPorts:: [],

    // Either the string 'default' or create a hard dependency on a custom network, use
    // somenetwork.nameRef, where somenetwork evaluates to a service derived from $.Network.
    networkName:: 'default',

    // A base class for the instances that will be used in this service.  This can be extended
    // to customise the instances.  The networkName is conveniently populated since it must
    // be the same for all instances in the service.
    //
    // Note that InstanceBasedService does not actually create any instances, it's up to sub-classes
    // to do that.  But they can use (and extend) this onfiguration when they do so.
    Instance:: $.StandardInstance(service) {
      networkName: service.networkName,
    },

    // Use this when you depend on this service to access the generated IP address.  This creates
    // an ordering dependency in Terraform because the IP address is not known until deployment
    // of this service has begun.
    addressRef:: '${google_compute_address.%s.address}' % service.fullName,

    infrastructure+: {
      google_compute_address: {
        // The single frontend IP for this service.
        [service.fullName]: { name: service.fullName },
      },
      google_compute_firewall: if !service.perServiceFirewalls then {} else {
        [service.fullName]: {
          name: service.fullName,
          source_ranges: ['0.0.0.0/0'],
          network: service.networkName,
          allow: [{ protocol: 'tcp', ports: [std.toString(p) for p in service.fwTcpPorts] }],
          target_tags: [service.fullName],
        },
      },
      google_compute_instance: error 'InstanceBasedService should define some instances.',
    },
  },


  // A service that is provided by many instances behind a network load balancer.  This supports:
  // - Automatic creation of the loadbalancer resources on whatever ports you need and bound to the
  // reserved IP address from $.InstanceBasedService (referencable with .addressRef).
  // - Health checking for the network load balancer.
  // - Management of a set of VMs across a range of zones (round robin).
  // - The facility to "Drain" VMs on demand by removing them from the load balancer.
  // - Canarying new instance configurations or blue / green deployment strategies.
  Cluster3(outer, name): self.InstanceBasedService(outer, name) {
    local service = self,
    lbTcpPorts:: [],
    lbUdpPorts:: [],
    httpHealthCheckPort:: 80,
    zones:: error 'Cluster3 version (or service) needs an array of zones.',

    Instance+: {
      // Creating a layer of indirection through the instance
      zones:: service.zones,
    },

    // This is a map from version code to instance configuration.  These are the versions that may
    // be deployed.
    versions:: {},

    // Which versions are deployed.  This is a map from version code to an object with fields
    // 'deployed' and 'attached'.  Those must hold arrays of integers, indicating which VM indexes
    // within that version are deployed or attached.  The instance names are generated as so:
    // ${service_name}-${version_name}-${index}
    deployment:: {},

    local instances = std.foldl(function(a, b) a + b, [
      {
        [service.prefixName('%s-%d' % [vname, i])]:
          if std.objectHas(service.versions, vname) then
            service.versions[vname] {
              name: service.prefixName('%s-%d' % [vname, i]),
              zone: self.zones[i % std.length(self.zones)],
              tags+: [vname, 'index-%d' % i],
            }
          else
            error 'Undefined version: %s' % vname
        for i in std.set(service.deployment[vname].deployed)
      }
      for vname in std.objectFields(service.deployment)
    ], {}),

    local attached_instances = std.join(
      [], [
        local attached = std.set(service.deployment[vname].attached);
        local deployed = std.set(service.deployment[vname].deployed);
        [service.prefixName('%s-%d' % [vname, i]) for i in std.setInter(attached, deployed)]
        for vname in std.objectFields(service.deployment)
      ]
    ),

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
          depends_on: ['google_compute_http_health_check.' + service.fullName],
          health_checks: [service.fullName],
          instances: [
            '%s/%s' % [instances[iname].zone, iname]
            for iname in attached_instances
          ],
        },
      },
      google_compute_forwarding_rule: {
        [service.prefixName(port)]: {
          name: service.prefixName(port),
          ip_address: '${google_compute_address.%s.address}' % service.fullName,
          target: '${google_compute_target_pool.%s.self_link}' % service.fullName,
          port_range: port,
        }
        for port in [std.toString(p) for p in service.lbTcpPorts]
      },
      google_compute_instance: instances,
    },

    outputs+: {
      [service.prefixName('addr')]: service.addressRef,
    },
  },


  // A service that is provided by a single instance.  This is less reliable than $.Cluster3 but is
  // useful for testing or other non-highly available services.  It supports:
  // - Management of the single instance on the frontend address created by $.InstanceBasedService.
  // This IP does not change when the instance is rebuilt.
  SingleInstance(outer, name): self.InstanceBasedService(outer, name) {
    local service = self,
    // Control the zone at the top-level.
    zone:: error 'SingleInstance needs a zone.',

    Instance+: {
      zone: service.zone,
      network_interface+: {
        access_config: {
          // Use the service's predefined IP address.
          nat_ip: '${google_compute_address.%s.address}' % service.fullName,
        },
      },
    },

    infrastructure+: {
      google_compute_instance: {
        [service.fullName]: service.Instance { name: service.fullName, },
      },
    },

    outputs+: {
      [service.prefixName('addr')]: service.addressRef,
    },
  },


  // A service that manages a DNS zone, which can then be used by other services to expose their
  // addresses via DNS.  The DNS zone should be recreated rarely, because its domain name servers
  // will change and then upstream NS record will need to be updated.  However, the records within
  // the zone can change at any time as the set of mapped IP addresses changes.
  DnsZone(outer, name):: self.Service(outer, name) {
    local service = self,
    // The suffix that some authority will delegate to the generated nameserves via a NS record.
    // All domain names managed by this zone will have the same suffix.
    dnsName:: error 'DnsZone must have dnsName, e.g. example.com',

    // Use this reference to ensure Terraform deploys the DNS zone before services that need to
    // register domain names in it.
    nameRef:: '${google_dns_managed_zone.%s.name}' % self.fullName,

    // A convenient field to override the description in the underlying resource.
    description:: 'Zone for ' + self.dnsName,

    infrastructure+: {
      google_dns_managed_zone: {
        [service.fullName]: {
          name: service.fullName,
          dns_name: service.dnsName,
          description: service.description,
        },
      },
    },

    outputs+: {
      // The first of the set of nameservers, which can be used to configure the upstream authority
      // to delegate to the new DNS zone via an NS record.
      [service.prefixName('name_servers')]:
        '${google_dns_managed_zone.%s.name_servers.0}' % service.fullName,
    },
  },


  // A simple service that creates a domain name alias from www.foo to target.foo, where target is
  // the front end IP of some other service.  This allows designating the front end of your
  // application, and allowing it to be accessed via www.foo instead of servicename.foo.
  DnsRecordWww(outer, name):: self.Service(outer, name) {
    local service = self,
    // A service derived from $.DnsZone.
    zone:: error 'DnsRecordWww requires zone.',

    // The name of the service within the zone, e.g. 'servicename' without the '.foo' suffix.
    target:: error 'DnsRecordWww requires target.',

    infrastructure+: {
      google_dns_record_set: {
        [service.fullName]: {
          managed_zone: service.zone.nameRef,
          name: 'www.' + service.zone.dnsName,
          type: 'CNAME',
          ttl: 300,
          rrdatas: [service.target + '.' + service.zone.dnsName],
        },
      },
    },
  },
}
