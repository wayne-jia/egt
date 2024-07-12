#!/bin/bash

# Execute gfx-convert -s to start the conversion
gfx-convert -s

# Convert multiple *.png files into a *eraw.bin
gfx-convert -m

# Find all the .png file numbers
file_count=$(ls -1 *.png 2>/dev/null | wc -l)
echo "There are total: $file_count PNG files to be converted"

# Quit if no .png file
if [ $file_count -eq 0 ]; then
    echo "No .png files found in the current directory."
    exit 1
fi

# Process all the .png files
for file in *.png; do
    if [[ -f $file ]]; then
        # Convert a non-svg file: gfx-convert -i img filename
        gfx-convert -i img "$file"
    else
        echo "Warning: File $file not found."
    fi
done

# Finish the conversion
gfx-convert -e
