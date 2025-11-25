#!/usr/bin/env bash
set -euo pipefail

# Creates a virtualenv in .venv and installs test dependencies
python3 -m venv .venv
. .venv/bin/activate
python -m pip install --upgrade pip
pip install -r requirements.txt

echo "Virtualenv created and packages installed. Activate with: source .venv/bin/activate"
