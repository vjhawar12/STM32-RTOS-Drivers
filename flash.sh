#!/bin/bash
set -euo pipefail

FIRMWARE_NAME="${1:-vehicle_telemetry}"
st-flash write "build/${FIRMWARE_NAME}.bin" 0x08000000
