#!/bin/bash
cd "$(dirname "$0")/src"
make -j$(nproc) && {
  pkill -f "whatsie" || true
  sleep 0.5
  ./whatsie &
}
