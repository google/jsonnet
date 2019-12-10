local amis_debian = import '../amis/debian.libsonnet';
local apt = import '../cmd/apt.libsonnet';
local cmd = import '../cmd/cmd.libsonnet';
local base = import 'base.libsonnet';

{
  Credentials:: {
    kind: 'Amazon',
    accessKey: error "Amazon credentials must have 'accessKey'",
    secretKey: error "Amazon credentials must have 'secretKey'",
    region: 'us-east-1',
  },


  Image:: {
    sourceAmi: error "Amazon AMI must have 'sourceAmi'",
    instanceType: 't2.small',
    sshUser: error "Amazon AMI must have 'sshUser'",
    cmds: [],
  },


  DebianImage:: $.Image + apt.Mixin + apt.PipMixin {
    sourceAmi: amis_debian.wheezy.amd64['20150128']['us-west-1'],
    sshUser: 'admin',
  },


  StandardInstance:: {
    local instance = self,
    instance_type: 't2.small',
    ami: instance.StandardRootImage,
    associate_public_ip_address: true,
    cmds: [],
    bootCmds: [],
    tags: {},
    // TODO(dcunnin): Figure out an equivalent here.
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


  Service(parent, name):: base.Service(parent, name) {
    // TODO: Automatic registration of domain names for instances and addresses.
  },


  Network(parent, name):: $.Service(parent, name) {
    local service = self,
    idRef:: '${aws_vpc.%s.id}' % service.fullName,
    nameRef:: '${aws_vpc.%s.name}' % service.fullName,
    subnetIdRef(zone):: '${aws_subnet.%s-%s.id}' % [service.fullName, zone],
    ipv4Range:: '10.0.0.0/16',
    infrastructure: {
      aws_vpc: {
        [service.fullName]: {
          cidr_block: service.ipv4Range,
        },
      },
      aws_internet_gateway: {
        [service.fullName]: {
          vpc_id: '${aws_vpc.%s.id}' % service.fullName,
        },
      },
      aws_route_table: {
        [service.fullName]: {
          vpc_id: '${aws_vpc.%s.id}' % service.fullName,
          route: {
            cidr_block: '0.0.0.0/0',
            gateway_id: '${aws_internet_gateway.%s.id}' % service.fullName,
          },
        },
      },
      aws_subnet: {
        [service.prefixName(zone)]: {
          vpc_id: '${aws_vpc.%s.id}' % service.fullName,
          cidr_block: service.subnets[zone],
          availability_zone: zone,
        }
        for zone in std.objectFields(service.subnets)
      },
      aws_route_table_association: {
        [service.prefixName(zone)]: {
          subnet_id: '${aws_subnet.%s.id}' % service.prefixName(zone),
          route_table_id: '${aws_route_table.%s.id}' % service.fullName,
        }
        for zone in std.objectFields(service.subnets)
      },
    },
    // Maps zone to CIDR.
    subnets:: {},
  },


  InstanceBasedService(parent, name): $.Service(parent, name) {
    local service = self,
    fwTcpPorts:: [22],
    fwUdpPorts:: [],
    networkName:: null,
    keyName:: error 'InstanceBasedService needs keyName',

    Mixin:: (
      if service.networkName != null then {
        vpc_security_group_ids: ['${aws_security_group.%s.id}' % service.fullName],
        subnet_id: $.Network.subnetIdRef(self.availability_zone),
      } else {
        security_groups: ['${aws_security_group.%s.name}' % service.fullName],
      }
    ) + {
      [if service.keyName != null then 'key_name']: service.keyName,
    },

    Instance:: $.StandardInstance + service.Mixin,

    infrastructure+: {
      aws_security_group: {
        [service.fullName]: {
          name: service.fullName,

          local IngressRule(p, protocol) = {
            from_port: p,
            to_port: p,
            protocol: protocol,
            cidr_blocks: ['0.0.0.0/0'],
          },
          ingress: [IngressRule(p, 'tcp') for p in service.fwTcpPorts]
                   + [IngressRule(p, 'udp') for p in service.fwUdpPorts],

          [if service.networkName != null then 'vpc_id']: $.Network.idRef,
        },
      },
      aws_instance: error 'InstanceBasedService should define some instances.',
    },
  },


  Cluster3(parent, name): $.InstanceBasedService(parent, name) {
    local service = self,
    lbTcpPorts:: [],
    lbUdpPorts:: [],
    fwTcpPorts:: [22],
    fwUdpPorts:: [],
    httpHealthCheckPort:: 80,
    zones:: error 'Cluster3 version (or service) needs an array of zones.',

    // In this service, the 'instance' is really an instance template used to create
    // a pool of instances.
    Mixin:: {
      zones:: service.zones,
    },
    versions:: {},
    deployment:: {},
    local merge(objs) = std.foldl(function(a, b) a + b, objs, {}),
    local instances = merge([
      {
        [service.prefixName('%s-%d' % [vname, i])]:
          if std.objectHas(service.versions, vname) then
            service.versions[vname] {
              availability_zone: self.zones[i % std.length(self.zones)],
              tags+: {
                version: vname,
                index: i,
              },
            }
          else
            error 'Undefined version: %s' % vname
        for i in std.set(service.deployment[vname].deployed)
      }
      for vname in std.objectFields(service.deployment)
    ]),
    local attached_instances = std.join([], [
      local attached = std.set(service.deployment[vname].attached);
      local deployed = std.set(service.deployment[vname].deployed);
      [service.prefixName('%s-%d' % [vname, i]) for i in std.setInter(attached, deployed)]
      for vname in std.objectFields(service.deployment)
    ]),

    addressRef:: '${aws_elb.%s.dns_name}' % service.fullName,

    infrastructure+: {
      aws_instance: instances,

      aws_elb: {
        [service.fullName]: {
          name: service.fullName,

          availability_zones: service.zones,

          listener: [{
            instance_port: p,
            instance_protocol: 'tcp',
            lb_port: p,
            lb_protocol: 'tcp',
          } for p in service.lbTcpPorts],

          health_check: {
            healthy_threshold: 2,
            unhealthy_threshold: 2,
            timeout: 3,
            target: 'HTTP:%d/' % service.httpHealthCheckPort,
            interval: 5,
          },

          instances: ['${aws_instance.%s.id}' % iname for iname in attached_instances],
          cross_zone_load_balancing: true,
          idle_timeout: 60,
          connection_draining: true,
          connection_draining_timeout: 60,
        },
      },
      aws_security_group: {
        [service.prefixName('elb')]: {
          name: service.prefixName('elb'),

          local IngressRule(p, protocol) = {
            from_port: p,
            to_port: p,
            protocol: protocol,
            cidr_blocks: ['0.0.0.0/0'],
          },
          ingress: [IngressRule(p, 'tcp') for p in service.lbTcpPorts]
                   + [IngressRule(p, 'udp') for p in service.lbUdpPorts],

          //[if service.networkName != null then "vpc_id"]: $.Network.idRef,
        },
      },
    },
  },


  SingleInstance(parent, name): $.InstanceBasedService(parent, name) {
    local service = self,
    zone:: error 'SingleInstance needs a zone.',

    addressRef:: '${aws_instance.%s.public_ip}' % service.fullName,

    infrastructure+: {
      aws_instance: {
        [service.fullName]: service.Instance {
          availability_zone: service.zone,
        },
      },
    },
  },

  DnsZone(parent, name):: self.Service(parent, name) {
    local service = self,
    dnsName:: error 'DnsZone must have dnsName, e.g. example.com',
    nameRef:: '${aws_route53_zone.%s.zone_id}' % service.fullName,
    description:: 'Zone for ' + self.dnsName,
    infrastructure+: {
      aws_route53_zone: {
        [service.fullName]: {
          dns_name: service.dnsName,
          comment: service.description,
        },
      },
      aws_dns_record: {
      },
    },
    outputs+: {
      [service.prefixName('name_servers')]:
        '${google_dns_managed_zone.%s.name_servers.0}' % service.fullName,
    },
  },

}
