#!/bin/bash

echo "Start of the script"

# Run a command in the background
sleep 5 &
bg_process_pid=$!
echo "Background process with PID $bg_process_pid started"

# Wait for a while
sleep 2

# Run another background command
sleep 3 &
bg_process_pid=$!
echo "Background process with PID $bg_process_pid started"

# Wait for all background processes to finish
wait

echo "End of the script"
