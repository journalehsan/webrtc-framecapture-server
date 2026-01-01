# WebRTC Frame Capture Server (C++17)

A standalone C++ WebRTC receiver service (video only). It accepts an SDP offer over HTTP, returns an SDP answer, receives a WebRTC video track, and saves decoded frames as a PNG sequence. Optionally, it can write an MP4 using OpenCV's `cv::VideoWriter`.

## What’s implemented
- Standalone C++17 service with HTTP signaling and a single peer connection
- WebRTC receive pipeline built on native Google libwebrtc
- OpenCV I420 -> BGR conversion and frame capture to PNG/MP4
- Thread-safe frame writer (v1 uses a simple mutex)
- Basic unit tests for pixel conversion and file output

## Features
- HTTP signaling with `/offer` and `/candidate`
- WebRTC video-only receive pipeline (libwebrtc)
- OpenCV frame conversion and writing
- Thread-safe frame writer
- Minimal unit tests for conversion and writing

## Dependencies
- Ubuntu 22.04
- CMake >= 3.16
- OpenCV
- libwebrtc (native Google WebRTC build)
- cpp-httplib (single header)
- nlohmann/json (single header)

Install OpenCV:
```bash
sudo apt-get update
sudo apt-get install -y libopencv-dev
```

### libwebrtc
Build or obtain a libwebrtc build and note:
- `WEBRTC_INCLUDE_DIR`: path containing `api/`, `rtc_base/`, etc.
- `WEBRTC_LIBRARY`: path to the compiled WebRTC library (e.g. `libwebrtc.a` or `.so`)

### Header-only deps
Install or vendor the headers so they are in your include path:
- `httplib.h`
- `nlohmann/json.hpp`

## Build
```bash
cmake -S . -B build \
  -DWEBRTC_INCLUDE_DIR=/path/to/webrtc/include \
  -DWEBRTC_LIBRARY=/path/to/libwebrtc.a \
  -DTHIRD_PARTY_INCLUDE_DIR=/path/to/headers

cmake --build build -j
```

## Run with Docker
Docker assumes the service binary is `webrtc_capture` and supports:
`--http-port`, `--ice-udp-min`, `--ice-udp-max`, `--out`, `--write-images`, `--write-video`.
If your binary or flags differ, adjust `Dockerfile` and `docker-compose.yml` accordingly.

Libwebrtc headers/libs must be available in the build context, for example:
- `third_party/webrtc/include`
- `third_party/webrtc/libwebrtc.a`

```bash
./manage.sh setup
./manage.sh start
./manage.sh logs
```

Notes:
- Bridge mode (default) maps TCP 8080 and a UDP range for ICE.
- For local demos on Linux, host networking is simplest:
  `./manage.sh start --hostnet`
- In bridge mode, ensure the UDP range is open in your firewall.

## Run
```bash
./build/webrtc_capture --port 8080 --output out --write-mp4 --mp4-fps 30
```

Outputs:
- PNG frames: `out/frames/frame_00000001.png`
- MP4 (optional): `out/capture.mp4`

## HTTP API
- `POST /offer` body: `{ "type":"offer", "sdp":"..." }`
- `POST /candidate` body: `{ "sdpMid":"0", "sdpMLineIndex":0, "candidate":"candidate:..." }`

## Flow (high level)
1. Browser sends `/offer` with SDP
2. Server creates a `PeerConnection`, sets remote offer, returns `/answer`
3. Browser sends ICE candidates to `/candidate`
4. On `OnTrack`, the server attaches a `VideoFrameSink`
5. Each frame is converted to BGR and written to disk

## Project layout
- `src/app` App orchestrates config, WebRTC, and HTTP signaling
- `src/signaling` HTTP server (cpp-httplib + nlohmann/json)
- `src/webrtc` WebRTC factory, peer, SDP utils, observers, video sink
- `src/media` OpenCV conversion and frame writer
- `src/util` args parsing and logging
- `tests` unit tests

## Browser client sample
```html
<!doctype html>
<html>
  <body>
    <video id="local" autoplay playsinline muted></video>
    <script>
      const pc = new RTCPeerConnection();
      const video = document.getElementById('local');

      async function start() {
        const stream = await navigator.mediaDevices.getUserMedia({ video: true, audio: false });
        video.srcObject = stream;
        stream.getTracks().forEach(track => pc.addTrack(track, stream));

        pc.onicecandidate = async (event) => {
          if (!event.candidate) return;
          await fetch('http://localhost:8080/candidate', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({
              sdpMid: event.candidate.sdpMid,
              sdpMLineIndex: event.candidate.sdpMLineIndex,
              candidate: event.candidate.candidate
            })
          });
        };

        const offer = await pc.createOffer({ offerToReceiveVideo: false, offerToReceiveAudio: false });
        await pc.setLocalDescription(offer);

        const res = await fetch('http://localhost:8080/offer', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ type: 'offer', sdp: offer.sdp })
        });

        const answer = await res.json();
        await pc.setRemoteDescription(new RTCSessionDescription(answer));
      }

      start().catch(console.error);
    </script>
  </body>
</html>
```

## Tests
```bash
ctest --test-dir build
```

## Notes
- The server handles a single peer connection for v1.
- ICE candidates from the browser must be sent to `/candidate`.
- Audio is ignored.
