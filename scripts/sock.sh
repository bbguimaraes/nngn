#!/bin/bash
set -euo pipefail

rlwrap nc -NU "${1:-$(dirname "$BASH_SOURCE")/../sock}"
