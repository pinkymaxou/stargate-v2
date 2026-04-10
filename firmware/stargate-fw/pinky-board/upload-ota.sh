#!/bin/bash
# OTA upload script for pinky-board
# Usage: ./upload-ota.sh [IP_ADDRESS]
IP=${1:-192.168.5.98}
BIN="$(dirname "$0")/build/stargate-fw.bin"

if [ ! -f "$BIN" ]; then
    echo "Error: firmware binary not found at $BIN"
    exit 1
fi

echo "Uploading $(basename $BIN) ($(du -h $BIN | cut -f1)) to http://$IP/ota/upload ..."

# The board reboots after flashing so curl will not receive an HTTP response —
# ignore the timeout error (exit 28) as long as 100% of bytes were uploaded.
curl --no-keepalive \
     --no-buffer \
     --max-time 120 \
     -X POST \
     -H "Content-Type: application/octet-stream" \
     --data-binary "@$BIN" \
     "http://$IP/ota/upload" || true

echo ""
echo "Upload sent — board is rebooting. Waiting for it to come back..."
for i in $(seq 1 20); do
    sleep 1
    if curl -s --max-time 2 "http://$IP/api/getstatus" > /dev/null 2>&1; then
        echo "Board is back online."
        exit 0
    fi
done
echo "Warning: board did not respond within 20s."
