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

import collections
import copy
import datetime
import json
import jsonschema
import re
import sys
import shutil
import signal
import subprocess
import sys
import tempfile

import _jsonnet

from service import *
from service_google import *

from util_google import *


service_kinds = {
    'Google': GoogleService(),
}
    
blueprint_schema = {
    '$schema': 'http://json-schema.org/schema#',
    'type': 'object',
    'properties' : {
        'environments': {
            'type': 'object',
            'additionalProperties': GoogleService().environmentSchema,
            #'additionalProperties': {
            #    'oneOf': [
            #        GoogleService().environmentSchema,
            #    ],
            #}
        },
    },
    'required': ['environments'],
    'additionalProperties': {
        'oneOf': [
            GoogleService().serviceSchema,
        ],
    },
}

class ConfigError (Exception):
    pass

def path_to_string(path):
    arr = []
    for x in path:
        if isinstance(x, int):
            arr += '[%d]' % x
        else:
            arr += '.%s' % x
    return ''.join(arr)

def config_check(config):
    try:        
        jsonschema.validate(config, blueprint_schema)
    except jsonschema.ValidationError as e:
        raise ConfigError('Config error: %s in $%s' % (e.message, path_to_string(e.absolute_path)))

    # Check that service environments all exist
    for service_name, service in config.iteritems():
        if service_name != 'environments':
            environment_name = service.get('environment', 'default')
            if not config['environments'].get(environment_name):
                raise ConfigError('Config error: No such environment %s in service %s' % (environment_name, service_name))
            

def config_load(filename):
    try:
        text = _jsonnet.evaluate_file(filename, max_trace=100)
    except RuntimeError as e:
        # Error from Jsonnet
        sys.stderr.write(e.message)
        sys.stderr.write('\n')
        sys.exit(1)

    config = json.loads(text)
    try:
        config_check(config)
    except ConfigError as e:
        sys.stderr.write(e.message)
        sys.stderr.write('\n')
        sys.exit(1)
    return config


# Merge b into a, prefering b on conflicts
def merge_into(a, b):
    for k, v in b.iteritems():
        a[k] = v


# Build a Terraform and packer config for all services
def compile(config):
    extras, packers, tfs = {}, {}, {}
    for environment_name, environment in config['environments'].iteritems():
        artefacts = service_kinds[environment['kind']].compileProvider(config, environment_name)
        merge_into(extras, artefacts['extras'])
        merge_into(tfs, artefacts['tfs'])

    for service_name, service in config.iteritems():
        if service_name != 'environments':
            artefacts = service_kinds[service['kind']].compile(config, service_name)
            merge_into(extras, artefacts['extras'])
            merge_into(packers, artefacts['packers'])
            merge_into(tfs, artefacts['tfs'])

    return extras, packers, tfs


def jsonstr(v):
    return json.dumps(v, sort_keys=True, indent=4, separators=(',', ': '))


def confirmation_dialog(msg):
    sys.stdout.write('%s  [y/N]:  ' % msg)
    while True:
        choice = raw_input().lower()
        if choice == '':
            choice = 'n'
        if choice in ['y', 'n']:
            break
        sys.stdout.write('Please press either y or n, then hit enter:  ')
    return choice


def output_generate(config, check_environment):
    extras, packers, tfs = compile(config)
    dirpath = tempfile.mkdtemp()
    print 'Generated files are in %s' % dirpath

    buildables = []
    extra_files = []
    packer_files = []
    tf_files = []

    # Output extras
    for filename, string in extras.iteritems():
        dirfilename = '%s/%s' % (dirpath, filename)
        extra_files.append(dirfilename)
        with open(dirfilename, 'w') as f:
            f.write(string)

    image_cache = {}  # Map project name to images

    # Output packer configs
    for filename, image in packers.iteritems():
        dirfilename = '%s/%s' % (dirpath, filename)
        packer_files.append(dirfilename)
        with open(dirfilename, 'w') as f:
            f.write(jsonstr(image))

        if check_environment:
            # TODO(dcunnin): Handle AWS
            assert image['builders'][0]['type'] == 'googlecompute'
            project = image['builders'][0]['project_id']
            image_name = image['builders'][0]['image_name']
            print 'Checking if image exists: %s/%s' % (project, image_name)
            service_account_key_file = '%s/%s' % (dirpath, image['builders'][0]['account_file'])
            if project in image_cache:
                imgs = image_cache[project]
            else:
                imgs = [img[0] for img in google_get_images(project, service_account_key_file)]
                image_cache[project] = imgs
            if image_name not in imgs:
                buildables.append(dirfilename)

    # Output Terraform configs
    for filename, tf in tfs.iteritems():
        dirfilename = '%s/%s' % (dirpath, filename)
        tf_files.append(dirfilename)
        with open(dirfilename, 'w') as f:
            f.write(jsonstr(tf))

    return buildables, dirpath, extra_files, packer_files, tf_files


def output_delete(dirpath):
    shutil.rmtree(dirpath)


def action_blueprint(config, args):
    if args:
        sys.stderr.write('Action "blueprint" accepts no arguments, but got:  %s\n' % ' '.join(args))
        sys.exit(1)
    print(jsonstr(config))
        

def action_schema(config, args):
    if args:
        sys.stderr.write('Action "schema" accepts no arguments, but got:  %s\n' % ' '.join(args))
        sys.exit(1)
    print(jsonstr(blueprint_schema))
        

