#!/bin/bash
set -euo pipefail

websocket 0.0.0.0:8000 tools/camera_server/index.html \
    | camera_server sock
