# Troubleshooting: Frames Not Saving

## Problem

WebRTC connection establishes successfully, RTP forward is configured, but the session drops immediately due to mDNS resolution errors. No frames are saved because the connection doesn't stay alive long enough.

### Symptoms
- Browser log shows: "WebRTC connected" → "RTP forward ok" → "WebRTC disconnected"
- Janus log shows: "WebRTC media is now available" → "Error resolving mDNS address" → "No WebRTC media anymore"
- Capture container times out waiting for RTP packets
- No frames saved to `out/frames/`

### Root Cause

Chrome uses mDNS (.local addresses) for ICE candidates for privacy. Janus cannot resolve these mDNS addresses, causing ICE connectivity check failures and immediate session teardown.

## Solution 1: Launch Chrome with mDNS Disabled (Recommended)

Close all Chrome/Chromium instances and launch with this flag:

```bash
# Linux
chromium --disable-features=WebRtcHideLocalIpsWithMdns http://localhost:8000/demo.html

# Or for Chrome
google-chrome --disable-features=WebRtcHideLocalIpsWithMdns http://localhost:8000/demo.html

# macOS
/Applications/Google\ Chrome.app/Contents/MacOS/Google\ Chrome --disable-features=WebRtcHideLocalIpsWithMdns

# Windows
"C:\Program Files\Google\Chrome\Application\chrome.exe" --disable-features=WebRtcHideLocalIpsWithMdns
```

Then open http://localhost:8000/demo.html and test webcam/screen share.

## Solution 2: Install mDNS Resolution in Janus (Advanced)

This requires rebuilding the Janus container with Avahi/mDNS support, which is complex due to D-Bus requirements.

## Solution 3: Use Firefox Instead

Firefox handles mDNS differently and may work better with Janus:

```bash
firefox http://localhost:8000/demo.html
```

## Verification

After applying a solution, you should see:

1. **In browser log:**
   - "WebRTC connected"
   - "RTP forward ok"
   - Connection stays alive (no immediate "WebRTC disconnected")

2. **In Janus logs:**
   ```bash
   docker compose logs janus | grep "rtp-sample"
   ```
   Should show: `[rtp-sample] New video stream! (#1, ssrc=...)`

3. **In capture logs:**
   ```bash
   docker compose logs capture
   ```
   Should NOT show timeout errors

4. **Frames saved:**
   ```bash
   ls -l out/frames/
   ```
   Should show `frame_00000001.png`, `frame_00000002.png`, etc.

## Testing

```bash
# Start containers
./manage.sh start

# Start a simple HTTP server (if not already running)
python3 -m http.server 8000

# Launch Chrome with mDNS disabled
chromium --disable-features=WebRtcHideLocalIpsWithMdns http://localhost:8000/demo.html

# Click "Start Webcam" and let it run for 10+ seconds
# Check output directory
ls -lh out/frames/
```
