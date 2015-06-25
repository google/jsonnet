local libimgcmd = import "lib/libimgcmd.jsonnet";
local libservice = import "lib/libservice.jsonnet";
local libcassandra = import "lib/libcassandra.jsonnet";
local libhttp = import "lib/libhttp.jsonnet";


{
    local app = self,

    cassandraUser:: "fractal",
    dnsSuffix:: error "Must override dnsSuffix",
    cassandraUserPass:: error "Must override cassandraUserPass",
    cassandraRootPass:: error "Must override cassandraRootPass",
    cassandraKeyspace:: "fractal",
    cassandraNodes:: ["db-n1", "db-n2", "db-n3", "db-n4", "db-n5"],
    tilegenPort:: 8080,

    // Configuration shared by the application server and tile generation nodes.
    ApplicationConf:: {
        width: 256,
        height: 256,
        thumb_width: 64,
        thumb_height: 64,
        db_endpoints: app.cassandraNodes,
    },

    ImageMixin:: {
        local network_debug = ["traceroute", "lsof", "iptraf", "tcpdump", "host", "dnsutils"],
        aptPackages +: ["vim", "git", "psmisc", "screen", "strace"] + network_debug,
        enableMonitoring: true,
        enableLogging: true,
    },

    AppservTemplate:: libhttp.GcpDebianNginxUwsgiFlask {
        local service = self,
        dnsSuffix: app.dnsSuffix,
        tilegen:: error "Appserv needs tilegen field",

        versions: {
            v1: service.CommonBaseInstance {
                Image+: app.ImageMixin {
                    pipPackages +: ["httplib2", "cassandra-driver", "blist"],
                },
                local version = self,
                conf:: app.ApplicationConf {
                    database: app.cassandraKeyspace,
                    database_name: app.cassandraKeyspace,
                    database_user: app.cassandraUser,
                    database_pass: app.cassandraUserPass,
                    tilegen_port: app.tilegenPort,
                },
                module: "main",  // Entrypoint in the Python code.
                uwsgiConf +: { lazy: "true" },  // cassandra-driver does not survive fork()
                // Copy website content and code.
                contentCmds+: [
                    "echo '${google_compute_address.%s.address} tilegen' >> /etc/hosts" % service.tilegen,
                    libimgcmd.CopyFile { from: "appserv/*", to: "/var/www" },
                    libimgcmd.LiteralFile { content: std.toString(version.conf), to: "/var/www/conf.json" },
                ],
                zones: ["us-central1-c", "us-central1-b", "us-central1-f"],
            }
        },
    },

    TilegenTemplate:: libhttp.GcpDebianNginxUwsgiFlask {
        local service = self,
        dnsSuffix: app.dnsSuffix,
        port: app.tilegenPort,
        versions: {
            v1: service.CommonBaseInstance {
                Image+: app.ImageMixin {
                    aptPackages +: ["g++", "libpng-dev"],
                },
                local version = self,
                srcDir:: "tilegen",
                conf:: app.ApplicationConf {
                    iters: 200,
                },
                module: "mandelbrot_service",
                contentCmds+: [
                    libimgcmd.CopyFile { from: version.srcDir + "/*", to: "/var/www" },
                    "g++ -Wall -Wextra -ansi -pedantic -O3 -ffast-math -g /var/www/mandelbrot.cpp -lpng -o /var/www/mandelbrot",
                    libimgcmd.LiteralFile { content: std.toString(version.conf), to: "/var/www/conf.json" },
                ],
                zones: ["us-central1-c", "us-central1-b", "us-central1-f"],
            },
        },

    },

    DbTemplate:: libcassandra.GcpDebianCassandra {
        local db = self,

        dnsSuffix: app.dnsSuffix,

        clusterName: "fractal-cluster",
        rootPassword: app.cassandraRootPass,
        initReplication:: "{ 'class' : 'SimpleStrategy', 'replication_factor' : 1 }",
        initAuthReplication:: db.initReplication,

        CommonBaseImage+: app.ImageMixin,

        CommonBaseInstance+: {
        },

        conf+: {
            cluster_name: db.clusterName,
            rpc_address:: null,  // Unset by making it hidden (::).
            listen_address:: null,  // Unset by making it hidden (::).
            authenticator: "PasswordAuthenticator",
            seed_provider: [
                {
                    class_name: "org.apache.cassandra.locator.SimpleSeedProvider",
                    parameters: [ { seeds: std.join(", ", app.cassandraNodes) } ],
                },
            ],
        },

        local cql_insert(uuid, x, y, l, n) =
            "INSERT INTO discoveries (Date, TimeId, X, Y, L, Text) "
            + ("VALUES ('FIXED', %s, %s, %s, %s, '%s');" % [uuid, x, y, l, n]),

        initCql: [
            "CREATE USER %s WITH PASSWORD '%s';" % [app.cassandraUser, app.cassandraUserPass],
            "CREATE KEYSPACE %s WITH REPLICATION = %s;" % [app.cassandraKeyspace, db.initReplication],
            "USE %s;" % app.cassandraKeyspace,
            "CREATE TABLE discoveries("
            + "Date TEXT, TimeId TIMEUUID, Text TEXT, X FLOAT, Y FLOAT, L INT, "
            + "PRIMARY KEY(Date, TimeId));",
            cql_insert("18063880-5a4d-11e4-ada4-247703d0f194", "0", "0", "0", "Zoomed Out"),
            cql_insert("66b6d100-5a53-11e4-aa05-247703d0f194",
                       "-1.21142578125", "0.3212890625", "4", "Lightning"),
            cql_insert("77ffdd80-5a53-11e4-8ccf-247703d0f194",
                       "-1.7568359375", "-0.0009765625", "5", "Self-similarity"),
            cql_insert("7fbf8200-5a53-11e4-804a-247703d0f194",
                       "0.342529296875", "0.419189453125", "5", "Windmills"),
            cql_insert("9ae7bd00-5a66-11e4-9c66-247703d0f194",
                       "-1.48309979046093", "0.00310595797955671", "39", "Star"),
            cql_insert("75fe4480-5a7c-11e4-a747-247703d0f194",
                       "-0.244976043701172", "0.716987609863281", "10", "Baroque"),
            cql_insert("abf70380-5b24-11e4-8a46-247703d0f194",
                       "-1.74749755859375", "0.009002685546875", "9", "Hairy windmills"),
        ],

        nodes: {
            n1: db.StarterNode {
                zone: "us-central1-c",
            },
        },

    },

    DnsZoneTemplate:: libservice.GcpZone {
        local service = self,
        dnsName: app.dnsSuffix,
        infrastructure+: {
            google_dns_record_set+: {
                www: {
                    managed_zone: "${google_dns_managed_zone.${-}.name}",
                    name: "www." + service.dnsName,
                    type: "CNAME",
                    ttl: 300,
                    rrdatas: ["appserv." + service.dnsName],
                },
            }
        }
    },

}
