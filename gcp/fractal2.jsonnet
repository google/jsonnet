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

{
    local gcp = import "lib/gcp.jsonnet",
    backend_port:: "8000",
    width:: "400",
    height:: "400",
    bucket:: "fermata-bucket",

    fractal_ip: gcp.Address { region: "us-central1", },
    fractal_network: gcp.Network,
    fractal_firewall: gcp.Firewall {
        network: "fractal_network",
        allowed: [{IPProtocol: "tcp", ports: ["22", $.backend_port]}],
    },
    fractal_pool: gcp.TargetPool {
        region: $.fractal_ip.region,
        instances: ["backend1", "backend2", "backend3", "backend4"],
    },
    fractal_lb: gcp.ForwardingRule {
        region: $.fractal_ip.region,
        IPAddress: "fractal_ip",
        portRange: $.backend_port,
        target: "fractal_pool",
    },

    fractal_frontend: gcp.AppEngine {
        source: "frontend",
        environmentVariables: {
            FRACTAL_WIDTH: $.width,
            FRACTAL_HEIGHT: $.height,
            FRACTAL_BACKEND_PORT: $.backend_port,
        },
    },

    backend1: gcp.SimpleInstance {
        zone: "us-central1-b",
        bootDiskName: "backend1",
        image: "luaimg",
        network: "fractal_network",
        packages: {
            image_server: {
                uri: "gs://" + $.bucket + "/backend.tar.gz"
            },
        },
        binary: {
            package: "image_server",
            bin: "backend/serve.py",
        },
        environmentVariables: {
            FRACTAL_WIDTH: $.width,
            FRACTAL_HEIGHT: $.height,
            FRACTAL_ITERATIONS: "120",
            FRACTAL_BUCKET: $.bucket,
        },
    },
    backend2: $.backend1 {
        zone: "us-central1-a",
        bootDiskName: "backend2",
    },
    backend3: $.backend1 {
        bootDiskName: "backend3",
    },
    backend4: $.backend2 {
        bootDiskName: "backend4",
    },
}
