#!/bin/bash
# Test script for SIGTSTP toggle with process group fix
cd "$(dirname "$0")"

echo "=== Starting estacao ==="
./bin/estacao &
PID=$!
echo "PID: $PID"

echo "=== Waiting 12s for normal operation ==="
sleep 12

echo ""
echo "=== Sending SIGTSTP (deactivate analysis) ==="
kill -TSTP $PID
sleep 10

echo ""
echo "=== Sending SIGTSTP again (reactivate analysis) ==="
kill -TSTP $PID
sleep 12

echo ""
echo "=== Sending SIGINT (terminate) ==="
kill -INT $PID

echo "=== Waiting for process to finish ==="
wait $PID
EXIT_CODE=$?
echo "=== Test complete (exit code: $EXIT_CODE) ==="

# Verify no zombies or leftover shm
sleep 1
echo "=== Checking for leftover processes ==="
ps aux | grep estacao | grep -v grep | grep -v test_toggle || echo "No leftover processes"
echo "=== Checking for leftover shm ==="
ls /dev/shm/ 2>/dev/null | grep estacao || echo "No leftover shm"
