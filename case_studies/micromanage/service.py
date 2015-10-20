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

import glob
import hashlib
import json
import os
import re
import subprocess

from cmds import *
from util import *
import validate

class Service(object):

    schama = {
        'additionalProperties': False,
    },

    cmdsSchema = {
        'type': 'array',
        'items': {
            'oneOf': [
                {
                    'type': 'string',
                },
                {
                    'type': 'object',
                    'properties': {
                        'kind': {'enum': ['CopyFile']},
                        'owner': {'type': 'string' },
                        'group': {'type': 'string' },
                        'dirPermissions': {'type': 'string' },
                        'filePermissions': {'type': 'string' },
                        'from': {'type': 'string' },
                        'to': {'type': 'string' },
                    },
                    'required': ['kind', 'owner', 'group', 'dirPermissions', 'filePermissions', 'from', 'to'],
                    'additionalProperties': False,
                },
                {
                    'type': 'object',
                    'properties': {
                        'kind': {'enum': ['LiteralFile']},
                        'owner': {'type': 'string' },
                        'group': {'type': 'string' },
                        'filePermissions': {'type': 'string' },
                        'content': {'type': 'string' },
                        'to': {'type': 'string' },
                    },
                    'required': ['kind', 'owner', 'group', 'filePermissions', 'content', 'to'],
                    'additionalProperties': False,
                },
                {
                    'type': 'object',
                    'properties': {
                        'kind': {'enum': ['EnsureDir']},
                        'owner': {'type': 'string' },
                        'group': {'type': 'string' },
                        'dirPermissions': {'type': 'string' },
                        'dir': {'type': 'string' },
                    },
                    'required': ['kind', 'owner', 'group', 'dirPermissions', 'dir'],
                    'additionalProperties': False,
                },
            ],
        },
    }

    def children(self, service):
        for child_name, child in service.iteritems():
            if child_name in {'environment', 'infrastructure', 'outputs'}:
                continue
            yield child_name, child

    def validateService(self, ctx, service_name, service):
        ctx = ctx + [service_name]
        validate.obj_field_opt(ctx, service, 'outputs', validate.is_string_map)
        validate.obj_field_opt(ctx, service, 'infrastructure', 'object')

    def fullName(self, ctx, service_name):
        return '-'.join(ctx + [service_name])

    def preprocess(self, ctx, service_name, service):
        def recursive_update(c):
            if isinstance(c, dict):
                return {
                    recursive_update(k): recursive_update(v)
                    for k, v in c.iteritems()
                }
            elif isinstance(c, list):
                return [recursive_update(v) for v in c]
            elif isinstance(c, basestring):
                return self.translateSelfName(self.fullName(ctx, service_name), c)
            else:
                return c
        return {
            'environment': service.get('environment', 'default'),
            'infrastructure': recursive_update(service.get('infrastructure',{})),
            'outputs': recursive_update(service.get('outputs', {})),
        }

    def compileStartupScript(self, cmds, bootCmds):
        lines = []
        lines.append('if [ ! -r /micromanage_instance_initialized ] ; then')
        for cmd in cmds:
            lines += compile_command_to_bash(cmd)
        lines.append('touch /micromanage_instance_initialized')
        lines.append('fi')
        for cmd in bootCmds:
            lines += compile_command_to_bash(cmd)
        return '\n'.join(lines)

    _selfNameRegex = re.compile(r'\$\{-\}')

    # Convert ${-} to the name of the service
    def translateSelfName(self, full_name, v):
        return self._selfNameRegex.sub(full_name, v)
    
    def compileProvider(self, environment_name, environment):
        raise NotImplementedError("%s has no override" % self.__class__.__name__)
    
    def getBuildArtefacts(self, environment, ctx, service):
        raise NotImplementedError("%s has no override" % self.__class__.__name__)

    def compile(self, ctx, service_name, service, barts):
        raise NotImplementedError("%s has no override" % self.__class__.__name__)
