local cmd = import '../cmd/cmd.libsonnet';

{
  // A mixin on top of a Debian instance that manages an Nginx installation.  This supports:
  // - Monitoring is enabled by default.
  // - /var/www is initialized correctly
  // - Two hooks are provided for running handler daemons.
  DebianNginxMixin:: {

    local version = self,

    enableMonitoring: version.supportsMonitoring,
    enableLogging: version.supportsLogging,

    StandardRootImage+: {
      local image = self,

      aptPackages+: ['nginx'],

      // Configure nginx to expose stub_status to the local agent.
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

      // Configure Collectd to find the stub_status at the above URL.
      nginxCollectdConf:: |||
        LoadPlugin "nginx"
        <Plugin "nginx">
          URL "http://local-stackdriver-agent.stackdriver.com/nginx_status"
        </Plugin>
      |||,


      cmds+: (if version.enableMonitoring then [
                cmd.LiteralFile {
                  to: '/etc/nginx/conf.d/monitoring.conf',
                  content: image.nginxMonitoringConf,
                },
                cmd.LiteralFile {
                  to: '/opt/stackdriver/collectd/etc/collectd.d/nginx.conf',
                  content: image.nginxCollectdConf,
                },
              ] else []) + [

        // Disable the default site.
        'rm /etc/nginx/sites-enabled/default',
      ],
    },


    // Files that must exist on top before any handling daemons are started.
    // Extend this to deploy more content for the web server.
    httpContentCmds:: [
      cmd.EnsureDir { dir: '/var/www', owner: 'www-data' },
    ],

    // Running handling daemons.
    httpHandlerCmds:: [
    ],

    // Additional Nginx config commands.
    nginxAdditionalCmds:: [
    ],

    cmds+: self.httpContentCmds + self.httpHandlerCmds + self.nginxAdditionalCmds + [
      'nginx -s reload',
    ],
  },
}
