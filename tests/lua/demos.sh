#!/bin/bash
set -euo pipefail

for x in $(find demos/ -name '*.lua' | sort); do
    nngn "@$x" @demos/check.lua
done
