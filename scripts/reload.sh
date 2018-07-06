#!/bin/bash
set -euo pipefail

main() {
    while [[ "$#" -gt 0 ]]; do
        case "$1" in
        modules) modules; shift;;
        *) echo >&2 "invalid option: $1"; exit 1;;
        esac
    done
}

modules() {
    cat <<'EOF'
for k in pairs(package.loaded) do
    if string.find(k, "^nngn%.") then
        if k ~= "nngn.lib.map" then
            package.loaded[k] = nil
        end
    end
end
EOF
}

main "$@"
