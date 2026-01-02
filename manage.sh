#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'USAGE'
Usage: ./manage.sh <command> [options]

Commands:
  setup      Create output dirs and build the image
  start      Start the service (bridge mode by default)
  stop       Stop the service
  restart    Restart the service
  logs       Tail service logs
  status     Show compose status
  clean      Remove containers, images, volumes, orphans

Options:
  --hostnet           Use host networking (Linux-only)
  --dockerless        Run capture locally without Docker (setup/start only)
  --install-janus     Install Janus Gateway via yay (Arch, dockerless setup only)
  --rtp-url <url>     Override RTP URL/SDP (default /app/config/rtp.sdp)
  --write-images <0|1>  Enable/disable PNG output (default 1)
  --write-video <0|1>   Enable/disable MP4 output (default 1)
  --fps <fps>         Override video FPS (default 30)
USAGE
}

if [[ $# -lt 1 ]]; then
  usage
  exit 1
fi

cmd="$1"
shift

use_hostnet=0
dockerless=0
install_janus=0
rtp_url=""
write_images=""
write_video=""
fps=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    --hostnet)
      use_hostnet=1
      shift
      ;;
    --dockerless)
      dockerless=1
      shift
      ;;
    --install-janus)
      install_janus=1
      shift
      ;;
    --rtp-url)
      rtp_url="$2"
      shift 2
      ;;
    --write-images)
      write_images="$2"
      shift 2
      ;;
    --write-video)
      write_video="$2"
      shift 2
      ;;
    --fps)
      fps="$2"
      shift 2
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "Unknown option: $1" >&2
      usage
      exit 1
      ;;
  esac
done

compose_files=("-f" "docker-compose.yml")
if [[ "$use_hostnet" -eq 1 ]]; then
  compose_files+=("-f" "docker-compose.hostnet.yml")
fi

env_args=()
if [[ -n "$rtp_url" ]]; then
  env_args+=("RTP_URL=$rtp_url")
fi
if [[ -n "$write_images" ]]; then
  env_args+=("WRITE_IMAGES=$write_images")
fi
if [[ -n "$write_video" ]]; then
  env_args+=("WRITE_VIDEO=$write_video")
fi
if [[ -n "$fps" ]]; then
  env_args+=("FPS=$fps")
fi

docker_compose() {
  env "${env_args[@]}" docker compose "${compose_files[@]}" "$@"
}

dockerless_setup() {
  mkdir -p out/frames
  if [[ "$install_janus" -eq 1 ]]; then
    if ! command -v yay >/dev/null 2>&1; then
      echo "Missing yay. Install yay first or omit --install-janus." >&2
      exit 1
    fi
    yay -S --needed janus-gateway
  fi
  if command -v ninja >/dev/null 2>&1; then
    cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
  else
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
  fi
  cmake --build build -j
}

dockerless_start_janus() {
  if ! command -v janus >/dev/null 2>&1; then
    echo "Janus binary not found. Install it or omit dockerless mode." >&2
    exit 1
  fi

  local janus_lib=""
  if [[ -d /usr/lib/janus ]]; then
    janus_lib="/usr/lib/janus"
  elif [[ -d /usr/local/lib/janus ]]; then
    janus_lib="/usr/local/lib/janus"
  else
    echo "Could not find Janus lib directory in /usr/lib/janus or /usr/local/lib/janus" >&2
    exit 1
  fi

  mkdir -p out/janus-config
  cp janus/janus.plugin.videoroom.jcfg out/janus-config/janus.plugin.videoroom.jcfg

  local cert_path="$(pwd)/out/janus-config/janus-cert.pem"
  local key_path="$(pwd)/out/janus-config/janus-key.pem"
  if [[ ! -f "$cert_path" || ! -f "$key_path" ]]; then
    if ! command -v openssl >/dev/null 2>&1; then
      echo "Missing openssl for generating DTLS certs." >&2
      exit 1
    fi
    openssl req -x509 -nodes -newkey rsa:2048 \
      -keyout "$key_path" \
      -out "$cert_path" \
      -days 3650 \
      -subj "/CN=janus"
  fi

  cat > out/janus-config/janus.jcfg <<EOF
general: {
  configs_folder = "$(pwd)/out/janus-config"
  plugins_folder = "${janus_lib}/plugins"
  transports_folder = "${janus_lib}/transports"
  events_folder = "${janus_lib}/events"
  interface = "127.0.0.1"
  log_to_stdout = true
}

nat: {
  ice_lite = true
  ice_tcp = false
  ice_enforce_list = "lo"
}

certificates: {
  cert_pem = "${cert_path}"
  cert_key = "${key_path}"
}
EOF

  cat > out/janus-config/janus.transport.http.jcfg <<'EOF'
general: {
  json = "indented"
  base_path = "/janus"
  http = true
  port = 8088
  ip = "0.0.0.0"
  https = false
}

cors: {
  allow_origin = "*"
  enforce_cors = false
}
EOF

  if [[ -f out/janus.pid ]] && kill -0 "$(cat out/janus.pid)" 2>/dev/null; then
    echo "Janus already running (pid $(cat out/janus.pid))"
    return
  fi

  echo "Starting Janus locally..."
  nohup janus -C out/janus-config/janus.jcfg > out/janus.log 2>&1 &
  echo $! > out/janus.pid
  sleep 1
}

