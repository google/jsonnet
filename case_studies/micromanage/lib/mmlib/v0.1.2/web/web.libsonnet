{
    /** A mixin on top of an InstanceBasedService that turns it into an HTTP and/or HTTPs service. */
    HttpSingleInstance: {
        local service = self,
        assert service.httpPort != null || service.httpsPort != null,
        assert service.httpsPort == null || service.dnsZone != null : 'HTTPs requires DNS.',
        httpPort:: 80,
        httpsPort:: null,
        sslCertificate:: error 'sslCertificate field was required but not present.',
        sslCertificateKey:: error 'sslCertificateKey field was required but not present.',
        fwTcpPorts+: (if self.httpPort != null then [self.httpPort] else [])
                     + (if self.httpsPort != null then [self.httpsPort] else []),
        httpEndpoint::
            if service.httpPort == 80 then
                "http://%s" % [service.addressRef]
            else
                "http://%s:%d" % [service.addressRef, service.httpPort],

        httpsEndpoint::
            if service.httpsPort == 443 then
                 "https://%s.%s" % [service.fullName, service.dnsZone.dnsName]
            else
                 "https://%s.%s:%d" % [service.fullName, service.dnsZone.dnsName, service.httpsPort],

        outputs: {
            [if service.httpPort != null then service.prefixName("http-endpoint")]:
                service.httpEndpoint,
            [if service.httpsPort != null then service.prefixName("https-endpoint")]:
                service.httpsEndpoint,
        },
        Instance+: {
            tags+: 
                (if service.httpPort != null then ['http-server'] else [])
                + (if service.httpsPort != null then ['https-server'] else []),
        }
    },

    /** A mixin on top of a cluster3 service that turns it into an HTTP service. */
    HttpService3: self.HttpSingleInstance {
        lbTcpPorts+: (if self.httpPort != null then [self.httpPort] else [])
                     + (if self.httpsPort != null then [self.httpsPort] else []),

        // Prefer http port for health checking unless only https is available.
        httpHealthCheckPort: if self.httpPort != null then self.httpPort else self.httpsPort,
    },

}
