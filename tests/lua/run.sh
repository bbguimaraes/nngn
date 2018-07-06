#!/bin/bash
set -euo pipefail

[[ "${srcdir:-}" ]] && cd "$srcdir"
lua tests/lua/unit.lua
