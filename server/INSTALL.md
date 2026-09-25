# Installing the dashboard server on a machine

Step-by-step for whoever sets this up on the always-on home machine (a
person or a Claude Code session there). Background on what it is:
`README.md` next to this file.

**Goal:** InfluxDB + Grafana running in Docker on a machine that is on the
**same WiFi/LAN as the desk display** (an ESP32), reachable on ports 34898
(InfluxDB, the display writes there) and 34897 (the dashboard). The ports
are deliberately not the defaults (8086/3000). At the end, hand back
two values: the machine's **LAN IP** and the **InfluxDB token**.

Nothing here touches the ESP32 firmware; that side is done from the
desk-display repo on the dev machine once the two values are known.

## 1. Check prerequisites

```sh
docker --version && docker compose version   # both must work
ip -4 -o addr show | awk '{print $2, $4}'    # find the LAN address
```

- The LAN IP is the one on the home network (typically `192.168.x.x` on
  the ethernet/WiFi interface), **not** `tailscale0` (100.x), `docker0` or
  `br-*`. The ESP32 can't use Tailscale.
- Check the ports are free: `ss -ltn | grep -E ":(34897|34898)\b"` should
  print nothing. If something already uses them, stop and ask the user
  (the ports are set in `docker-compose.yml`; changing the InfluxDB one
  means changing `METRICS_PORT` in the firmware's `main/metrics.c` too).

## 2. Get the files

Only the `server/` folder is needed. The repo is public:

```sh
git clone --depth 1 https://github.com/Grigoriym/esp32-desk-display.git ~/esp32-desk-display
cd ~/esp32-desk-display/server
```

(If the user prefers another location or already keeps compose stacks in a
certain folder, put it there instead: the folder only needs `server/`'s
contents.)

## 3. Create `.env`

```sh
cp .env.example .env
```

Fill in both values in `.env`:

- `INFLUX_TOKEN`: if the user gave you a token (e.g. the one the display is
  already flashed with), use it. Otherwise generate one:
  `openssl rand -hex 32`.
- `INFLUX_ADMIN_PASSWORD`: any password, e.g. `openssl rand -hex 12`.
  It's the login for Grafana's and InfluxDB's admin UIs.

`.env` is gitignored; never commit it.

## 4. Start it

```sh
docker compose up -d
docker compose ps        # both influxdb and grafana: running
```

The first start creates the `desk` org and bucket and the token from
`.env`. Those settings only apply on the **first** start: if `.env` is
changed later, the old token stays valid and the new one isn't created.
Starting over means `docker compose down -v` (deletes all stored data).

## 5. Verify

Run from this directory (reads the token from `.env`):

```sh
. ./.env
curl -s -o /dev/null -w "influx %{http_code}\n" localhost:34898/health        # 200
curl -s -o /dev/null -w "grafana %{http_code}\n" localhost:34897/api/health   # 200

# A test write with the token, exactly like the display does it (expect 204),
# then delete it again (expect 204):
curl -s -o /dev/null -w "write %{http_code}\n" -X POST \
  "http://localhost:34898/api/v2/write?org=desk&bucket=desk&precision=s" \
  -H "Authorization: Token $INFLUX_TOKEN" --data-binary 'indoor,device=test temp_c=20'
curl -s -o /dev/null -w "delete %{http_code}\n" -X POST \
  "http://localhost:34898/api/v2/delete?org=desk&bucket=desk" \
  -H "Authorization: Token $INFLUX_TOKEN" -H 'Content-Type: application/json' \
  -d '{"start":"2020-01-01T00:00:00Z","stop":"2100-01-01T00:00:00Z","predicate":"device=\"test\""}'
```

Then check it's reachable **from the LAN**, not just locally: from another
device on the home network (or ask the user to open it on their phone),
`http://<LAN IP>:34897` should show the "Desk display" dashboard without a
login. If it only works locally, a firewall is blocking it:

```sh
sudo ufw status            # if active:
sudo ufw allow from 192.168.0.0/16 to any port 34898 proto tcp
sudo ufw allow from 192.168.0.0/16 to any port 34897 proto tcp
```

(Use the actual home subnet. Don't open these ports to the internet: the
dashboard has anonymous read access and the write API is plain HTTP.)

## 6. Hand back

Tell the user, for the desk-display session on the dev machine:

- `METRICS_HOST` = the LAN IP from step 1
- `METRICS_TOKEN` = `INFLUX_TOKEN` from `.env`

Those go into the gitignored `main/metrics_secrets.h` there, then the
firmware is rebuilt and flashed. The display's boot log then shows
`metrics: upload OK` about a minute after start, and the dashboard fills
in from then on.

Recommend a **fixed IP** for this machine (a DHCP reservation in the
router): the IP is compiled into the firmware, so if it changes the
display silently stops uploading.

## Maintenance

- Update the images: `docker compose pull && docker compose up -d`
  (versions are pinned to `influxdb:2.9` and `grafana/grafana:13.2` in
  `docker-compose.yml`; data survives in named volumes).
- Logs: `docker compose logs -f grafana` / `influxdb`.
- Dashboard changes: edit `grafana/make_dashboard.py` and run it
  (`python3 grafana/make_dashboard.py`, regenerates
  `grafana/dashboards/desk.json`, Grafana reloads it within ~10 s); edits
  made in the Grafana UI are not saved for this dashboard.
- Stop: `docker compose down` (keeps data); `down -v` wipes it.
