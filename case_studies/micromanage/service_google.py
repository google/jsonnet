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

import copy
import json
import dateutil.parser
import re

from oauth2client.client import flow_from_clientsecrets
from oauth2client.client import GoogleCredentials
from oauth2client.client import SignedJwtAssertionCredentials
from googleapiclient.discovery import build

from packer import *
from service import *
from util import *
import validate

IMAGE_CACHE = {}

def google_get_images_json_key(project, key_json):
    credentials = SignedJwtAssertionCredentials(
        key_json['client_email'],
        key_json['private_key'],
        scope='https://www.googleapis.com/auth/compute')
        
    compute = build('compute', 'v1', credentials=credentials)
    images = compute.images().list(project=project).execute()
    items = images.get('items', [])
    return [(i['name'], dateutil.parser.parse(i['creationTimestamp'])) for i in items]


class GooglePackerBuildArtefact(PackerBuildArtefact):
    def __init__(self, image, environment):
        super(GooglePackerBuildArtefact, self).__init__(image['cmds'])

        self.machineType = image['machine_type']
        self.source = image['source']
        self.zone = image['zone']
        self.project = environment['project']
        self.sshUser = environment['sshUser']
        self.serviceAccount = environment['serviceAccount']

    def builderHashCode(self):
        builder_hash = 0;
        builder_hash ^= hash_string(self.machineType)
        builder_hash ^= hash_string(self.source)
        builder_hash ^= hash_string(self.zone)
        return builder_hash

    def builder(self):
        return {
            'name': self.name(),
            'image_name': self.name(),
            'instance_name': self.name(),

            'type': 'googlecompute',
            'image_description': 'Image built by micromanage',
            'project_id': self.project,
            'account_file': json.dumps(self.serviceAccount),
            'machine_type': self.machineType,
            'source_image': self.source,
            'zone': self.zone,
            'ssh_username': self.sshUser,
        }

    def needsBuild(self):
        print 'Checking if image exists: %s/%s' % (self.project, self.name())
        if self.project in IMAGE_CACHE:
            existing_image_names = IMAGE_CACHE[self.project]
        else:
            existing_image_names = [img[0] for img in google_get_images_json_key(self.project, self.serviceAccount)]
            IMAGE_CACHE[self.project] = existing_image_names
        return self.name() not in existing_image_names

    def doBuild(self, dirpath):
        super(GooglePackerBuildArtefact, self).doBuild(dirpath)
        if self.project not in IMAGE_CACHE:
            IMAGE_CACHE[self.project] = []
        IMAGE_CACHE[self.project] += [self.name()]

    def postBuild(self):
        pass

class GoogleService(Service):

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
            'environment', 
            'infrastructure', 
            'outputs',
            'children',
        ],
        'additionalProperties': False,
    }

    def validateEnvironment(self, env_name, env):
        ctx = 'Environment "%s"' % env_name
        fields = ['kind', 'project', 'region', 'sshUser', 'serviceAccount']
        validate.obj_only(ctx, env, fields)
        validate.obj_field(ctx, env, 'project', 'string')
        validate.obj_field(ctx, env, 'region', 'string')
        validate.obj_field(ctx, env, 'sshUser', 'string')

        acc = validate.obj_field(ctx, env, 'serviceAccount', validate.is_type('object'))
        validate.obj_field(ctx + ' serviceAccount', acc, 'client_email', 'string')
        validate.obj_field(ctx + ' serviceAccount', acc, 'private_key', 'string')
        validate.obj_field_opt(ctx + ' serviceAccount', acc, 'type', validate.is_value('service_account'))
        validate.obj_field_opt(ctx + ' serviceAccount', acc, 'client_id', 'string')
        validate.obj_field_opt(ctx + ' serviceAccount', acc, 'private_key_id', 'string')
        fields = ['client_email', 'private_key', 'type', 'client_id', 'private_key_id']
        validate.obj_only(ctx + ' serviceAccount', acc, fields)

    def compileProvider(self, environment_name, environment):
        return {
            'environment.%s.tf' % environment_name: {
                'provider': {
                    'google': {
                        'alias': environment_name,
                        'account_file': json.dumps(environment['serviceAccount']),
                        'project': environment['project'],
                        'region' : environment['region'],
                    },
                },
            },
        }

    def getBuildArtefacts(self, environment, ctx, service):
        service = copy.deepcopy(service)
        barts = {}  # Build artefacts.

        infra = service['infrastructure']
        instances = infra.get('google_compute_instance', {})
        disks = infra.get('google_compute_disk', {})

        # Process image configs
        for inst_name, inst in instances.iteritems():
            image = inst['disk'][0].get('image')
            if isinstance(image, dict):
                bart = GooglePackerBuildArtefact(image, environment)
                barts[bart.name()] = bart
                inst['disk'][0]['image'] = bart.name()
        for disk_name, disk in disks.iteritems():
            image = disk['image']
            if isinstance(image, dict):
                bart = GooglePackerBuildArtefact(image, environment)
                barts[bart.name()] = bart
                disk['image'] = bart.name()

        return service, barts


    def compile(self, ctx, service_name, service, barts):
        infra = service['infrastructure']

        # Add provider attributes
        for res_kind_name, res_kind_obj in infra.iteritems():
            for res_name, res in res_kind_obj.iteritems():
                res['provider'] = 'google.%s' % service['environment']
            
        # Process instance commands
        instances = infra.get('google_compute_instance', {})
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

        return {
            'service.%s.tf' % self.fullName(ctx, service_name): {
                'resource': infra,
                'output': {
                    k: { 'value': v }
                    for k, v in service['outputs'].iteritems()
                }
            }
        }

