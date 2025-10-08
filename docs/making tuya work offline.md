# Running Tuya Blinds Locally (Offline Helper Guide)

This guide walks you through extracting the dynamic Tuya local key, preparing a minimal Debian VM, deploying the helper Python scripts plus a tiny PHP HTTP wrapper, and invoking open / close / status commands over your LAN. Everything is in English and expands on the raw step list you provided.

> Scope: This is a pragmatic, minimal setup focused on a single Tuya blinds device using the official Tuya Cloud only to (re)discover the *current* local key. Once you have the key you operate locally. Whenever the device changes Wi‑Fi network (or sometimes after a reset / re-pair) the local key can change, so you must refresh it again via the Tuya Cloud API Explorer.

---

## 1. Prerequisites

Hardware / environment:
- A Tuya-based blinds controller already paired in the Tuya (Smart Life / Tuya Smart) mobile app.
- Internet temporarily (only to retrieve the local key) — afterwards the blinds can be driven locally while both VM and device stay on the same LAN.
- A host machine (your PC) with virtualization (e.g. VirtualBox, VMware, Hyper-V) to run a minimal Debian VM.

Accounts / tools:
- Tuya Developer Account: https://iot.tuya.com
- Basic knowledge of SSH and running shell commands.
- (Optional) Git for versioning your helper scripts.

Security mindset:
- Treat the *Device ID* and especially the *Local Key* as secrets. Do **not** commit them to a public repository.
- Regenerate keys only when needed; store them in a `.env` or a root-readable config file if you script automation.

---

## 2. Get (or Refresh) the Tuya Local Key

1. Log in to the Tuya IoT Platform and create a Cloud Project.
   - Choose the correct Data Center that matches the region of your mobile app account (critical — an incorrect region means empty device lists).
2. Link your existing app account to the project (App Link section) so the project can see your devices.
3. Locate the target blinds Device ID (sometimes called `devId` or `device_id`). Copy it.
4. Open the API Explorer in the Tuya IoT console.
5. Go to: Device Management → "Query Device Details in Bulk" (or similarly named endpoint; Tuya occasionally renames UI labels).
6. Paste your Device ID and execute the request.
7. In the JSON response locate `local_key`.

Notes:
- The local key changes if the device is re-provisioned or switches to another router / SSID. Each time that happens, repeat the above to refresh it.
- If the API returns nothing, double‑check data center alignment and that the device is *online* in the mobile app.

---

## 3. Minimal Debian VM Setup

Suggested VM resources (worked in testing):
- vCPUs: 1–2
- RAM: 2 GB
- Disk: 10 GB

### 3.1 Install Base System
Download the current Debian netinst ISO and perform a minimal installation (no desktop environment needed). After first boot:

```bash
sudo apt update && sudo apt -y upgrade
sudo apt install -y python3 python3-pip python3-venv curl
```

(Optional) Install `pipx` for isolated tool management:

```bash
python3 -m pip install --user pipx
python3 -m pipx ensurepath
```

### 3.2 Install TinyTuya (if you plan to expand)
Your current scripts may not depend directly on TinyTuya, but it is a common base for local control & debugging:

```bash
pip install tinytuya
```

### 3.3 Web Server Choice
You can use either:
- **Caddy** (simple, automatic headers, single binary), or
- **Apache2** (traditional LAMP-style), or
- **Nginx + php-fpm** (not covered here for brevity).

For Caddy (recommended for minimal friction):

```bash
sudo apt install -y debian-keyring debian-archive-keyring apt-transport-https
sudo apt install -y caddy php php-cli php-curl
```

For Apache2 instead:

```bash
sudo apt install -y apache2 libapache2-mod-php php php-cli php-curl
```

---

## 4. Deploy the Tuya Helper Scripts

Files referenced (from this repository):
- `tuya scripts/index.php`
- `tuya scripts/blinds_open.py`
- `tuya scripts/blinds_close.py`
- `tuya scripts/blinds_status.py`
- (Optional) `blinds_control.py`, `socket_listen.py`, `check_ports.py` for advanced debugging.

### 4.1 Destination Directory
Assuming Caddy or Apache is serving `/var/www/html`:

```bash
sudo rm -rf /var/www/html/*
sudo mkdir -p /var/www/html
cd /var/www/html
```

Copy the Python scripts and `index.php` into this directory. Ensure execution bits:

```bash
sudo chmod +x *.py
```

### 4.2 Shebang
Add the shebang line to the very top of every Python file (already present in your instructions but verify):

```python
#!/usr/bin/env python3
```

This ensures the web server (via `shell_exec`) invokes the correct interpreter.

### 4.3 (Optional) Python Virtual Environment

If you need additional Python dependencies:
```bash
python3 -m venv /var/www/html/venv
source /var/www/html/venv/bin/activate
pip install --upgrade pip
pip install tinytuya
deactivate
```

Your `index.php` already calls scripts inside `/var/www/html/venv/bin/python` — make sure that path exists (create the venv before calling the scripts). If you prefer system Python, adjust the PHP commands accordingly.

### 4.4 PHP Wrapper Explanation

The provided `index.php` inspects the `cmd` query parameter and dispatches to one of three Python scripts:

```
/?cmd=open     → blinds_open.py
/?cmd=close    → blinds_close.py
/?cmd=status   → blinds_status.py
```

Response policy:
- On success: echoes raw JSON produced by the Python script (Content-Type: application/json)
- On unknown command: HTTP 400 with `{"error":"Unknown command"}`
- On internal failure (no output): HTTP 500 with `{"error":"No output from Python"}`

Make sure each Python script prints a valid JSON object (e.g. `{"success":true,"action":"open"}`) so the HTTP caller receives structured data.

---

