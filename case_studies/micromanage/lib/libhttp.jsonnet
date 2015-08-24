local libimgcmd = import "libimgcmd.jsonnet";
local libservice = import "libservice.jsonnet";
local libos = import "libos.jsonnet";

{
    GcpHttpService3: libservice.GcpCluster3 {
        port:: 80,
        lbTcpPorts+: [self.port],
        fwTcpPorts+: [self.port],
        httpHealthCheckPort: self.port,
    },

    GcpDebianNginx(port):: {

        local version = self,

        port:: port,

        enableMonitoring: true,
        enableLogging: true,

        StandardRootImage+: {
            local image = self,

            aptPackages+: ["nginx"],

            nginxMonitoringConf:: |||
                server {
                    listen 80;
                    server_name local-stackdriver-agent.stackdriver.com;
                    location /nginx_status {
                      stub_status on;
                      access_log   off;
                      allow 127.0.0.1;
                      deny all;
                    }
                    location / {
                      root /dev/null;
                    }
                }
            |||,

            nginxCollectdConf:: |||
                LoadPlugin "nginx"
                <Plugin "nginx">
                  URL "http://local-stackdriver-agent.stackdriver.com/nginx_status"
                </Plugin>
            |||,


            cmds+: (if version.enableMonitoring then [
                libimgcmd.LiteralFile {
                    to: "/etc/nginx/conf.d/monitoring.conf",
                    content: image.nginxMonitoringConf,
                },
                libimgcmd.LiteralFile {
                    to: "/opt/stackdriver/collectd/etc/collectd.d/nginx.conf",
                    content: image.nginxCollectdConf,
                },
            ] else []) + [
                "rm /etc/nginx/sites-enabled/default",
            ]
        },


        // Filse that must exist on top before any handling daemons are started.
        httpContentCmds:: [
            libimgcmd.EnsureDir { dir: "/var/www", owner: "www-data" },
        ],

        // Running handling daemons.
        httpHandlerCmds:: [
        ],

        // Additional Nginx config commands.
        nginxAdditionalCmds:: [
        ],

        cmds+: self.httpContentCmds + self.httpHandlerCmds + self.nginxAdditionalCmds + [
            "nginx -s reload",
        ],
    },

    DebianUwsgiFlask:: {

        local version = self,

        StandardRootImage+: {
            aptPackages+: ["python-dev"],
            pipPackages+: ["flask", "uwsgi"],
            cmds+: [
                libimgcmd.LiteralFile {
                    to: "/etc/cron.d/emperor",
                    content: "@reboot root /usr/local/bin/uwsgi --master --emperor /etc/uwsgi/vassals "
                             + "--daemonize /var/log/uwsgi/emperor.log --pidfile /var/run/uwsgi.pid "
                             + "--die-on-term --uid www-data --gid www-data\n",
                    filePermissions: "700",
                },
            ]
        },

        application:: "app",
        module:: "uwsgi_module",

        uwsgiSocket:: "/var/www/uwsgi.sock",

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

        uwsgiModuleContent:: null,

        httpContentCmds+: if version.uwsgiModuleContent == null then [ ] else [
            libimgcmd.LiteralFile {
                content: version.uwsgiModuleContent,
                to: "/var/www/%s.py" % version.module,
            },
        ],


        httpHandlerCmds+: [
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
            "/usr/local/bin/uwsgi --master --emperor /etc/uwsgi/vassals "
                     + "--daemonize /var/log/uwsgi/emperor.log --pidfile /var/run/uwsgi.pid "
                     + "--die-on-term --uid www-data --gid www-data\n",
        ],
    },

    NginxUwsgiGlue: {
        local version = self,

        nginxUwsgiConf:: |||
            server {
                listen %(port)d;
                server_name ~^.*$;
                charset     utf-8;
                client_max_body_size 75M;
                location / { try_files $uri @yourapplication; }
                location @yourapplication {
                    include uwsgi_params;
                    uwsgi_pass unix:%(uwsgiSocket)s;
                }
            }
        ||| % {port: version.port, uwsgiSocket: version.uwsgiSocket},

        nginxAdditionalCmds+: [
            libimgcmd.LiteralFile {
                to: "/etc/nginx/conf.d/frontend_nginx.conf",
                content: version.nginxUwsgiConf,
            },
        ],
    },

    GcpStandardFlask: $.GcpHttpService3 {
        local service = self,
        uwsgiModuleContent:: null,
        StandardVersion+: $.GcpDebianNginx(self.port) + $.DebianUwsgiFlask + $.NginxUwsgiGlue {
            uwsgiModuleContent: service.uwsgiModuleContent
        }
    }

}
