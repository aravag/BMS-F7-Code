#!/usr/bin/env bash
# Copy the built .bin to the Jetson and flash the NUCLEO-H533RE plugged into it.
# The Nucleo's ST-LINK shows up as a USB drive (label NOD_H533RE); copying a .bin onto it flashes the board.
# (OpenOCD from apt on Ubuntu 22.04 is too old for STM32H5, so we don't use it.)
# Usage: JETSON=user@100.92.239.43 ./scripts/deploy_jetson.sh [Debug|Release]
set -euo pipefail
: "${JETSON:?set JETSON=user@host (e.g. the Tailscale IP)}"
CONFIG="${1:-Debug}"
BIN="$(dirname "$0")/../bms_h5/build/$CONFIG/bms_h5.bin"
[ -f "$BIN" ] || { echo "Build first: $BIN not found"; exit 1; }
ssh "$JETSON" 'mkdir -p ~/bms'
scp "$BIN" "$JETSON:~/bms/bms_h5.bin"
ssh -t "$JETSON" '
  set -e
  DEV=$(readlink -f /dev/disk/by-label/NOD_H533RE 2>/dev/null || true)
  [ -n "$DEV" ] || { echo "Nucleo drive NOD_H533RE not found. Is the ST-LINK USB cable plugged in?"; exit 1; }
  MNT=$(lsblk -no MOUNTPOINT "$DEV" | head -n1)
  if [ -z "$MNT" ]; then MNT=/mnt/nucleo; sudo mkdir -p $MNT; sudo mount "$DEV" $MNT; fi
  echo "Flashing via $MNT ..."
  sudo cp "$HOME/bms/bms_h5.bin" "$MNT/" && sync
  echo "Done. The ST-LINK flashes and resets the board (LED on the ST-LINK blinks while it works)."
'
