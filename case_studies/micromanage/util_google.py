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
import dateutil.parser

from oauth2client.client import flow_from_clientsecrets
from oauth2client.client import GoogleCredentials
from oauth2client.client import SignedJwtAssertionCredentials
from googleapiclient.discovery import build

def google_get_images(project, key_file):
    with open(key_file) as json_file:
        key_json = json.load(json_file)
    
    return google_get_images_json_key(project, key_json)


def google_get_images_json_key(project, key_json):
    credentials = SignedJwtAssertionCredentials(
        key_json['client_email'],
        key_json['private_key'],
        scope='https://www.googleapis.com/auth/compute')
        
    compute = build('compute', 'v1', credentials=credentials)
    images = compute.images().list(project=project).execute()
    return [(i['name'], dateutil.parser.parse(i['creationTimestamp'])) for i in images['items']]

