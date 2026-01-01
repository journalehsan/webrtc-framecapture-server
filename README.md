# WebRTC RTP Capture (Janus + FFmpeg + OpenCV)

This project avoids building Google libwebrtc from source. Instead, it uses:
- **Janus Gateway** to accept WebRTC from the browser.
- **RTP forward** from Janus to a capture container.
- **FFmpeg/libav** to receive/decode RTP.
- **OpenCV** in C++17 to save frames and optional MP4.

Audio is ignored.

## Architecture
- `janus` container: receives WebRTC, forwards video via RTP to the Docker network.
- `capture` container: decodes RTP with libavformat/libavcodec and writes frames with OpenCV.

Code layout:
- `src/ingest/RtpReceiver.*` FFmpeg-based RTP receiver
- `src/media/FrameWriter.*` OpenCV output
- `src/app/App.*` orchestration

## Quick start
```bash
./manage.sh setup
./manage.sh start
```

Serve `demo.html` locally (required for camera/screen permissions):
```bash
python3 -m http.server 8000
```
Then open `http://localhost:8000/demo.html` and click **Start Webcam** or **Start Screen Share**.

Outputs:
- PNG frames: `out/frames/frame_00000001.png`
- MP4 (optional): `out/capture.mp4`

## Demo flow (browser → Janus → RTP → capture)
1. Browser connects to Janus over HTTP or WebSocket.
2. Browser publishes a VP8 video stream to a VideoRoom.
3. The demo calls Janus `rtp_forward` so video is sent to `capture:5004`.
4. The capture service reads `/app/config/rtp.sdp` to decode RTP and writes frames.

## Janus configuration
Files:
- `janus/janus.jcfg`
- `janus/janus.plugin.videoroom.jcfg`

The VideoRoom plugin enables RTP forwarding and sets a default room. RTP forward uses:
- host: `capture`
- port: `5004`
- payload type: `96` (VP8)

## Capture configuration
Default RTP SDP: `config/rtp.sdp`
```
v=0
o=- 0 0 IN IP4 127.0.0.1
s=Janus RTP
c=IN IP4 0.0.0.0
t=0 0
m=video 5004 RTP/AVP 96
a=rtpmap:96 VP8/90000
a=recvonly
```

## CLI options (capture)
```
--rtp-url <url|sdp>      e.g. /app/config/rtp.sdp or rtp://0.0.0.0:5004?protocol_whitelist=file,udp,rtp
--out <dir>              Output directory (default: out)
--write-images 1|0       Enable/disable PNG output (default: 1)
--write-video 1|0        Enable/disable MP4 output (default: 1)
--fps <fps>              MP4 FPS (default: 30)
--mp4 <path>             Override MP4 output path
```

You can pass these via env in `docker-compose.yml` or:
```bash
./manage.sh start --rtp-url /app/config/rtp.sdp --write-images 1 --write-video 1 --fps 30
```

## Docker ports
- Janus HTTP: `8088`
- Janus WebSocket: `8188`
- Janus ICE/RTP: `10000-10200/udp`

## Troubleshooting
- If decoding fails, confirm the RTP payload type in `demo.html` matches `config/rtp.sdp`.
- Ensure your browser publishes VP8 (default in the demo).
- Use `./manage.sh logs` to inspect Janus and capture logs.

## Tests
```bash
ctest --test-dir build
```
