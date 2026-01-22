#!/bin/bash
set -e

ROOT_DIR=$(cd "$(dirname "$0")/.." && pwd)
source "$ROOT_DIR/cicd_tools/env.sh"

cd "$ROOT_DIR"

echo "Cleaning..."
make clean

echo "Building..."
make

echo "Build completed successfully"


