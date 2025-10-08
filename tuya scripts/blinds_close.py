import os
import tinytuya
import json
DEVICE_ID = "device_id_here"
DEVICE_IP = "device_local_ip_here"
LOCAL_KEY = "local_key_here"
VERSION = "3.1, 3.2, 3.3, 3.4" #to determinate the correct version use 
DPS = 1


d = tinytuya.OutletDevice(DEVICE_ID, DEVICE_IP, LOCAL_KEY)
d.set_version(VERSION)
d.set_socketPersistent(False)
d.set_value(DPS, 'close')

data = d.status()
result = {
    "state": data['dps']['1'],
    "battery": data['dps']['13']
}
print(json.dumps(result))




