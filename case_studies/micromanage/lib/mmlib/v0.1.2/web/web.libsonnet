{
  // A mixin on top of an InstanceBasedService that turns it into an HTTP and/or HTTPs service.
  // This is only really suitable for SingleInstance services because it does not configure load
  // balancing. For that, use HttpService3.
  HttpSingleInstance: {
    local service = self,

    // You must have either an httpPort or an httpsPort or both.
    assert service.httpPort != null || service.httpsPort != null,

    // HTTPs requires SSL certificates which need DNS.
    assert service.httpsPort == null || service.dnsZone != null : 'HTTPs requires DNS.',

    // Set this to null for an https only service.  However that is not compatible with
    // health checking so may not be a good idea.
    httpPort:: 80,

    // Enable HTTPs here.  Use importstr if your certificate and keys are files on disk.
    httpsPort:: null,
    sslCertificate:: error 'sslCertificate field was required but not present.',
    sslCertificateKey:: error 'sslCertificateKey field was required but not present.',

    // Ensure the firewall permits traffic to the ports.
    fwTcpPorts+: (if self.httpPort != null then [self.httpPort] else [])
                 + (if self.httpsPort != null then [self.httpsPort] else []),

    // A useful way to determine the URL where this service can be accessed.
    httpEndpoint::
      if service.httpPort == 80 then
        'http://%s' % [service.addressRef]
      else
        'http://%s:%d' % [service.addressRef, service.httpPort],

    // A useful way to determine the URL where this service can be accessed.
    httpsEndpoint::
      if service.httpsPort == 443 then
        'https://%s.%s' % [service.fullName, service.dnsZone.dnsName]
      else
        'https://%s.%s:%d' % [service.fullName, service.dnsZone.dnsName, service.httpsPort],

    outputs: {
      [if service.httpPort != null then service.prefixName('http-endpoint')]:
        service.httpEndpoint,
      [if service.httpsPort != null then service.prefixName('https-endpoint')]:
        service.httpsEndpoint,
    },

    // Adding the http-server and https-server tags makes the instances comptible with GCE's
    // default firewall configuration, if that is being used.
    Instance+: {
      tags+:
        (if service.httpPort != null then ['http-server'] else [])
        + (if service.httpsPort != null then ['https-server'] else []),
    },
  },


  // A mixin on top of a cluster3 service that turns it into an HTTP service.
  HttpService3: self.HttpSingleInstance {
    // Ensure the correct ports are forwarded in.
    lbTcpPorts+: (if self.httpPort != null then [self.httpPort] else [])
                 + (if self.httpsPort != null then [self.httpsPort] else []),

    // Prefer http port for health checking unless only https is available.
    httpHealthCheckPort: if self.httpPort != null then self.httpPort else self.httpsPort,
  },

}
