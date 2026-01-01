FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    ninja-build \
    pkg-config \
    git \
    libopencv-dev \
  && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Expect libwebrtc headers/libs to be available in the build context.
# Example paths (customize via build args if needed):
#   /app/third_party/webrtc/include
#   /app/third_party/webrtc/libwebrtc.a
ARG WEBRTC_INCLUDE_DIR=/app/third_party/webrtc/include
ARG WEBRTC_LIBRARY=/app/third_party/webrtc/libwebrtc.a
ARG THIRD_PARTY_INCLUDE_DIR=/app/third_party

COPY . /app

RUN cmake -S . -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DWEBRTC_INCLUDE_DIR=${WEBRTC_INCLUDE_DIR} \
    -DWEBRTC_LIBRARY=${WEBRTC_LIBRARY} \
    -DTHIRD_PARTY_INCLUDE_DIR=${THIRD_PARTY_INCLUDE_DIR} \
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
    ca-certificates \
  && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY --from=builder /app/bin/webrtc_capture /app/bin/webrtc_capture

ENV HTTP_PORT=8080 \
    ICE_UDP_MIN=30000 \
    ICE_UDP_MAX=30100 \
    OUT_DIR=/app/out \
    WRITE_IMAGES=1 \
    WRITE_VIDEO=1

ENTRYPOINT ["/bin/sh", "-c"]
CMD ["/app/bin/webrtc_capture --http-port ${HTTP_PORT} --ice-udp-min ${ICE_UDP_MIN} --ice-udp-max ${ICE_UDP_MAX} --out ${OUT_DIR} --write-images ${WRITE_IMAGES} --write-video ${WRITE_VIDEO}"]