def action_generate_to_editor(config, args):
    if args:
        sys.stderr.write('Action "generate-to-editor" accepts no arguments, but got:  %s\n' % ' '.join(args))
        sys.exit(1)
    buildables, dirpath, extra_files, packer_files, tf_files = output_generate(config, False)
    command = [os.getenv('EDITOR')] + extra_files + packer_files + tf_files
    tf_process = subprocess.Popen(command)
    tf_process.wait()
    output_delete(dirpath)


def action_apply(config, args):
    if args:
        sys.stderr.write('Action "apply" accepts no arguments, but got:  %s\n' % ' '.join(args))
        sys.exit(1)

    buildables, dirpath, extra_files, packer_files, tf_files = output_generate(config, True)

    # Run packer
    # TODO(dcunnin): Do we need to handle ctrl+C better?
    #def signal_handler(signal, frame):
    #    print('You pressed Ctrl+C!')
    #    sys.exit(0)
    #signal.signal(signal.SIGINT, signal_handler)
    packer_processes = []
    for packer_file in buildables:
        command = ['packer', 'build', packer_file]
        logfilename = '%s.log' % packer_file
        print '%s > %s' % (' '.join(command), logfilename)
        with open(logfilename, 'w') as logfile:
            p = subprocess.Popen(command, stdout=logfile, cwd=dirpath)
            packer_processes.append(p)
    for p in packer_processes:
        exitcode = p.wait()
        if exitcode != 0:
            sys.stderr.write('Error from packer, aborting.\n')
            sys.exit(1)

    state_file = 'tf.state'

    command = ['terraform', 'plan',
               '-state', '%s/%s' % (os.getcwd(), state_file),
               '-detailed-exitcode',
               '-out', 'tf.plan']
    tf_process = subprocess.Popen(command, cwd=dirpath)
    plan_exitcode = tf_process.wait()
    if plan_exitcode == 0:
        pass  # Empty plan, nothing to do
    elif plan_exitcode == 2:
        choice = confirmation_dialog('Apply these changes?')

        if choice == 'y':
            command = ['terraform', 'apply', '-state', '%s/%s' % (os.getcwd(), state_file), 'tf.plan']
            tf_process = subprocess.Popen(command, cwd=dirpath)
            exitcode = tf_process.wait()
            if exitcode != 0:
                sys.stderr.write('Error from terraform apply, aborting.\n')
                sys.exit(1)
        else:
            print 'Not applying the changes.'
    else:
        sys.stderr.write('Error from terraform plan, aborting.\n')
        sys.exit(1)

    output_delete(dirpath)


def action_destroy(config, args):
    if args:
        sys.stderr.write('Action "apply" accepts no arguments, but got:  %s\n' % ' '.join(args))
        sys.exit(1)

    buildables, dirpath, extra_files, packer_files, tf_files = output_generate(config, False)
    command = ['terraform', 'destroy', '-force', '-state', '%s/tf.state' % os.getcwd()]
    tf_process = subprocess.Popen(command, cwd=dirpath)
    exitcode = tf_process.wait()
    if exitcode != 0:
        sys.stderr.write('Error from terraform, aborting.\n')
        sys.exit(1)
    output_delete(dirpath)



def action_image_gc(config, args):
    extras, packers, tfs = compile(config)
    used_images = collections.defaultdict(lambda: [])
    all_images = {}

    class UTC(datetime.tzinfo):
      def utcoffset(self, dt):
        return datetime.timedelta(0)
      def tzname(self, dt):
        return "UTC"
      def dst(self, dt):
        return datetime.timedelta(0)
    now = datetime.datetime.now(UTC())

    # Output packer configs
    for filename, image in packers.iteritems():
        # TODO(dcunnin): Handle AWS 
        assert image['builders'][0]['type'] == 'googlecompute'
        project = image['builders'][0]['project_id']
        image_name = image['builders'][0]['image_name']
        used_images[project].append(image_name)

        service_account_key_json = extras[image['builders'][0]['account_file']]
        service_account_key_json = json.loads(service_account_key_json)
        if project not in all_images:
            imgs = google_get_images_json_key(project, service_account_key_json)
            all_images[project] = imgs

    got_any = False
    # Delete all images older than X which are not currently in a version / module
    for project, imgs in all_images.iteritems():
        mmimgs = [img for img in imgs if img[0].startswith('micromanage-') and not img[0] in used_images[project]]
        for img in mmimgs:
            if (now - img[1]).days > 7:
                if not got_any:
                    got_any = True
                    print 'Execute the following commands to clean up images:'
                print 'gcloud --project=%s compute images delete -q %s  # %s days old' % (project, img[0],  (now - img[1]).days)
    if not got_any:
        print 'There were no images to clean up.'


actions = {
    'blueprint': action_blueprint,
    'schema': action_schema,
    'generate-to-editor': action_generate_to_editor,
    'apply': action_apply,
    'destroy': action_destroy,
    'image-gc': action_image_gc,
}


def print_usage(channel):
    channel.write("Usage: python micromanage.py <config.jsonnet> <action> <args>\n")
    channel.write("Available actions: %s\n" % ', '.join(actions.keys()))

if len(sys.argv) < 3:
    sys.stderr.write('Only %d cmdline param(s).\n' % (len(sys.argv) - 1))
    print_usage(sys.stderr)
    sys.exit(1)

config_file = sys.argv[1]
action = sys.argv[2]
args = sys.argv[3:]

if action not in actions:
    sys.stderr.write('Invalid action: "%s"' % action)
    print_usage(sys.stderr)
    sys.exit(1)

config = config_load(config_file)
actions[action](config, args)
