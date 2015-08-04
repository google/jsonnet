local libimgcmd = import "libimgcmd.jsonnet";
local libservice = import "libservice.jsonnet";
local libos = import "libos.jsonnet";

{
    GcpDebianNginxUwsgiFlask: libservice.GcpCluster3 {
        local service = self,
        lbTcpPorts+: [self.port],
        fwTcpPorts+: [self.port],
        port:: 80,
        httpHealthCheckPort: self.port,

        CommonBaseInstance+: {
            local version = self,

            Image:: libservice.GcpImage + libos.DebianMixin {
                local image = self,

                aptPackages+: ["nginx", "python-dev"],
                pipPackages+: ["flask", "uwsgi"],

                nginxMonitoringConf:: [
                    "server {",
                    "    listen 80;",
                    "    server_name local-stackdriver-agent.stackdriver.com;",
                    "    location /nginx_status {",
                    "      stub_status on;",
                    "      access_log   off;",
                    "      allow 127.0.0.1;",
                    "      deny all;",
                    "    }",
                    "    location / {",
                    "      root /dev/null;",
                    "    }",
                    "}",
                ],

                nginxCollectdConf:: [
                    "LoadPlugin \"nginx\"",
                    "<Plugin \"nginx\">",
                    "  URL \"http://local-stackdriver-agent.stackdriver.com/nginx_status\"",
                    "</Plugin>",
                ],


                cmds+: (if self.enableMonitoring then [
                    libimgcmd.LiteralFile {
                        to: "/etc/nginx/conf.d/monitoring.conf",
                        content: std.lines(image.nginxMonitoringConf),
                    },
                    libimgcmd.LiteralFile {
                        to: "/opt/stackdriver/collectd/etc/collectd.d/nginx.conf",
                        content: std.lines(image.nginxCollectdConf),
                    },
                ] else []) + [
                    "rm /etc/nginx/sites-enabled/default",
                    libimgcmd.LiteralFile {
                        to: "/etc/cron.d/emperor",
                        content: "@reboot root /usr/local/bin/uwsgi --master --emperor /etc/uwsgi/vassals "
                                 + "--daemonize /var/log/uwsgi/emperor.log --pidfile /var/run/uwsgi.pid "
                                 + "--die-on-term --uid www-data --gid www-data\n",
                        filePermissions: "700",
                    },
                ]
            },

            disk: [
                {
                    image: version.Image,
                }
            ],

            application:: "app",
            module:: error "GcpDebianNginxUwsgiFlask must have module",

            uwsgiSocket:: "/var/www/uwsgi.sock",
            enableMonitoring:: true,

            uwsgiConf:: {
                chdir: "/var/www",
                base: "/var/www",
                module: version.module,
                pythonpath: "/var/www",
                socket: version.uwsgiSocket,
                "chmod-socket": "644",
                callable: version.application,
                logto: "/var/log/uwsgi/uwsgi.log",
            },

            nginxConf:: [
                "server {",
                "    listen %d;" % service.port,
                "    server_name ~^.*$;",
                "    charset     utf-8;",
                "    client_max_body_size 75M;",
                "    location / { try_files $uri @yourapplication; }",
                "    location @yourapplication {",
                "        include uwsgi_params;",
                "        uwsgi_pass unix:%s;" % version.uwsgiSocket,
                "    }",
                "}",
            ],

            contentCmds:: [
                libimgcmd.EnsureDir { dir: "/var/www", owner: "www-data" },
            ],
            cmds+: [
                libimgcmd.LiteralFile {
                    to: "/etc/nginx/conf.d/frontend_nginx.conf",
                    content: std.lines(version.nginxConf),
                },
            ] + [
                libimgcmd.EnsureDir { dir: "/etc/uwsgi/vassals" },
                libimgcmd.LiteralFile {
                    to: "/etc/uwsgi/vassals/uwsgi.ini",
                    content: std.manifestIni({
                        sections: {
                            uwsgi: version.uwsgiConf
                        }
                    })
                },
                libimgcmd.EnsureDir { dir: "/var/log/uwsgi", owner: "www-data" },
            ] + self.contentCmds + [
                "/usr/local/bin/uwsgi --master --emperor /etc/uwsgi/vassals "
                         + "--daemonize /var/log/uwsgi/emperor.log --pidfile /var/run/uwsgi.pid "
                         + "--die-on-term --uid www-data --gid www-data\n",
                "nginx -s reload",
            ],
        },
    },
}
