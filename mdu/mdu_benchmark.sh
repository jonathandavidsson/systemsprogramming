#!/bin/bash

EXECUTABLE="./mdu"

DIRECTORY="/pkg/"

OUTPUT_FILE="mdu_results.txt"

echo "" > $OUTPUT_FILE

for THREADS in {1..100}; do
    echo "Running with $THREADS threads..."

    TIMEFORMAT='%3R'
    { time $EXECUTABLE -j "$THREADS" "$DIRECTORY" > /dev/null; } 2>> "$OUTPUT_FILE"
done
echo "Finished"