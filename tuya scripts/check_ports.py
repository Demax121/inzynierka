import os
import socket
DEVICE_IP = os.environ.get('DEVICE_IP')
for port in (6666, 6667, 6668, 7000):
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.settimeout(2)
    try:
        s.connect((DEVICE_IP, port))
        print(f"✅ Port {port} open on {DEVICE_IP}")
        s.close()
    except Exception as e:
        print(f"❌ Port {port} closed on {DEVICE_IP} ({e})")