local cmd = import 'mmlib/v0.1.2/cmd/cmd.libsonnet';
local cassandra = import 'mmlib/v0.1.2/db/cassandra.libsonnet';
local service_google = import 'mmlib/v0.1.2/service/google.libsonnet';
local web_solutions = import 'mmlib/v0.1.2/web/solutions.libsonnet';
local web = import 'mmlib/v0.1.2/web/web.libsonnet';

function(parent, name) service_google.Service(parent, name) {
  local app = self,

  cassandraUser:: 'fractal',
  dnsSuffix:: error 'Must override dnsSuffix',
  cassandraUserPass:: error 'Must override cassandraUserPass',
  cassandraRootPass:: error 'Must override cassandraRootPass',
  cassandraKeyspace:: 'fractal',
  cassandraNodes:: [self.prefixName(n) for n in ['db-n1', 'db-n2', 'db-n3', 'db-n4', 'db-n5']],

  // Configuration shared by the application server and tile generation nodes.
  ApplicationConf:: {
    width: 256,
    height: 256,
    thumb_width: 64,
    thumb_height: 64,
  },

  DebugPackagesMixin:: {
    StandardRootImage+: {
      local network_debug = ['traceroute', 'lsof', 'iptraf', 'tcpdump', 'host', 'dnsutils'],
      aptPackages+: ['vim', 'git', 'psmisc', 'screen', 'strace'] + network_debug,
    },
  },


  zone: service_google.DnsZone(app, 'zone') {
    dnsName: app.dnsSuffix,
  },

  www: service_google.DnsRecordWww(app, 'www') {
    zone: app.zone,
    target: app.appserv.fullName,
  },

  network: service_google.Network(app, 'network'),

  appserv: service_google.Cluster3(app, 'appserv') + web.HttpService3 + web_solutions.DebianFlaskHttpService {

    local service = self,
    dnsZone: app.zone,

    networkName: app.network.nameRef,

    httpsPort: 443,

    Instance+: app.DebugPackagesMixin {
      machine_type: 'n1-standard-1',
      StandardRootImage+: {
        pipPackages+: ['httplib2', 'cassandra-driver', 'blist'],
      },
      local version = self,
      conf:: app.ApplicationConf {
        database: app.cassandraKeyspace,
        db_endpoints: app.cassandraNodes,
        database_name: app.cassandraKeyspace,
        database_user: app.cassandraUser,
        database_pass: app.cassandraUserPass,
        tilegen_endpoint: app.tilegen.endpoint,
      },
      module: 'main',  // Entrypoint in the Python code.
      uwsgiConf+: { lazy: 'true' },  // cassandra-driver does not survive fork()
      // Copy website content and code.
      httpContentCmds+: [
        "echo '%s tilegen' >> /etc/hosts" % app.tilegen.addressRef,
        cmd.CopyFile { from: std.resolvePath(std.thisFile, 'appserv/*'), to: '/var/www' },
        cmd.LiteralFile { content: std.toString(version.conf), to: '/var/www/conf.json' },
      ],
    },
  },

  tilegen: service_google.Cluster3(app, 'tilegen') + web.HttpService3 + web_solutions.DebianFlaskHttpService {
    local service = self,
    dnsZone: app.zone,
    endpoint:: self.httpsEndpoint,

    networkName: app.network.nameRef,

    httpsPort: 443,
    Instance+: app.DebugPackagesMixin {
      machine_type: 'n1-standard-1',
      StandardRootImage+: {
        aptPackages+: ['g++', 'libpng-dev'],
      },
      local version = self,
      srcDir:: 'tilegen3',
      conf:: app.ApplicationConf {
        iters: 200,
      },
      module: 'mandelbrot_service',
      httpContentCmds+: [
        cmd.CopyFile { from: std.resolvePath(std.thisFile, 'tilegen/*'), to: '/var/www' },
        'g++ -Wall -Wextra -ansi -pedantic -O3 -ffast-math -g /var/www/mandelbrot.cpp -lpng -o /var/www/mandelbrot',
        cmd.LiteralFile { content: std.toString(version.conf), to: '/var/www/conf.json' },
      ],
    },
  },

  db: cassandra.GcpDebianCassandra(app, 'db') {
    local db = self,
    dnsZone: app.zone,

    networkName: app.network.nameRef,

    rootPassword: app.cassandraRootPass,

    cassandraConf+: {
      rpc_address:: null,  // Unset by making it hidden (::).
      listen_address:: null,  // Unset by making it hidden (::).
      authenticator: 'PasswordAuthenticator',
      seed_provider: [
        {
          class_name: 'org.apache.cassandra.locator.SimpleSeedProvider',
          parameters: [{ seeds: std.join(', ', app.cassandraNodes) }],
        },
      ],
    },

    Instance+: app.DebugPackagesMixin {
      machine_type: 'n1-standard-4',
    },

    // Put some initial data in a fresh database.
    StarterNode+: {
      local cql_insert(uuid, x, y, l, n) =
        'INSERT INTO discoveries (Date, TimeId, X, Y, L, Text) '
        + ("VALUES ('FIXED', %s, %s, %s, %s, '%s');" % [uuid, x, y, l, n]),

      initCql: [
        "CREATE USER %s WITH PASSWORD '%s';" % [app.cassandraUser, app.cassandraUserPass],
        'CREATE KEYSPACE %s WITH REPLICATION = %s;' % [app.cassandraKeyspace, self.initReplication],
        'USE %s;' % app.cassandraKeyspace,
        'CREATE TABLE discoveries('
        + 'Date TEXT, TimeId TIMEUUID, Text TEXT, X FLOAT, Y FLOAT, L INT, '
        + 'PRIMARY KEY(Date, TimeId));',
        cql_insert('18063880-5a4d-11e4-ada4-247703d0f194', '0', '0', '0', 'Zoomed Out'),
        cql_insert('66b6d100-5a53-11e4-aa05-247703d0f194',
                   '-1.21142578125',
                   '0.3212890625',
                   '4',
                   'Lightning'),
        cql_insert('77ffdd80-5a53-11e4-8ccf-247703d0f194',
                   '-1.7568359375',
                   '-0.0009765625',
                   '5',
                   'Self-similarity'),
        cql_insert('7fbf8200-5a53-11e4-804a-247703d0f194',
                   '0.342529296875',
                   '0.419189453125',
                   '5',
                   'Windmills'),
        cql_insert('9ae7bd00-5a66-11e4-9c66-247703d0f194',
                   '-1.48309979046093',
                   '0.00310595797955671',
                   '39',
                   'Star'),
        cql_insert('75fe4480-5a7c-11e4-a747-247703d0f194',
                   '-0.244976043701172',
                   '0.716987609863281',
                   '10',
                   'Baroque'),
        cql_insert('abf70380-5b24-11e4-8a46-247703d0f194',
                   '-1.74749755859375',
                   '0.009002685546875',
                   '9',
                   'Hairy windmills'),
      ],
    },

    nodes: {
      n1: db.StarterNode {
        initReplicationFactor: 1,
      },
    },
  },

  prober: service_google.SingleInstance(app, 'prober') {
    zone: 'us-central1-b',
    dnsZone: app.zone,
    networkName: app.network.nameRef,
    perServiceFirewalls:: false,
    Instance+: app.DebugPackagesMixin {
      local version = self,
      script:: |||
        from selenium import webdriver
        import sched
        import time
        # Option 1 - with ChromeOptions
        chrome_options = webdriver.ChromeOptions()
        chrome_options.add_argument('--headless')
        chrome_options.add_argument('--no-sandbox')

        s = sched.scheduler(time.time, time.sleep)
        def do_probe(sc):
            driver = webdriver.Chrome(
                chrome_options=chrome_options,
                service_args=['--verbose', '--log-path=/var/log/chromedriver.log'])
            driver.get('%(endpoint)s')
            time.sleep(5)
            print(driver.title)
            print(len(driver.find_element_by_id('remembered_list').find_elements_by_tag_name('img')))
            s.enter(60, 1, do_probe, (sc,))
        s.enter(0, 1, do_probe, (s,))
        s.run()
      ||| % { endpoint: app.appserv.httpsEndpoint },
      StandardRootImage+: {
        aptPackages+: ['chromium', 'unzip'],
        pipPackages+: ['selenium'],
        cmds+: [
          'wget https://chromedriver.storage.googleapis.com/2.35/chromedriver_linux64.zip',
          'unzip chromedriver_linux64.zip',
          'mv chromedriver /usr/local/bin',
        ],
      },
      cmds+: [
        cmd.EnsureDir { dir: '/var/prober', owner: 'root' },
        cmd.LiteralFile { content: std.toString(version.script), to: '/var/prober/script.py' },
      ],
      bootCmds+: [
        'python /var/prober/script.py',
      ],
    },
  },
}
