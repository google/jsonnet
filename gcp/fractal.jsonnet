/*
Copyright 2014 Google Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

local gcp = import "lib/gcp.jsonnet";

local bucket = "fermata-bucket",
    width = "400",
    height = "400",
    iterations = "120",
    backend_port = "8000",
    num_backends = 4;

local region = "us-central1",
    zones = [region+suffix for suffix in ["-a", "-b"]];

local backends = [
    {name: "backend"+i, zone: zones[i%2]}
    for i in std.range(1, num_backends)
];

{
    fractal_ip: gcp.Address {region: region},
    fractal_network: gcp.Network,
    fractal_firewall: gcp.Firewall {
        network: "fractal_network",
        allowed: [{IPProtocol: "tcp", ports: ["22", backend_port]}],
    },
    fractal_pool: gcp.TargetPool {
        region: region,
        instances: [backend.name for backend in backends],
    },
    fractal_lb: gcp.ForwardingRule {
        region: region,
        IPAddress: "fractal_ip",
        portRange: backend_port,
        target: "fractal_pool",
    },
    fractal_frontend: gcp.AppEngine {
        source: "frontend",
        environmentVariables: {
            FRACTAL_WIDTH: width,
            FRACTAL_HEIGHT: height,
            FRACTAL_BACKEND_PORT: backend_port,
        },
    },
} + {
    [backend.name]: gcp.SimpleInstance {
        bootDiskName: backend.name,
        zone: backend.zone,
        image: "luaimg",
        network: "fractal_network",
        packages: {
            image_server: {
                uri: "gs://" + bucket + "/backend.tar.gz",
            },
        },
        binary: {
            package: "image_server",
            bin: "backend/serve.py",
        },
        environmentVariables: {
            FRACTAL_WIDTH: width,
            FRACTAL_HEIGHT: height,
            FRACTAL_ITERATIONS: iterations,
            FRACTAL_BUCKET: bucket,
        },
    }
    for backend in backends
}
