#!/bin/bash
set -e

source "$(dirname "$0")/env.sh"

if ! command -v cppcheck >/dev/null 2>&1; then
    echo "ERROR: cppcheck not installed"
    exit 1
fi

echo "Running cppcheck on core and drivers"

cppcheck \
    --enable=all \
    --inline-suppr \
    --error-exitcode=1 \
    --suppress=missingIncludeSystem \
	-I "$PROJECT_ROOT/configuration" \
    -I "$PROJECT_ROOT/core/inc" \
    -I "$PROJECT_ROOT/drivers/inc" \
    "$PROJECT_ROOT/core" \
    "$PROJECT_ROOT/drivers"
