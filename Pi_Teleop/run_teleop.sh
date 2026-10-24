#!/usr/bin/env bash
# Run on the laptop: copies the teleop to the Pi and starts it over SSH (arrow keys drive the car).
# Extra arguments go to arrow_teleop.py, e.g.  ./run_teleop.sh --duty 0.7   or   ./run_teleop.sh --dry-run
set -e
PI=${PI:-f0@10.0.16.135}
cd "$(dirname "$0")"
scp -q arrow_teleop.py "$PI":~/arrow_teleop.py
exec ssh -t "$PI" python3 ~/arrow_teleop.py "$@"