## 5. Supplying Device Credentials to Scripts

Depending on your implementation style, the Python scripts may need:
- Device ID (e.g. `DEVICE_ID`)
- Local Key
- IP address (if you bypass local discovery)

Best practice: put secrets in a file not served publicly:

```
/var/www/html/.env (600 root:root)

DEVICE_ID=xxxxxxxxxxxxxxxxxxxx
LOCAL_KEY=xxxxxxxxxxxxxxxx
DEVICE_IP=192.168.1.50
```

Then have each Python script load it, e.g.:
```python
from pathlib import Path
for line in Path('.env').read_text().splitlines():
	if '=' in line:
		k,v = line.split('=',1)
		globals()[k] = v
```

Never expose `.env` via the web server (default configs usually prevent dotfile serving; verify anyway).

---

## 6. Testing With curl

Replace `<SERVER_IP>` below with the LAN IP of your Debian VM.

```bash
curl http://<SERVER_IP>/?cmd=status
curl http://<SERVER_IP>/?cmd=open
curl http://<SERVER_IP>/?cmd=close
```

Expected outcomes:
- Each returns HTTP 200 and a JSON body describing the action / state.
- Running `status` after `open` or `close` reflects the new position (subject to device reporting latency).

Troubleshooting:
- 500 error + `No output from Python`: ensure scripts are executable and shebang present; check `php -v` and error logs (`journalctl -u caddy` or Apache's `error.log`).
- 400 error: double-check `cmd` spelling.
- Hang / timeout: network isolation or firewall blocking outbound traffic to device; verify that the VM and device share the same subnet.

---

## 7. Updating the Local Key After Network Changes

If you move the device to another router or change Wi‑Fi credentials:
1. Re-add / confirm device in mobile app until it shows Online.
2. Re-run the API Explorer "Query Device Details in Bulk".
3. Update `LOCAL_KEY` in your `.env` (or directly in script if hardcoded).
4. (Optional) Restart the web service for cleanliness.

Automating refresh is usually unnecessary unless you rotate networks frequently.

---

## 8. Hardening & Operational Tips

- Firewall: restrict inbound HTTP to your trusted management subnet (e.g., via `ufw allow from 192.168.1.0/24 to any port 80`).
- TLS: optionally front with a reverse proxy offering HTTPS (Caddy can auto‑TLS for public domains; for LAN use self-signed or disable if not needed).
- Logging: wrap Python scripts to log actions to `/var/log/tuya-blinds.log` for audit.
- Systemd service: If you switch to a pure Python HTTP API later, create a `tuya-blinds.service` for reliability rather than relying on PHP.

---

## 9. Example Minimal Python Script (Conceptual)

Below is a *conceptual* structure if you later refactor to a single `blinds_control.py` instead of three one-off scripts:

```python
#!/usr/bin/env python3
import json, sys
ACTION = sys.argv[1] if len(sys.argv) > 1 else 'status'

def perform(action: str):
	# Placeholder; integrate TinyTuya or raw socket protocol here
	if action not in {'open','close','status'}:
		return {"success": False, "error": "invalid action"}
	# Implement device call...
	return {"success": True, "action": action}

print(json.dumps(perform(ACTION)))
```

Then in PHP you could dispatch as: `shell_exec("/var/www/html/venv/bin/python /var/www/html/blinds_control.py open 2>&1")` etc.

---

## 10. Common Errors & Fixes

| Symptom | Likely Cause | Resolution |
|---------|--------------|-----------|
| Empty JSON / 500 | Python printed nothing | Add explicit `print(json.dumps(...))` |
| 500: No output | Wrong interpreter path | Verify venv exists or adjust path to `/usr/bin/python3` |
| Device unresponsive | Wrong local key | Re-query API Explorer; update .env |
| 403 in API Explorer | Project lacks permission | Add the needed API to Project (Cloud → Project → API Group authorization) |
| No devices shown | Wrong data center | Recreate project selecting correct region |

---

## 11. Quick Checklist (TL;DR)

1. Create Tuya developer account & project (correct data center).
2. Link app account; copy Device ID.
3. API Explorer → Query Device Details in Bulk → copy `local_key`.
4. Debian VM: install Python + (optionally) Caddy or Apache.
5. Copy scripts + `index.php` into `/var/www/html`; add shebangs; make them executable.
6. (Optional) Create venv; install dependencies.
7. Store Device ID + Local Key securely (e.g., `.env`).
8. Call with `curl http://<SERVER_IP>/?cmd=status|open|close`.
9. Refresh local key after network changes.
10. Harden firewall & keep secrets out of version control.

---

## 12. Example curl Session

```bash
$ curl http://192.168.1.120/?cmd=status
{"success":true,"state":"open","percent":100}

$ curl http://192.168.1.120/?cmd=close
{"success":true,"action":"close","result":"issued"}

$ curl http://192.168.1.120/?cmd=open
{"success":true,"action":"open","result":"issued"}
```

Your actual JSON keys depend on what each Python script prints.

---

## 13. Where to Go Next

- Integrate this into the existing WebSocket infrastructure (see `bunServer.js`) so blinds commands can be issued from the main dashboard UI.
- Convert to a unified REST endpoint in the PHP backend for consistent logging.
- Add retry / timeout logic in Python scripts if the device intermittently drops packets.

---

## 14. Summary

You now have a reproducible offline control path:
Tuya Cloud (only to fetch refreshed local key) → Local Debian VM hosting Python scripts → HTTP wrapper (PHP/Caddy) → Device over LAN.

All commands (`open`, `close`, `status`) return JSON and can be scripted or integrated into pipelines. Refresh the local key only after provisioning changes, and otherwise enjoy offline operation.

Happy hacking!

