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

local su_startup_script = "#!/bin/bash -x
URL=http://metadata/computeMetadata/v1/instance/attributes/app-startup-script
curl $URL -H 'X-Google-Metadata-Request: True' -o app-startup-script
chmod a+x app-startup-script
su - appserver -c /app-startup-script";

{
    local gcp = self,

    Address: {
        kind: "compute#address",
        // abstract name,
        // abstract region,
    },

    Network: {
        kind: "compute#network",
        // abstract name,
        IPv4Range: "10.0.0.0/8",
    },

    Firewall: {
        kind: "compute#firewall",
        // abstract name,
        // abstract network,
        sourceRanges: ["0.0.0.0/0"],
        // abstract allowed,
    },

    TargetPool: {
        kind: "compute#targetPool",
        // abstract name,
        // abstract region,
        sessionAffinity: "NONE",
        // abstract instances,
    },

    ForwardingRule: {
        kind: "compute#forwardingRule",
        // abstract name,
        // abstract region,

        // abstract IPAddress,
        // abstract portRange,
        // abstract target,
    },

    SimpleNetworkInterface: function(network) {
        accessConfigs: [{
            type: "ONE_TO_ONE_NAT",
            name: "External NAT",
        }],
        network: network,
    },

    SimpleBootDisk: function(name, image) {
        autoDelocale: "true",
        boot: "true",
        type: "PERSISTENT",
        initializeParams: {
            diskName: name,
            sourceImage: image,
        },
    },

    Instance: {
        kind: "compute#instance",
        // abstract name,
        // abstract zone,
        machineType: "n1-standard-1",
        disks: [],
        networkInterfaces: [],
        serviceAccounts: [],
        metadata: [],
    },

    /** Installs a set of packages, and runs a given binary as user "appserver". */
    SimpleInstance: gcp.Instance {
        local instance = self,
        // abstract bookDiskName
        disks: [gcp.SimpleBootDisk(self.bootDiskName, self.image)],
        networkInterfaces: [gcp.SimpleNetworkInterface(self.network)],
        environmentVariables: [],
        // abstract packages,
        // abstract binary,
        serviceAccounts: [{
            email: "default",
            scopes: ["devstorage.full_control"],
        }],
        metadata: [{
            items: [{
                key: "startup-script",
                value: su_startup_script,
            }, {
                key: "app-startup-script",
                value: instance.script,
            }], 
        }],     
        script:: std.join(
            "\n",
            ["#!/bin/bash -x"] +
            std.join([], [
                ["mkdir '" + k + "'",
                 "cd '" + k + "'",
                 "gsutil cat '" + self.packages[k].uri + "' | tar xfz -",
                 "cd ..",]
                for k in std.objectFields(self.packages)]) +
            ["cd '" + self.binary.package + "'"] +
            std.join([], [
                ["export " + k + "='" + self.environmentVariables[k] + "'"]
                for k in std.objectFields(self.environmentVariables)]) +
            [self.binary.bin + " >../app.out 2>../app.err &"])

    },

    AppEngine: {
        kind: "compute#appEngine",
    },
}
