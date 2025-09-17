#!/bin/bash
# arg 1 filename stem (no .trace), arg 2 "title", arg 3/4 spantrim args

# Must sort by pure byte values, not local collating sequence
export LC_ALL=C

# Strip trailing .trace if it is there
var1=${1%.trace}

# Extract date and time from filename (e.g., ku_20240709_152922_dclab-2_11686.trace)
# Assuming filename starts with ku_YYYYMMDD_HHMMSS
filename=$(basename "$var1")
if [[ $filename =~ ku_([0-9]{8})_([0-9]{6}) ]]; then
    date_part=${BASH_REMATCH[1]}
    time_part=${BASH_REMATCH[2]}
    output_dir="ku_${date_part}_${time_part}"
else
    echo "Error: Filename does not match expected pattern ku_YYYYMMDD_HHMMSS..."
    exit 1
fi

# Create the output directory
mkdir -p "$output_dir"

# Process the trace file and output to the new directory
cat "$var1.trace" | ./rawtoevent | sort -n | ./eventtospan3 "$2" | sort > "$output_dir/$filename.json"
echo "  $output_dir/$filename.json written"

trim_arg='0'
if [ -n "$3" ]; then
    delimit=' '
    trim_arg=$3$delimit$4
fi

cat "$output_dir/$filename.json" | ./spantotrim $trim_arg | ./makeself show_cpu.html > "$output_dir/$filename.html"
echo "  $output_dir/$filename.html written"

# Move the original trace file to the output directory
mv "$var1.trace" "$output_dir/$filename.trace"
echo "  $output_dir/$filename.trace moved"

# Open the HTML file in Google Chrome
google-chrome "$output_dir/$filename.html" &
