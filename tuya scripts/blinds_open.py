import os
import tinytuya
import json

DEVICE_ID = os.environ.get('DEVICE_ID')
DEVICE_IP = os.environ.get('DEVICE_IP')
LOCAL_KEY = os.environ.get('DEVICE_LOCAL_KEY')
VERSION = os.environ.get('DEVICE_VERSION')
DPS = 1

d = tinytuya.OutletDevice(DEVICE_ID, DEVICE_IP, LOCAL_KEY)
d.set_version(VERSION)
d.set_socketPersistent(False)
d.set_value(DPS, 'open')

data = d.status()
result = {
    "state": data['dps']['1'],
    "battery": data['dps']['13']
}
print(json.dumps(result))




