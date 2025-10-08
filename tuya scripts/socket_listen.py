import tinytuya
import os


DEVICE_ID = os.environ.get('DEVICE_ID')
DEVICE_IP = os.environ.get('DEVICE_IP')
LOCAL_KEY = os.environ.get('DEVICE_LOCAL_KEY')
VERSION = os.environ.get('DEVICE_VERSION')


d = tinytuya.OutletDevice(DEVICE_ID, DEVICE_IP, LOCAL_KEY)
d.set_version(VERSION)
d.set_socketPersistent(True)

print(" > Send Request for Status < ")
payload = d.generate_payload(tinytuya.DP_QUERY)
d.send(payload)

print(" > Begin Monitor Loop <")
while(True):
    # See if any data is available
    data = d.receive()
    print('Received Payload: %r' % data)

    # Send keyalive heartbeat
    print(" > Send Heartbeat Ping < ")
    payload = d.generate_payload(tinytuya.HEART_BEAT)
    d.send(payload)
