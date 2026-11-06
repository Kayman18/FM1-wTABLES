#!/bin/sh
set -e
cd "$(dirname "$0")"
PY="${PYTHON:-python3}"
: "${AC79_SDK:=$PWD/sdk}"
export AC79_SDK
exec "$PY" base/tools/build.py app "$@"
