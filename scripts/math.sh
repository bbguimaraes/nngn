#!/bin/bash
set -e
exec lua -e 'm = loadfile("src/lua/lib/math.lua")()' -i
