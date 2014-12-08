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

local packer = import "packer.jsonnet";
local terraform = import "terraform.jsonnet";
local cassandra = import "cassandra.jsonnet";

{
    project:: error "You must override abstract_service and provide: project",
    bucket:: error "You must override abstract_service and provide: bucket",

    cassandraRootPass:: error "You must override abstract_service and provide: cassandraRootPass",
    cassandraUserPass:: error "You must override abstract_service and provide: cassandraUserPass",
    cassandraUser:: "fractal",
    cassandraKeyspace:: "fractal",

    cassandraNodes:: ["db1", "db2", "db3", "db4", "db5"],
    cassandraReplication:: "{ 'class' : 'SimpleStrategy', 'replication_factor' : 2 }",

    CassandraConf:: cassandra.conf {
        cluster_name: "Fractal Cluster",
        rpc_address:: null,  // Unset by making it hidden (::).
        listen_address:: null,  // Unset by making it hidden (::).
        authenticator: "PasswordAuthenticator",
        seed_provider: [
            {
                class_name: "org.apache.cassandra.locator.SimpleSeedProvider",
                parameters: [ { seeds: std.join(", ", $.cassandraNodes) } ],
            },
        ],
    },

    cassandraInitCql:: [
        "CREATE USER %s WITH PASSWORD '%s';" % [$.cassandraUser, $.cassandraUserPass],
        local rep1 =  "{ 'class' : 'SimpleStrategy', 'replication_factor' : 1 }";
            "CREATE KEYSPACE %s WITH REPLICATION = %s;" % [$.cassandraKeyspace, rep1],
        "USE %s;" % $.cassandraKeyspace,
        "CREATE TABLE discoveries("
        + "Date TEXT, TimeId TIMEUUID, Text TEXT, X FLOAT, Y FLOAT, L INT, "
        + "PRIMARY KEY(Date, TimeId));",
        "ALTER KEYSPACE %s WITH REPLICATION = %s;" % [$.cassandraKeyspace, $.cassandraReplication],
        "ALTER KEYSPACE system_auth WITH REPLICATION = %s;" % [$.cassandraReplication],
    ],

    ApplicationConf:: {
        width: 256,
        height: 256,
        thumb_width: 64,
        thumb_height: 64,
        iters: 200,
        database: $.cassandraKeyspace,
    },


    ImageMixin:: {
        project_id: $.project,
        account_file: "service_account_key.json",
        bucket_name: $.bucket,

        // For debugging:
        local network_debug = ["traceroute", "lsof", "iptraf", "tcpdump", "host", "dnsutils"],
        aptPackages +: ["vim", "git", "psmisc", "screen", "strace" ] + network_debug,
    },

    MyFlaskImage:: packer.GcpDebianNginxUwsgiFlaskImage + $.ImageMixin,


    "frontend.packer.json": $.MyFlaskImage {
        name: "frontend-v15",
        module: "main",
        pipPackages +: ["httplib2", "cassandra-driver", "blist"],
        uwsgiConf +: { lazy: "true" },  // cassandra-driver does not survive fork()
        provisioners +: [
            packer.File {
                source: "frontend",
                destination: "/tmp/",
            },
            packer.RootShell { inline: [
                "mv /tmp/frontend/* /var/www/",
                "chown -R www-data.www-data /var/www/*",
            ] },
        ],
    },

    "cassandra-primed.packer.json": packer.GcpDebianCassandraPrimedImage + $.ImageMixin {
        name: "cassandra-primed-v4",
        rootPassword: $.cassandraRootPass,
        clusterName: $.CassandraConf.cluster_name,
    },


    "imgproc.packer.json": $.MyFlaskImage {
        name: "imgproc-v5",
        module: "mandelbrot_service",

        aptPackages +: ["g++", "libpng-dev"],

        provisioners +: [
            packer.File {
                source: "imgproc",
                destination: "/tmp/",
            },
            packer.RootShell { inline: [
                "mv /tmp/imgproc/* /var/www/",
                "chown -R www-data.www-data /var/www/*",
            ] },
            packer.RootShell { inline: [
                "g++ -Wall -Wextra -ansi -pedantic -O3 -ffast-math -g "
                + "/var/www/mandelbrot.cpp -lpng -o /var/www/mandelbrot",
            ] },
        ],
    }, // imgproc.packer.json


    "terraform.tf": {

        provider: {
            google: {
                account_file: "service_account_key.json",
                client_secrets_file: "service_account_key.json",
                project: $.project,
                region: "us-central1",
            }
        },

        resource: {
            local resource = self,

            google_compute_address: {
                frontend: { name: "frontend" },
                imgproc: { name: "imgproc" },
            },

            google_compute_http_health_check: {
                fractal: {
                    name: "fractal",
                    port: 80,
                },
            },

            google_compute_target_pool: {
                frontend: {
                    name: "frontend",
                    health_checks: ["${google_compute_http_health_check.fractal.name}"],
                    instances: [ "us-central1-f/frontend" + k for k in [1, 2, 3, 4, 5, 6] ],
                },
                imgproc: {
                    name: "imgproc",
                    health_checks: ["${google_compute_http_health_check.fractal.name}"],
                    instances: [ "us-central1-f/imgproc" + k for k in [1, 2, 3, 4] ],
                },
            },

            google_compute_forwarding_rule: {
                frontend: {
                    ip_address: "${google_compute_address.frontend.address}",
                    name: "frontend",
                    target: "${google_compute_target_pool.frontend.self_link}",
                    port_range: "80",
                },
                imgproc: {
                    ip_address: "${google_compute_address.imgproc.address}",
                    name: "imgproc",
                    target: "${google_compute_target_pool.imgproc.self_link}",
                    port_range: "80",
                }
            },

            google_compute_network: {
                fractal: {
                    name: "fractal",
                    ipv4_range: "10.0.0.0/16",
                },
            },

            google_compute_firewall: terraform.GcpFirewallSsh + terraform.GcpFirewallHttp
                                     + cassandra.Firewall {
                network:: "${google_compute_network.fractal.name}",
            },

            // TODO: load balancers & instance groups
            FractalInstance:: terraform.GcpInstance {
                network +: {source: "${google_compute_network.fractal.name}"},
                tags +: ["fractal"],
                scopes +: ["devstorage.full_control"],
            },

            CassandraInstance:: self.FractalInstance {
                machine_type: "n1-standard-1",
                tags +: ["fractal-db", "cassandra-server"],
            },

            google_compute_instance: {
                ["frontend" + k]: resource.FractalInstance {
                    name: "frontend" + k,
                    image: "frontend-v1",
                    conf:: $.ApplicationConf {
                        database_name: $.cassandraKeyspace,
                        database_user: $.cassandraUser,
                        database_pass: $.cassandraUserPass,
                        imgproc: "${google_compute_address.imgproc.address}",
                        db_endpoints: $.cassandraNodes,
                    },
                    tags +: ["fractal-frontend", "http-server"],
                    startup_script +: [self.addFile(self.conf, "/var/www/conf.json")],
                }
                for k in [1, 2, 3]

            } + {
                // First node
                ["db1"]: resource.CassandraInstance {
                    name: "db1",
                    image: "cassandra-v1",
                    startup_script +: [
                        // Wait for the misconfigured cassandra to start up.
                        cassandra.waitForCqlsh("cassandra", $.cassandraRootPass, "localhost"),
                        // Kill it.
                        "/etc/init.d/cassandra stop",
                        // Drop in the correct configuration.
                        self.addFile($.CassandraConf, "/etc/cassandra/cassandra.yaml"),
                        // Start it up again.
                        "/etc/init.d/cassandra start",
                        // Wait until it can be contacted.
                        cassandra.waitForCqlsh("cassandra", $.cassandraRootPass, "$HOSTNAME"),
                        // Set up users, empty tables, etc.
                        local cql = std.lines($.cassandraInitCql);
                            "echo %s | cqlsh -u cassandra -p %s $HOSTNAME"
                            % [std.escapeStringBash(cql), $.cassandraRootPass],
                    ],
                },

            } + {
                // Runners up, will wait for db1 and then form a cluster
                ["db" + k]: resource.CassandraInstance {
                    name: "db" + k,
                    image: "cassandra-v1",
                    startup_script +: [
                        // Wait for the misconfigured cassandra to start up.
                        cassandra.waitForCqlsh("cassandra", $.cassandraRootPass, "localhost"),
                        // Kill it.
                        "/etc/init.d/cassandra stop",
                        // Clean up the mess it caused due to being misconfigured.
                        "rm -rf /var/lib/cassandra/*",
                        // Drop in the correct configuration.
                        self.addFile($.CassandraConf, "/etc/cassandra/cassandra.yaml"),
                        // Wait for db1 to be available.
                        cassandra.waitForCqlsh("cassandra", $.cassandraRootPass, "db1"),
                        // Start it up again.
                        "/etc/init.d/cassandra start",
                    ],
                }
                for k in [2, 3]

/*
            } + {
                // Top-up nodes, don't run unless seeds are contactable
                ["db" + k]: resource.CassandraInstance {
                    name: "db" + k,
                    image: "cassandra-primed-v4",
                    startup_script +: [
                        cassandra.waitForCqlsh("cassandra", $.cassandraRootPass, "localhost"),
                        "/etc/init.d/cassandra stop",
                        "rm -rf /var/lib/cassandra/*",
                        self.addFile($.CassandraConf, "/etc/cassandra/cassandra.yaml"),
                        "/etc/init.d/cassandra start",
                    ],
                }
                for k in [4, 5]
*/

            } + {
                ["imgproc" + k]: resource.FractalInstance {
                    name: "imgproc" + k,
                    image: "imgproc-v1",
                    tags +: ["fractal-imgproc", "http-server"],
                    startup_script +: [self.addFile($.ApplicationConf, "/var/www/conf.json")],
                }
                for k in [1, 2, 3, 4]
            }

        }

    } // deployment.tf

}
