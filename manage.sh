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

case "$cmd" in
  setup)
    mkdir -p out/frames
    docker_compose build
    ;;
  start)
    docker_compose up -d
    ;;
  stop)
    docker_compose down
    ;;
  restart)
    docker_compose down
    docker_compose up -d
    ;;
  logs)
    docker_compose logs -f
    ;;
  status)
    docker_compose ps
    ;;
  clean)
    docker_compose down --rmi all --volumes --remove-orphans
    ;;
  *)
    echo "Unknown command: $cmd" >&2
    usage
    exit 1
    ;;
esac
