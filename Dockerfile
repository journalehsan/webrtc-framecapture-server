FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

ARG HTTP_PROXY
ARG HTTPS_PROXY
ARG ALL_PROXY
ARG NO_PROXY
ENV HTTP_PROXY=${HTTP_PROXY} \
    HTTPS_PROXY=${HTTPS_PROXY} \
    ALL_PROXY=${ALL_PROXY} \
    NO_PROXY=${NO_PROXY} \
    http_proxy=${HTTP_PROXY} \
    https_proxy=${HTTPS_PROXY} \
    all_proxy=${ALL_PROXY} \
    no_proxy=${NO_PROXY}

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    ninja-build \
    pkg-config \
    libavcodec-dev \
    libavformat-dev \
    libavutil-dev \
    libswscale-dev \
    libopencv-dev \
  && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . /app

RUN cmake -S . -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
  && cmake --build build -j \
  && mkdir -p /app/bin \
  && cp build/webrtc_capture /app/bin/webrtc_capture

FROM ubuntu:22.04 AS runtime

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    libopencv-core4.5d \
    libopencv-imgcodecs4.5d \
    libopencv-imgproc4.5d \
    libopencv-videoio4.5d \
    libavformat58 \
    libavcodec58 \
    libavutil56 \
    libswscale5 \
    ca-certificates \
  && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY --from=builder /app/bin/webrtc_capture /app/bin/webrtc_capture
COPY config/rtp.sdp /app/config/rtp.sdp

ENV RTP_URL=/app/config/rtp.sdp \
    OUT_DIR=/app/out \
    WRITE_IMAGES=1 \
    WRITE_VIDEO=1 \
    FPS=30

ENTRYPOINT ["/bin/sh", "-c"]
CMD ["/app/bin/webrtc_capture --rtp-url ${RTP_URL} --out ${OUT_DIR} --write-images ${WRITE_IMAGES} --write-video ${WRITE_VIDEO} --fps ${FPS}"]
