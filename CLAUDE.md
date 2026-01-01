# CLAUDE.md

This repository is a Janus + RTP capture stack (no Google libwebrtc).

## Overview
- **janus**: Janus Gateway receives WebRTC and forwards RTP to the capture container.
- **capture**: C++17 service uses FFmpeg (libav*) to decode RTP and OpenCV to save frames/MP4.

## Build
```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

## Run with Docker
```bash
./manage.sh setup
./manage.sh start
./manage.sh logs
```

Open `demo.html` and publish webcam/screen.

## Key files
- `src/ingest/RtpReceiver.*` FFmpeg receiver
- `src/media/FrameWriter.*` OpenCV output
- `src/app/App.*` orchestration
- `config/rtp.sdp` RTP SDP used by the capture container
- `janus/janus.plugin.videoroom.jcfg` enables RTP forwarding

## CLI args
```
--rtp-url <url|sdp>
--out <dir>
--write-images 1|0
--write-video 1|0
--fps <fps>
--mp4 <path>
```
