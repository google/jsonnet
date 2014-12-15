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

// Import some libraries
local packer = import "packer.jsonnet";
local terraform = import "terraform.jsonnet";
local cassandra = import "cassandra.jsonnet";

// Credentials file (don't commit this!)
local credentials = import "credentials.jsonnet";

{
    ///////////////////////////
    // GENERAL CONFIGURATION //
    ///////////////////////////

    cassandraUser:: "fractal",  // Password in credentials import.
    cassandraKeyspace:: "fractal",
    cassandraReplication:: "{ 'class' : 'SimpleStrategy', 'replication_factor' : 2 }",

    // These are the nodes the frontends attempt to use (client-side load balancing).
    cassandraNodes:: ["db1", "db2", "db3", "db4", "db5"],

    // The Cassandra configuration file we use for this service.
    cassandraConf:: cassandra.conf {
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

    // Create an initial (empty) database for storing 'discovered' fractal co-ordinates.
    cassandraInitCql:: [
        "CREATE USER %s WITH PASSWORD '%s';" % [$.cassandraUser, credentials.cassandraUserPass],
        local rep1 =  "{ 'class' : 'SimpleStrategy', 'replication_factor' : 1 }";
            "CREATE KEYSPACE %s WITH REPLICATION = %s;" % [$.cassandraKeyspace, rep1],
        "USE %s;" % $.cassandraKeyspace,
        "CREATE TABLE discoveries("
        + "Date TEXT, TimeId TIMEUUID, Text TEXT, X FLOAT, Y FLOAT, L INT, "
        + "PRIMARY KEY(Date, TimeId));",
        "ALTER KEYSPACE %s WITH REPLICATION = %s;" % [$.cassandraKeyspace, $.cassandraReplication],
        "ALTER KEYSPACE system_auth WITH REPLICATION = %s;" % [$.cassandraReplication],
    ],

    // Configuration shared by the application server and image processing nodes.
    ApplicationConf:: {
        width: 256,
        height: 256,
        thumb_width: 64,
        thumb_height: 64,
        iters: 200,
        database: $.cassandraKeyspace,
    },

    ///////////////////////////
    // PACKER CONFIGURATIONS //
    ///////////////////////////

    // Some config used in every Packer image we create.
    ImageMixin:: {
        project_id: credentials.project,
        account_file: "service_account_key.json",

        // For debugging:
        local network_debug = ["traceroute", "lsof", "iptraf", "tcpdump", "host", "dnsutils"],
        aptPackages +: ["vim", "git", "psmisc", "screen", "strace" ] + network_debug,
    },

    // An Nginx/uwsgi/flask installation is used to serve HTTP on the frontend and image processors.
    MyFlaskImage:: packer.GcpDebianNginxUwsgiFlaskImage + $.ImageMixin,

    // Frontend image.
    "frontend.packer.json": $.MyFlaskImage {
        name: "frontend-v20141213-1300",
        module: "main",   // Entrypoint in the Python code.
        pipPackages +: ["httplib2", "cassandra-driver", "blist"],
        uwsgiConf +: { lazy: "true" },  // cassandra-driver does not survive fork()
        // Copy website content and code.
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

    // The Cassandra image is basic, but more configuration is done at deployment time.
    "cassandra.packer.json": cassandra.GcpDebianImage + $.ImageMixin {
        name: "cassandra-v20141211-1100",
        rootPassword: credentials.cassandraRootPass,
        clusterName: $.cassandraConf.cluster_name,
    },

    // The image processing node runs a C++ program to generate fractal tiles.
    "imgproc.packer.json": $.MyFlaskImage {
        name: "imgproc-v20141211-1100",
        module: "mandelbrot_service",

        aptPackages +: ["g++", "libpng-dev"],

        // Copy the flask handlers and also build the C++ executable.
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
    },


    /////////////////////////////
    // TERRAFORM CONFIGURATION //
    /////////////////////////////

    "terraform.tf": {

        // How to contact the Google Cloud Platform APIs.
        provider: {
            google: {
                account_file: "service_account_key.json",
                client_secrets_file: "service_account_key.json",
                project: credentials.project,
                region: "us-central1",
            }
        },

        // The deployed resources.
        resource: {
            local resource = self,

            // Instances are assigned zones on a round robin scheme.
            local zone(hash) = 
                local arr = [
                    "us-central1-a",
                    "us-central1-b",
                    "us-central1-f",
                ];
                arr[hash % std.length(arr)],

            // The internal subnet.
            google_compute_network: {
                fractal: {
                    name: "fractal",
                    ipv4_range: "10.0.0.0/16",
                },
            },

            // Publicly visible static ip addresses.
            google_compute_address: {
                frontend: { name: "frontend" },
                imgproc: { name: "imgproc" },
            },

            // The next 3 resource types configure load balancing for the frontend and imgproc.
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
                    instances: [ "%s/frontend%d" % [zone(k), k] for k in [1, 2, 3] ],
                },
                imgproc: {
                    name: "imgproc",
                    health_checks: ["${google_compute_http_health_check.fractal.name}"],
                    instances: [ "%s/imgproc%d" % [zone(k), k] for k in [1, 2, 3, 4] ],
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

            // Open ports for the various services, to instances (identified by tags)
            google_compute_firewall: terraform.GcpFirewallSsh + terraform.GcpFirewallHttp
                                     + cassandra.GcpFirewall {
                network:: "${google_compute_network.fractal.name}",
            },

            // All our instances share this configuration.
            FractalInstance(zone_hash):: terraform.GcpInstance {
                network +: {source: "${google_compute_network.fractal.name}"},
                tags +: ["fractal"],
                scopes +: ["devstorage.full_control"],
                zone: zone(zone_hash),
            },

            // The various kinds of Cassandra instances all share this basic configuration.
            CassandraInstance(zone_hash):: self.FractalInstance(zone_hash) {
                image: "cassandra-v20141211-1100",
                machine_type: "n1-standard-1",
                tags +: ["fractal-db", "cassandra-server"],
                user:: $.cassandraUser,
                userPass:: credentials.cassandraUserPass,
                rootPass:: credentials.cassandraRootPass,
                conf:: $.cassandraConf,
            },

            google_compute_instance: {

                // The frontend instances have database credentials and the address of the imgproc
                // loadbalancer.  This is all stored in a conf.json, read by the python code.
                ["frontend" + k]: resource.FractalInstance(k) {
                    name: "frontend" + k,
                    image: "frontend-v20141213-1300",
                    conf:: $.ApplicationConf {
                        database_name: $.cassandraKeyspace,
                        database_user: $.cassandraUser,
                        database_pass: credentials.cassandraUserPass,
                        imgproc: "${google_compute_address.imgproc.address}",
                        db_endpoints: $.cassandraNodes,
                    },
                    tags +: ["fractal-frontend", "http-server"],
                    startup_script +: [self.addFile(self.conf, "/var/www/conf.json")],
                }
                for k in [1, 2, 3]

            } + {

                // Bootstrapping the Cassandra database is a little subtle.  We bring up 3 nodes
                // in parallel, one of which is special and creates the initial database.  The other
                // two wait for it to be ready and join the cluster one at a time.

                // The CQL code is used to bootstrap the database.
                db1: resource.CassandraInstance(1) + cassandra.GcpStarterMixin {
                    name: "db1",
                    initCql:: $.cassandraInitCql,
                },

                // TopUpMixin creates an empty Cassandra node which can join an existing cluster.
                db2: resource.CassandraInstance(2) + cassandra.GcpTopUpMixin {
                    name: "db2",
                    waitFor:: "db1",
                },

                db3: resource.CassandraInstance(3) + cassandra.GcpTopUpMixin {
                    name: "db3",
                    waitFor:: "db2",
                },

                // To increase the size of the cluster, these can be used.  Bring them up one by one
                // verifying the state of the database each time by logging onto an existing db node
                // and running "nodetool status fractal".  To reduce the size of the cluster, use
                // nodetool -h $HOST decommission and then remove the node from this configuration.
                // If a node is removed without decommissioning, use nodetool removenode <UUID>.
                /*
                db4: resource.CassandraInstance(4) + cassandra.GcpTopUpMixin {
                    name: "db4",
                },
                */

            } + {
                // The image processor instances are similar to the frontend ones, but do not
                // require database credentials so these are omitted for security.
                ["imgproc" + k]: resource.FractalInstance(k) {
                    name: "imgproc" + k,
                    image: "imgproc-v20141211-1100",
                    tags +: ["fractal-imgproc", "http-server"],
                    startup_script +: [self.addFile($.ApplicationConf, "/var/www/conf.json")],
                }
                for k in [1, 2, 3, 4]
            }

        }

    } // deployment.tf

}
