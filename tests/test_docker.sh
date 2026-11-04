#!/bin/sh
set -eu
docker build -t c-system-monitor:test .
for memory in 128 256; do
  output=$(docker run --rm --memory="${memory}m" --cpus=0.5 \
    --pids-limit=64 --network=none --read-only --cap-drop=ALL \
    c-system-monitor:test --json)
  printf '%s\n' "$output" | python3 -c '
import json, sys
expected = int(sys.argv[1])
data = json.load(sys.stdin)
assert data["uid"] == 65534, data
assert data["gid"] == 65534, data
assert data["memory_limit_mib"] == expected, data
assert abs(data["cpu_quota_cores"] - 0.5) < 0.001, data
print(f"PASS: non-root, {expected} MiB memory, 0.5 CPU")
' "$memory"
done
