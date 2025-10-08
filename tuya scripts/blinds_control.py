import os
import tinytuya
import json
import time
import subprocess

DEVICE_ID = os.environ.get('DEVICE_ID')
DEVICE_IP = os.environ.get('DEVICE_IP')
LOCAL_KEY = os.environ.get('DEVICE_LOCAL_KEY')
VERSION = os.environ.get('DEVICE_VERSION')
DPS = 1


# d = tinytuya.OutletDevice(DEVICE_ID, DEVICE_IP, LOCAL_KEY)
d = tinytuya.CoverDevice(DEVICE_ID, DEVICE_IP, LOCAL_KEY)
d.set_version(VERSION)
d.set_socketPersistent(False)

def get_status():
    data = d.status()
    result = {
        "state": data['dps']['1'],
        "battery": data['dps']['13']
    }
    print(json.dumps(result))

while True:
    print("\n1. Open")
    print("2. Close")
    print("3. Status")
    print("4. Exit")
    choice = input("Choose (1-4): ").strip()
    if choice == '1':
        # d.set_value(DPS, 'open')
        d.open_cover(switch=1)
    elif choice == '2':
        # d.set_value(DPS, 'close')
        d.close_cover(switch=1)
    elif choice == '3':
        get_status()
    elif choice == '4':
        break
    else:
        print("Invalid choice.")