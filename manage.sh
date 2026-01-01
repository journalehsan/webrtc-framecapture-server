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
  --http-port <port>  Override HTTP port (default 8080)
  --udp-min <port>    Override ICE UDP min (default 30000)
  --udp-max <port>    Override ICE UDP max (default 30100)
USAGE
}

if [[ $# -lt 1 ]]; then
  usage
  exit 1
fi

cmd="$1"
shift

use_hostnet=0
http_port=""
udp_min=""
udp_max=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    --hostnet)
      use_hostnet=1
      shift
      ;;
    --http-port)
      http_port="$2"
      shift 2
      ;;
    --udp-min)
      udp_min="$2"
      shift 2
      ;;
    --udp-max)
      udp_max="$2"
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
if [[ -n "$http_port" ]]; then
  env_args+=("HTTP_PORT=$http_port")
fi
if [[ -n "$udp_min" ]]; then
  env_args+=("ICE_UDP_MIN=$udp_min")
fi
if [[ -n "$udp_max" ]]; then
  env_args+=("ICE_UDP_MAX=$udp_max")
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
