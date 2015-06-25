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
import os
import re

# E.g. replace Simon's cat with 'Simon'\''s cat'.
def escape(s):
    return "'%s'" % s.replace("'", "'\"'\"'")


NAME_CHARS = ([chr(i) for i in range(ord('0'), ord('9'))]
              + [chr(i) for i in range(ord('a'), ord('z'))])


class Service:

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
                        'kind': {'type': 'string', 'pattern': '^CopyFile$'},
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
                        'kind': {'type': 'string', 'pattern': '^LiteralFile$'},
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
                        'kind': {'type': 'string', 'pattern': '^EnsureDir$'},
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

    def encodeImageHash(self, n):
        n = n % (2**63)
        if n < 0:
            n += 2**63
        radix = len(NAME_CHARS)
        s = ""
        for i in range(12):
            d = n % radix
            s = NAME_CHARS[d] + s
            n /= radix
        return s

    def hashString(self, s):
        r = long(hashlib.sha1(s).hexdigest(), 16)
        if r < 0:
            r += 2**63
        return r

    def fileGlob(self, given_glob, to, prefix):
        dirs = []
        files = []
        lp = len(prefix)
        for f in glob.glob(given_glob):
            if os.path.isdir(f):
                more_files = self.fileGlob('%s/*' % f, to, prefix)
                files += more_files
            else:
                files.append((f, to + f[lp:]))
        return files        

    def hashProvisioners(self, cmds):
        hash_code = 0l
        for cmd in cmds:
            if isinstance(cmd, basestring):
                hash_code ^= self.hashString(cmd)
            elif cmd['kind'] == 'LiteralFile':
                hash_code ^= self.hashString(cmd['content'])
                hash_code ^= self.hashString(cmd['to'])
                hash_code ^= self.hashString(cmd['filePermissions'])
                hash_code ^= self.hashString(cmd['owner'])
                hash_code ^= self.hashString(cmd['group'])
            elif cmd['kind'] == 'CopyFile':
                # TODO(dcunnin): Scan these files, compute hash
                hash_code ^= self.hashString(cmd['from'])
                hash_code ^= self.hashString(cmd['to'])
                hash_code ^= self.hashString(cmd['dirPermissions'])
                hash_code ^= self.hashString(cmd['filePermissions'])
                hash_code ^= self.hashString(cmd['owner'])
                hash_code ^= self.hashString(cmd['group'])
                raise RuntimeError('CopyFile not supported in image')
            elif cmd['kind'] == 'EnsureDir':
                hash_code ^= self.hashString(cmd['dir'])
                hash_code ^= self.hashString(cmd['dirPermissions'])
                hash_code ^= self.hashString(cmd['owner'])
                hash_code ^= self.hashString(cmd['group'])
            else:
                raise RuntimeError('Did not recognize image command kind: ' + cmd['kind'])
        return hash_code

    def compileCommandToBash(self, cmd):
        if isinstance(cmd, basestring):
            return [cmd]
        elif cmd['kind'] == 'LiteralFile':
            return [
                'echo -n %s > %s' % (escape(cmd['content']), escape(cmd['to'])),
                'chmod -v %s %s' % (cmd['filePermissions'], escape(cmd['to'])),
                'chown -v %s.%s %s' % (cmd['owner'], cmd['group'], escape(cmd['to'])),
            ]
        elif cmd['kind'] == 'CopyFile':
            files = self.fileGlob(cmd['from'], cmd['to'], os.path.dirname(cmd['from']))
            dirs = set([os.path.dirname(f[1]) for f in files]) - {cmd['to']}
            lines = []
            for d in dirs:
                lines += [
                    'mkdir -v -p %s' % escape(d),
                    'chmod -v %s %s' % (cmd['dirPermissions'], escape(d)),
                    'chown -v %s.%s %s' % (cmd['owner'], cmd['group'], escape(d)),
                ]
            for f in files:
                with open (f[0], "r") as stream:
                    content = stream.read()
                lines += [
                    'echo -n %s > %s' % (escape(content), escape(f[1])),
                    'chmod -v %s %s' % (cmd['filePermissions'], escape(f[1])),
                    'chown -v %s.%s %s' % (cmd['owner'], cmd['group'], escape(f[1])),
                ]
            return lines
        elif cmd['kind'] == 'EnsureDir':
            return [
                'mkdir -v -p %s' % escape(cmd['dir']),
                'chmod -v %s %s' % (cmd['dirPermissions'], escape(cmd['dir'])),
                'chown -v %s.%s %s' % (cmd['owner'], cmd['group'], escape(cmd['dir'])),
            ]
        else:
            raise RuntimeError('Did not recognize image command kind: ' + cmd['kind'])

    def compileStartupScript(self, cmds):
        lines = []
        for cmd in cmds:
            lines += self.compileCommandToBash(cmd)
        return '\n'.join(lines)

    def compileProvisioners(self, cmds):
        def shell_provisioner(lines):
            return {
                'type': 'shell',
                'execute_command': "{{ .Vars }} sudo -E /bin/bash '{{ .Path }}'",
                'inline': lines,
            }
        provs = []
        for cmd in cmds:
            provs.append(shell_provisioner(self.compileCommandToBash(cmd)))
        return provs

    _selfNameRegex = re.compile(r'\$\{-\}')

    # Convert ${-} to the name of the service
    def translateSelfName(self, service_name, v):
        return self._selfNameRegex.sub(service_name, v)
    
    def compile(self, config, service_name):
        raise NotImplementedError("%s has no override" % self.__class__.__name__)
