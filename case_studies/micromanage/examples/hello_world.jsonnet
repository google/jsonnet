# Copyright 2015 Google Inc. All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

local libhttp = import "lib/libhttp.jsonnet";
local libservice = import "lib/libservice.jsonnet";

{
    environments: {
        default: {
            kind: "Google",
            project: "readable-name-123",  // Change this.
            region: "us-central1",  // Maybe change this.
            // Download this file from the developers console.
            serviceAccount: import "service_account.json",
            sshUser: "yourusername"  // Change this.
        },
    },

    // Simple case -- one machine serving this Python script.
    helloworld: libhttp.GcpStandardFlask + libservice.UniCluster("us-central1-f") {
        uwsgiModuleContent: |||
            import flask
            import socket
            app = flask.Flask(__name__) 
            @app.route('/') 
            def hello_world():
                return 'Hello from %s!' % socket.gethostname()
        |||,
    },

    // For production -- allows canarying changes.
    helloworld2: libhttp.GcpStandardFlask {
        local service = self,
        httpPort: 8080,
        zones: ["us-central1-b", "us-central1-c", "us-central1-f"],
        versions: {
            v1: service.StandardVersion {
                uwsgiModuleContent: |||
                    import flask
                    import socket
                    app = flask.Flask(__name__) 
                    @app.route('/') 
                    def hello_world():
                        return 'Hello from %s!' % socket.gethostname()
                |||,
            },
            v2: service.StandardVersion {
                uwsgiModuleContent: |||
                    import flask
                    import socket
                    app = flask.Flask(__name__) 
                    @app.route('/') 
                    def hello_world():
                        return 'Greetings from %s!' % socket.gethostname()
                |||,
            },
        },
        deployment: {
            v1: {
                deployed: [1, 2, 3],
                attached: [1, 2, 3],
            },
            v2: {
                deployed: [1],
                attached: [1],
            },
        },
    },

}
