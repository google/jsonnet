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

import re

import cmds as cmds_lib
import validate

class Service(object):

    def validateCmds(self, root, path):
        cmds = validate.array(root, path, validate.is_any_type({'string', 'object'}), [])
        for i, cmd in enumerate(cmds):
            cmd_path = path + [i]
            if isinstance(cmd, basestring):
                # Any string will do for validation purposes.
                pass
            elif isinstance(cmd, dict):
                kinds = {'CopyFile', 'LiteralFile', 'EnsureDir'}
                kind = validate.path_val(root, cmd_path + ['kind'], validate.is_any_value(kinds))
                if kind == 'CopyFile':
                    fields = {'owner', 'group', 'dirPermissions', 'filePermissions', 'from', 'to'}
                    for f in fields:
                        validate.path_val(root, cmd_path + [f], 'string')
                    validate.obj_only(root, cmd_path, fields | {'kind'})
                elif kind == 'LiteralFile':
                    fields = {'owner', 'group', 'filePermissions', 'content', 'to'}
                    for f in fields:
                        validate.path_val(root, cmd_path + [f], 'string')
                    validate.obj_only(root, cmd_path, fields | {'kind'})
                elif cmd['kind'] == 'EnsureDir':
                    fields = {'owner', 'group', 'dirPermissions', 'dir'}
                    for f in fields:
                        validate.path_val(root, cmd_path + [f], 'string')
                    validate.obj_only(root, cmd_path, fields | {'kind'})
                else:
                    raise RuntimeError('Internal error: %s' % kind)
            else:
                raise RuntimeError('Internal error: %s' % type(cmd))

    def validateImage(self, root, path):
        # Superclasses override this method and validate specific image attributes.
        # Byt here we can do the cmds.
        self.validateCmds(root, path + ['cmds'])

    def children(self, service):
        for child_name in sorted(service.keys()):
            if child_name in {'environment', 'infrastructure', 'outputs'}:
                continue
            yield child_name, service[child_name]

    def validateInfrastructure(self, root, service_name, path):
        infrastructure = validate.path_val(root, path, 'object', {})
        for rtype in sorted(infrastructure.keys()):
            resources = validate.path_val(root, path + [rtype], 'object', {})
            for resource_name in sorted(resources.keys()):
                if not resource_name.startswith(service_name):
                    validate.err(path,
                                 'Expected "%s" to be prefixed by "%s"' % (resource_name, service_name),
                                 'Resource names must be prefixed by the name of the service defining them')

    def validateService(self, root, path):
        validate.path_val(root, path + ['outputs'], validate.is_string_map, {})
        self.validateInfrastructure(root, self.fullName(path), path + ['infrastructure'])

    def fullName(self, path):
        return '-'.join(path)

    def preprocess(self, service):
        return {
            'environment': service.get('environment', 'default'),
            'infrastructure': service.get('infrastructure',{}),
            'outputs': service.get('outputs', {}),
        }

    def compileStartupScript(self, cmds, bootCmds):
        lines = []
        lines.append('#!/bin/bash')
        lines.append('if [ ! -r /micromanage_instance_initialized ] ; then')
        for cmd in cmds:
            lines += cmds_lib.compile_command_to_bash(cmd)
        lines.append('touch /micromanage_instance_initialized')
        lines.append('fi')
        for cmd in bootCmds:
            lines += cmds_lib.compile_command_to_bash(cmd)
        return '\n'.join(lines)
    