dockerless_stop_janus() {
  if [[ -f out/janus.pid ]] && kill -0 "$(cat out/janus.pid)" 2>/dev/null; then
    echo "Stopping Janus (pid $(cat out/janus.pid))"
    kill "$(cat out/janus.pid)" || true
    return
  fi
  if pkill -f "janus -C out/janus-config/janus.jcfg" >/dev/null 2>&1; then
    echo "Stopped Janus (matched command line)"
    return
  fi
  echo "No running Janus process found for dockerless mode"
}

dockerless_start() {
  if [[ ! -x build/webrtc_capture ]]; then
    echo "Missing build/webrtc_capture. Run: ./manage.sh setup --dockerless" >&2
    exit 1
  fi

  dockerless_start_janus

  local rtp_arg="${rtp_url:-config/rtp.sdp}"
  local out_dir="out"
  mkdir -p "${out_dir}/frames"
  local args=(--rtp-url "$rtp_arg" --out "$out_dir")
  if [[ -n "$write_images" ]]; then
    args+=(--write-images "$write_images")
  else
    args+=(--write-images 1)
  fi
  if [[ -n "$write_video" ]]; then
    args+=(--write-video "$write_video")
  fi
  if [[ -n "$fps" ]]; then
    args+=(--fps "$fps")
  fi

  echo "Running capture locally (Ctrl+C to stop): ./build/webrtc_capture ${args[*]}"
  ./build/webrtc_capture "${args[@]}"
}

case "$cmd" in
  setup)
    if [[ "$dockerless" -eq 1 ]]; then
      dockerless_setup
    else
      mkdir -p out/frames
      docker_compose build
    fi
    ;;
  start)
    if [[ "$dockerless" -eq 1 ]]; then
      dockerless_start
    else
      docker_compose up -d
    fi
    ;;
  stop)
    if [[ "$dockerless" -eq 1 ]]; then
      dockerless_stop_janus
      echo "dockerless mode: stop capture by Ctrl+C in the terminal running ./build/webrtc_capture"
    else
      docker_compose down
    fi
    ;;
  restart)
    if [[ "$dockerless" -eq 1 ]]; then
      dockerless_stop_janus
      echo "dockerless mode: stop/start capture manually with Ctrl+C then ./manage.sh start --dockerless"
    else
      docker_compose down
      docker_compose up -d
    fi
    ;;
  logs)
    if [[ "$dockerless" -eq 1 ]]; then
      echo "dockerless mode: logs are printed in the running terminal"
    else
      docker_compose logs -f
    fi
    ;;
  status)
    if [[ "$dockerless" -eq 1 ]]; then
      echo "dockerless mode: no Docker status available"
    else
      docker_compose ps
    fi
    ;;
  clean)
    if [[ "$dockerless" -eq 1 ]]; then
      echo "dockerless mode: remove ./build and ./out manually if desired"
    else
      docker_compose down --rmi all --volumes --remove-orphans
    fi
    ;;
  *)
    echo "Unknown command: $cmd" >&2
    usage
    exit 1
    ;;
esac
