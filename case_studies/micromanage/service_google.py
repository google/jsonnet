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

import json
import re

from service import *

class GoogleService(Service):

    environmentSchema = {
        'type': 'object',
        'properties': {
            'kind': {'enum': ['Google']},
            'project': {'type': 'string'},
            'region': {'type': 'string'},
            'serviceAccount': {
                'type': 'object',
                'properties': {
                    'private_key_id': {'type': 'string'},
                    'private_key': {'type': 'string'},
                    'client_email': {'type': 'string'},
                    'client_id': {'type': 'string'},
                    'type': {'enum': ['service_account']},

                },
                'required': ['private_key', 'client_email'],
                'additionalProperties': False,
            },
            'sshUser': {'type': 'string'},
        },
        'required': ['kind', 'project', 'region', 'serviceAccount', 'sshUser'],
        'additionalProperties': False,
    }

    imageSchema = {
        'type': 'object',
        'properties': {
            'source': {'type': 'string'},
            'machine_type': {'type': 'string'},
            'zone': {'type': 'string'},
            'cmds': Service.cmdsSchema,
        },
        'additionalProperties': False,
    }

    serviceSchema = {
        'type': 'object',
        'properties' : {
            'environment' : {'type' : 'string'},
            'kind' : {'type' : 'string'},
            'children': {
                'type': 'object',
                'additionalProperties': {'$ref': '#/additionalProperties'},
            },
            'infrastructure' : {
                'type': 'object',
                'properties': {
                    'google_compute_disk': {
                        'type': 'object',
                        'additionalProperties': {
                            'type': 'object',
                            'properties': {
                                'image': {
                                    'oneOf': [
                                        {'type': 'string'},
                                        imageSchema,
                                    ],
                                },
                            },
                            'additionalProperties': True,
                        },
                    },
                    'google_compute_instance': {
                        'type': 'object',
                        'additionalProperties': {
                            'type': 'object',
                            'properties': {
                                'metadata': {
                                    'type': 'object',
                                    'additionalProperties': {'type': 'string'},
                                },
                                'disk': {
                                    'type': 'array',
                                    'items': [
                                        {
                                            'oneOf': [
                                                {
                                                    'type': 'object',
                                                    'properties': {
                                                        'image': {
                                                            'oneOf': [
                                                                {'type': 'string'},
                                                                imageSchema,
                                                            ],
                                                        }
                                                    },
                                                    'required': ['image'],
                                                    'additionalItems': True,
                                                },
                                                {
                                                    'type': 'object',
                                                    'properties': {
                                                        'disk': {'type': 'string'},
                                                    },
                                                    'required': ['disk'],
                                                    'additionalItems': True,
                                                },
                                            ]
                                        },
                                    ],
                                    'additionalItems': True,
                                    'minItems': 1,
                                },
                                'cmds': Service.cmdsSchema,
                                'bootCmds': Service.cmdsSchema,
                            },
                            'required': ['metadata', 'disk', 'cmds', 'bootCmds'],
                            'additionalProperties': True,
                        },
                    }
                },
                'additionalProperties': True,
            },
            'outputs': {
                'type': 'object',
                'additionalProperties': {'type': 'string'},
            },
        },
        'required': [
            'kind', 
            'infrastructure', 
            'outputs',
            'children',
        ],
        'additionalProperties': False,
    }

    def compilePackerImage(self, environment_name, environment, service_name, service, image):
        image_hash = 0;
        image_hash ^= self.hashString(image['machine_type'])
        image_hash ^= self.hashString(image['source'])
        image_hash ^= self.hashString(image['zone'])
        image_hash ^= self.hashProvisioners(image['cmds'])

        image_name = 'micromanage-%s' % self.encodeImageHash(image_hash)
        provisioners = self.compileProvisioners(image['cmds'])
        return {
            'builders': [
                {
                    'type': 'googlecompute',
                    'name': image_name,
                    'image_name': image_name,
                    'image_description': 'Image built by micromanage',
                    'project_id': environment['project'],
                    'account_file': environment_name + '-service-account-key.json',
                    'machine_type': image['machine_type'],
                    'source_image': image['source'],
                    'instance_name': image_name,
                    'zone': image['zone'],
                    'ssh_username': environment['sshUser'],
                }
            ],
            'provisioners': provisioners
        }, image_name

    def compileProvider(self, config, environment_name):
        environment = config['environments'][environment_name]
        terraform_file = 'environment.%s.tf' % environment_name
        service_account_key_file = environment_name + '-service-account-key.json'
        return {
            'tfs': {
                terraform_file: {
                    'provider': {
                        'google': {
                            'alias': environment_name,
                            'account_file': service_account_key_file,
                            'project': environment['project'],
                            'region' : environment['region'],
                        },
                    },
                },
            },
            'extras': {
                service_account_key_file: json.dumps(environment['serviceAccount'])
            },
        }

    def compile(self, environments, prefix, service_name, service):
        service_name = prefix + service_name
        environment_name = service.get('environment', 'default')
        environment = environments[environment_name]
        environment_name_tf = 'google.%s' % environment_name
        infra = service['infrastructure']
        packers = {}
        outputs = service['outputs']
        children = service['children']


        #def all_resources(sname, s):
        #    other_infra = s['infrastructure']
        #    r = []
        #    for res_kind_name, resources in other_infra.iteritems():
        #        for res_name in resources:
        #            expanded_name = self.translateSelfName(sname, res_name)
        #            r.append('%s.%s' % (res_kind_name, expanded_name))
        #    return r
        
        # Translate ${-} to service name
        def recursive_update(c):
            if isinstance(c, dict):
                return {
                    recursive_update(k): recursive_update(v)
                    for k, v in c.iteritems()
                }
            elif isinstance(c, list):
                return [recursive_update(v) for v in c]
            elif isinstance(c, basestring):
                return self.translateSelfName(service_name, c)
            else:
                return c
        infra = recursive_update(infra)
        outputs = recursive_update(outputs)


        # Add provider attributes
        for res_kind_name, res_kind_obj in infra.iteritems():
            for res_name, res in res_kind_obj.iteritems():
                res['provider'] = environment_name_tf
            
        instances = infra.get('google_compute_instance') or {}
        disks = infra.get('google_compute_disk') or {}

        # Process image configs
        for inst_name, inst in instances.iteritems():
            image = inst['disk'][0].get('image')
            if isinstance(image, dict):
                packer, image_name = self.compilePackerImage(environment_name, environment, service_name, service, image)
                inst['disk'][0]['image'] = image_name
                packers[image_name + '.packer.json'] = packer
        for disk_name, disk in disks.iteritems():
            image = disk['image']
            if isinstance(image, dict):
                packer, image_name = self.compilePackerImage(environment_name, environment, service_name, service, image)
                disk['image'] = image_name
                packers[image_name + '.packer.json'] = packer

        # Process commands
        for inst_name, inst in instances.iteritems():
            cmds = inst['cmds']
            boot_cmds = inst['bootCmds']
            metadata = inst['metadata']
            def curl_md(k):
                md_pref = 'http://169.254.169.254/computeMetadata/v1/instance/attributes'
                return 'curl -s -H Metadata-Flavor:Google %s/%s' % (md_pref, k)
            if 'startup-script' in metadata:
                # Move user startup script out of the way (but still run it at every boot).
                metadata['micromanage-user-startup-script'] = metadata['startup-script']
                metadata.pop('startup-script', None)
                bootCmds += ['%s | bash' % curl_md('micromanage-user-startup-script')]
            inst['metadata'] = metadata
            inst['metadata_startup_script'] = self.compileStartupScript(cmds, boot_cmds)
            inst.pop('cmds', None)
            inst.pop('bootCmds', None)

        terraform_file = 'service.%s.tf' % service_name

        return [
            {
                'packers': packers,
                'tfs': {
                    terraform_file: {
                        'resource': infra,
                        'output': {
                            k: { 'value': outputs[k] }
                            for k in outputs
                        }
                    },
                },
                'extras': { },
            }
        ] + [
            artefacts
            for child_name, child in children.iteritems()
            for artefacts in self.compile(environments, '%s%s-' % (prefix, service_name), child_name, child)
        ]
