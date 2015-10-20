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

import numbers
import re


class ConfigError (Exception):
    note = None
    pass

_KEYWORDS = {
    'import', 'importstr', 'function', 'self', 'super', 'assert', 'if', 'then',
    'else', 'for', 'in', 'local', 'tailstrict', 'true', 'false', 'null', 'error',
}

def _isidentifier(name):
    return name not in _KEYWORDS and re.match("[_A-Za-z_][_a-zA-Z0-9]*$", name)

_TYPE_FROM_STR = {
    'null': type(None),
    'bool': bool,
    'number': numbers.Number,
    'object': dict,
    'array': list,
    'string': basestring,
}

def _typeerr(v):
    if isinstance(v, basestring):
        return '"%s"' % v
    if isinstance(v, numbers.Number):
        return '%s' % v
    if isinstance(v, bool):
        return '%s' % v
    return _typestr(v)

def _typestr(v):
    if v is None:
        return 'null'
    if isinstance(v, basestring):
        return 'string'
    if isinstance(v, dict):
        return 'object'
    if isinstance(v, list):
        return 'array'
    if isinstance(v, numbers.Number):
        return 'number'
    if isinstance(v, bool):
        return 'bool'


def render_path(path):
    if isinstance(path, basestring):
        return path
    def aux(p):
        if isinstance(p, long):
            return '[%d]' % p
        return ('.%s' if _isidentifier(p) else '["%s"]') % p
    return '$' + ''.join([aux(p) for p in path])

def is_type(t):
    def aux(v):
        if _typestr(v) != t:
            return ' to have type %s (found %s)' % (t, _typeerr(v))
    return aux

def is_string_map(v):
    msg = is_type('object')(v)
    if msg:
        return msg
    for k, v2 in v.iteritems():
        if not isinstance(v2, basestring):
            return ' to be a string map'

def is_value(y):
    def check(x):
        if x != y:
            return 'to have value %s' % y
    return check

def _sanitize_func(func):
    if isinstance(func, basestring):
        return is_type(func)
    return func

def obj_field(path, obj, field, func):
    func = _sanitize_func(func)
    msg = ''
    if field in obj:
        msg = func(obj[field])
    if msg is not None:
        raise ConfigError('At %s, expected field "%s"%s' % (render_path(path), field, msg))
    return obj[field]

def obj_field_opt(path, obj, field, func, default=None):
    func = _sanitize_func(func)
    if field not in obj:
        return default
    msg = func(obj[field])
    if msg is not None:
        raise ConfigError('At %s, expected optional field "%s"%s' % (render_path(path), field, msg))
    return obj[field]

def obj_only(path, obj, fields):
    for f in obj:
        if f not in fields:
            raise ConfigError('At %s, didn\'t expect field%s' % (render_path(path), f))

def value(path, val, func):
    func = _sanitize_func(func)
    msg = func(val)
    if msg is not None:
        raise ConfigError('At %s, but expected it %s' % (render_path(path), msg))

