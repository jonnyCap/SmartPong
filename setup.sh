#!/usr/bin/env bash
# SmartPong Setup & Configuration Launcher
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

if ! command -v python3 &> /dev/null; then
    echo "Error: Python 3 is required to run the configuration utility." >&2
    exit 1
fi

python3 "${SCRIPT_DIR}/configure.py" "$@"
